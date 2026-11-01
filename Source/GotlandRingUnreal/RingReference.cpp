#include "RingCar.h"
#include "RingReferenceData.h"
#include "RingRouteData.h"
#include "RingAudio.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
namespace {
float ReferenceLength(){const auto& A=Ring::RouteData[UE_ARRAY_COUNT(Ring::RouteData)-1];const auto&B=Ring::RouteData[0];return A.Distance+FVector2D(A.CX-B.CX,A.CY-B.CY).Size();}
float HorizontalFov(float Vertical,float Aspect){return FMath::RadiansToDegrees(2*FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Vertical)*.5f)*Aspect));}
}
FVector ARingCar::ReferencePoint(float Distance)const{
 const float Length=ReferenceLength();Distance=FMath::Fmod(Distance,Length);if(Distance<0)Distance+=Length;
 int Low=0,High=UE_ARRAY_COUNT(Ring::RouteData)-1;
 while(Low<High){int Mid=(Low+High+1)/2;if(Ring::RouteData[Mid].Distance<=Distance)Low=Mid;else High=Mid-1;}
 int Next=(Low+1)%UE_ARRAY_COUNT(Ring::RouteData);const auto&A=Ring::RouteData[Low];const auto&B=Ring::RouteData[Next];
 float T=(Distance-A.Distance)/((Next==0?Length:B.Distance)-A.Distance);
 return FVector(FMath::Lerp(A.LX,B.LX,T)*100,FMath::Lerp(A.LY,B.LY,T)*100,FMath::Lerp(A.Z,B.Z,T)*100);
}
void ARingCar::ToggleReferenceLap(){
 if(ReferenceActive){
  ReferenceActive=false;ReplayBonnet->DestroyComponent();ReplayCockpit->DestroyComponent();ReplayBonnet=nullptr;ReplayCockpit=nullptr;BonnetTexture=nullptr;CockpitTexture=nullptr;
  SetActorTransform(ReferenceDrivePose,false,nullptr,ETeleportType::TeleportPhysics);
  for(int I=0;I<Visuals.Num();I++)Visuals[I]->SetRelativeTransform(ReferenceVisualPoses[I]);
  AnimateCockpitSteering(Dynamics.SteeringDegrees);UpdateInstruments(Dynamics.Motor.Rpm(),SpeedKmh/3.6f,Dynamics.Throttle<0?-1:Dynamics.Gear);
  SetDrivePaused(ReferenceWasPaused);UpdateView(1.f/60,nullptr);
 }else{
  ReferenceWasPaused=DrivePaused;if(MenuWidget.IsValid())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());SetDrivePaused(true);ReferenceDrivePose=GetActorTransform();ReferenceVisualPoses.Empty();
  for(auto V:Visuals)ReferenceVisualPoses.Add(V->GetRelativeTransform());
  ReferenceActive=true;ReferencePaused=false;ReferenceRoll=0;
  auto Capture=[this](UTextureRenderTarget2D*& Target,int Width,int Height){
   Target=NewObject<UTextureRenderTarget2D>(this);Target->RenderTargetFormat=RTF_RGBA16f;Target->bForceLinearGamma=true;Target->TargetGamma=1;Target->InitAutoFormat(Width,Height);
   auto* C=NewObject<USceneCaptureComponent2D>(this);C->SetupAttachment(Chassis);C->TextureTarget=Target;C->CaptureSource=SCS_FinalToneCurveHDR;
   C->PostProcessSettings=Camera->PostProcessSettings;C->PostProcessSettings.bOverride_AutoExposureBias=true;C->PostProcessSettings.AutoExposureBias=0;C->bAlwaysPersistRenderingState=true;C->bCaptureEveryFrame=false;C->bCaptureOnMovement=false;C->RegisterComponent();AddInstanceComponent(C);return C;
  };
  UTextureRenderTarget2D* B;ReplayBonnet=Capture(B,1600,590);BonnetTexture=B;
  UTextureRenderTarget2D* C;ReplayCockpit=Capture(C,800,310);CockpitTexture=C;
  ReplayBonnet->SetRelativeLocation(FVector(102,0,54));ReplayBonnet->SetRelativeRotation(FRotator(7,0,0));ReplayBonnet->FOVAngle=HorizontalFov(48,1920.f/708);
  ReplayCockpit->SetRelativeLocation(FVector(-12.4041891,-38.8264,68.379341));ReplayCockpit->SetRelativeRotation(FRotator(-8.45f,0,0));ReplayCockpit->FOVAngle=HorizontalFov(58,960.f/372);
  for(int I=0;I<Visuals.Num();I++)if(VisualWheels[I]==-2){ReplayCockpit->HiddenComponents.Add(Visuals[I]);Visuals[I]->SetVisibility(true);}
  SeekReference(0);TickReference(0);
 }
 if(auto* PC=Cast<APlayerController>(GetController())){PC->bShowMouseCursor=ReferenceActive||DrivePaused;PC->SetInputMode(FInputModeGameOnly());FSlateApplication::Get().SetAllUserFocusToGameViewport();}
 if(!ReferenceActive&&MenuWidget.IsValid())BuildMenu();
}
void ARingCar::SeekReference(float Seconds){ReferenceSeconds=FMath::Clamp(Seconds,0.f,UE_ARRAY_COUNT(Ring::ReferenceFrames)/Ring::ReferenceFps);ReferenceRoll=0;ReplayRide=FRingRide();}
void ARingCar::TickReference(float Dt){
 const float Duration=UE_ARRAY_COUNT(Ring::ReferenceFrames)/Ring::ReferenceFps;
 const float Advance=ReferencePaused?0:FMath::Min(Dt,Duration-ReferenceSeconds);ReferenceSeconds+=Advance;
 float Index=FMath::Clamp(ReferenceSeconds*Ring::ReferenceFps,0.f,float(UE_ARRAY_COUNT(Ring::ReferenceFrames)-1));int I=FMath::FloorToInt(Index),J=FMath::Min(I+1,int(UE_ARRAY_COUNT(Ring::ReferenceFrames)-1));float T=Index-I;
 const auto&A=Ring::ReferenceFrames[I];const auto&B=Ring::ReferenceFrames[J];
 const float Distance=FMath::Lerp(A.Distance,B.Distance,T),Speed=FMath::Lerp(A.Speed,B.Speed,T),Rpm=FMath::Lerp(A.Rpm,B.Rpm,T),Load=FMath::Lerp(A.Load,B.Load,T);
 FVector Position=ReferencePoint(Distance),Normal=FVector::UpVector;
 FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(RingReferenceSurface),true,this);
 if(CircuitRoad->LineTraceComponent(Hit,Position+FVector(0,0,2000),Position-FVector(0,0,2000),Query)){Position.Z=Hit.ImpactPoint.Z;Normal=Hit.Normal;}
 FVector Forward=FVector::VectorPlaneProject(ReferencePoint(Distance+3)-ReferencePoint(Distance-3),Normal).GetSafeNormal();
 Position+=Normal*52;SetActorLocationAndRotation(Position,FRotationMatrix::MakeFromXZ(Forward,Normal).Rotator(),false,nullptr,ETeleportType::TeleportPhysics);
 FVector Before=Position-Normal*52-ReferencePoint(Distance-6),After=ReferencePoint(Distance+6)-(Position-Normal*52);Before.Z=After.Z=0;
 float Curvature=FMath::Atan2(FVector::CrossProduct(Before,After).Z,FVector::DotProduct(Before,After))/6;
 float Angle=FMath::RadiansToDegrees(FMath::Atan(2.52f*Curvature));AnimateCockpitSteering(Angle);UpdateInstruments(Rpm,Speed,A.Gear);
 ReferenceRoll+=Speed*Advance/.33865723f;
 for(int K=0;K<Visuals.Num();K++)if(VisualWheels[K]>=0){int W=VisualWheels[K];Visuals[K]->SetRelativeRotation(FRotator(0,W<2?Angle:0,0).Quaternion()*FRotator(-FMath::RadiansToDegrees(ReferenceRoll),0,0).Quaternion());}
 ReferenceClock=FMath::Clamp(ReferenceSeconds-Ring::ReferenceLapStart,0.f,Ring::ReferenceLapDuration);
 ReplayRide.Tick(Advance,Forward*Speed,GetActorQuat(),SurfaceContacts());
 const float Amount=Settings.CockpitMovement;
 ReplayCockpit->SetRelativeLocation(FVector(-12.4041891,-38.8264,68.379341)+ReplayRide.Offset*(100*Amount));
 ReplayCockpit->SetRelativeRotation(FRotator(-8.45f+ReplayRide.Angles.X*Amount,0,ReplayRide.Angles.Z*Amount));
 ReplayBonnet->SetRelativeLocation(FVector(102,0,54)+ReplayRide.RoadOffset*(25*Amount));
 ReplayBonnet->SetRelativeRotation(FRotator(7+ReplayRide.RoadAngles.X*Amount*.25f,0,ReplayRide.RoadAngles.Z*Amount*.25f));
 const FVector Contacts=ReplayRide.Contacts*Settings.SurfaceSound;EngineAudio->SetSurface(Contacts.X,Contacts.Y,Contacts.Z);
 EngineAudio->SetEngine(Rpm,Load,Speed,0,Muted||ReferencePaused||ReferenceSeconds>=Duration);
 // The replay advances independently of the suspended driving lap and physics.
 if(Advance>0)AnimateWindRotors(Advance);
 ReplayBonnet->CaptureScene();ReplayCockpit->CaptureScene();
}
