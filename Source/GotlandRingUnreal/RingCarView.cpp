#include "RingCar.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
void ARingCar::UpdateView(float Dt,APlayerController* PC){
 if(PC&&!DrivePaused){
  float X,Y;PC->GetInputMouseDelta(X,Y);X*=.1f;Y*=.1f; // Unity InputManager mouse sensitivity.
  auto* Viewport=GetWorld()->GetGameViewport()->Viewport;
  bool Captured=Viewport->HasFocus()&&Viewport->HasMouseCapture();
  if(!Captured||!HadMouseCapture||Pressed(EKeys::U)||Pressed(EKeys::C)||PC->WasInputKeyJustPressed(EKeys::Escape))IgnoreMouseFrames=2;
  HadMouseCapture=Captured;
  if(IgnoreMouseFrames>0){X=Y=0;IgnoreMouseFrames--;}

  if(MouseControl){if(!Autopilot)MouseSteering=FMath::Clamp(MouseSteering+X*.04f,-1.f,1.f);}
  else {LookYaw+=X*2;LookPitch=FMath::Clamp(LookPitch-Y*1.5f,ViewMode>=2?-13.f:-45.f,ViewMode>=2?57.f:40.f);LookYaw=ViewMode>=2?FMath::UnwindDegrees(LookYaw):FMath::Clamp(LookYaw,-115.f,115.f);}
  if(PC->WasInputKeyJustPressed(EKeys::RightMouseButton)){LookYaw=LookPitch=MouseSteering=0;}
 }
 FVector Lean=FVector::ZeroVector,Tilt=FVector::ZeroVector;
 if(!DrivePaused){
  FVector Velocity=Settings.AwdMode?Chassis->GetPhysicsLinearVelocity()/100:ArcadeVelocity;
  FVector Acceleration=GetActorTransform().InverseTransformVectorNoScale((Velocity-PreviousVelocity)/Dt);Acceleration.Z=0;Acceleration=Acceleration.GetClampedToMaxSize(15);
  if(Velocity.Size()<.2f)Acceleration=FVector::ZeroVector;
  PreviousVelocity=Velocity;ViewAcceleration=FMath::Lerp(ViewAcceleration,Acceleration,1-FMath::Exp(-Dt/.14f));
  float Asphalt=0,Loose=0;for(const auto& W:Dynamics.Wheels)if(W.Grounded){if(W.Asphalt)Asphalt+=.25f;else Loose+=.25f;}
  RoadMix=FMath::Lerp(RoadMix,Asphalt,1-FMath::Exp(-Dt/.06f));LooseMix=FMath::Lerp(LooseMix,Loose,1-FMath::Exp(-Dt/.06f));
  ViewLean=FMath::Lerp(ViewLean,FVector(-ViewAcceleration.X*.0025f,-ViewAcceleration.Y*.002f,0),1-FMath::Exp(-Dt/.12f));
  ViewTilt=FMath::Lerp(ViewTilt,FVector(ViewAcceleration.X*.09f,0,-ViewAcceleration.Y*.07f),1-FMath::Exp(-Dt/.12f));
  ViewDistance=FMath::Fmod(ViewDistance+Velocity.Size()*Dt,1000.f);
  float Rough=(RoadMix*.0012f+LooseMix*.014f)*FMath::Clamp((Velocity.Size()-.5f)/11.5f,0.f,1.f);
  float Wave=.65f*FMath::Sin(ViewDistance*2*PI/2.5f)+.35f*FMath::Sin(ViewDistance*2*PI/.8f);
  FVector RoadOffset(0,FMath::Sin(ViewDistance*2*PI/1.25f)*Rough*.22f,Wave*Rough);
  FVector RoadAngles(-Wave*Rough*24,0,-FMath::Sin(ViewDistance*2*PI/5)*Rough*32);
  Lean=(ViewMode==0?(ViewLean+RoadOffset).GetClampedToMaxSize(.045f):RoadOffset*.25f)*(100*Settings.CockpitMovement);
  Tilt=(ViewMode==0?ViewTilt+RoadAngles:RoadAngles*.25f)*Settings.CockpitMovement;
 }
 Boom->bEnableCameraLag=false;Boom->bDoCollisionTest=ViewMode==2;
 Camera->SetRelativeLocation(FVector::ZeroVector);Camera->SetRelativeRotation(FRotator::ZeroRotator);
 if(ViewMode==3){
  // Unity drone uses world up and yaw only, independent of chassis pitch/roll.
  const float Heading=GetActorRotation().Yaw+LookYaw;
  const FVector Position=GetActorLocation()+FVector::UpVector*(Settings.DroneHeight*100)-FRotator(0,Heading,0).Vector()*(Settings.DroneDistance*100);
  const float Pitch=FMath::RadiansToDegrees(FMath::Atan2(Settings.DroneHeight,Settings.DroneDistance))*.5f;
  Boom->TargetArmLength=0;Boom->SetWorldLocationAndRotation(Position,FRotator(-Pitch-LookPitch,Heading,0));
 }else if(ViewMode==2){
  ChaseYaw=FMath::Lerp(ChaseYaw,SpeedKmh>28.8f?FMath::Clamp(SideslipDegrees,-20.f,20.f):0.f,1-FMath::Exp(-Dt*5));
  Boom->TargetArmLength=550;Boom->SetRelativeLocation(FVector(0,0,22));Boom->SetRelativeRotation(FRotator(-18-LookPitch,LookYaw+ChaseYaw,0));
 }else{
  Boom->TargetArmLength=0;Boom->SetRelativeLocation(ViewMode==0?FVector(-12.4041891,-38.8264,68.379341):FVector(102,0,54));
  Boom->SetRelativeLocation(Boom->GetRelativeLocation()+Lean);
  Boom->SetRelativeRotation(FRotator((ViewMode==0?-8.45f:-2.f)-LookPitch+Tilt.X,LookYaw,Tilt.Z));
 }
 const float Aspect=GetWorld()->GetGameViewport()->Viewport->GetSizeXY().X/float(FMath::Max(1,GetWorld()->GetGameViewport()->Viewport->GetSizeXY().Y));
 const float Vertical=76+SpeedKmh/3.6f*.12f;
 const float Horizontal=FMath::RadiansToDegrees(2*FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Vertical)*.5f)*Aspect));
 Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Horizontal,Dt,3));
 for(int32 I=0;I<Visuals.Num();I++)if(VisualWheels[I]==-2)Visuals[I]->SetVisibility(ViewMode!=0);
}
void ARingCar::RecoverCar(){
 Laps.Discontinuity();
 FVector P=GetActorLocation()/100;float Fraction,Offset;bool OnRoad;
 int I=Pilot->Nearest({float(P.X),float(P.Y)},Fraction,Offset,OnRoad),J=Pilot->Next(I);
 const auto& R=Pilot->Rows[I];const auto& N=Pilot->Rows[J];
 FVector Position(FMath::Lerp(R.CX,N.CX,Fraction)*100,FMath::Lerp(R.CY,N.CY,Fraction)*100,FMath::Lerp(R.Z,N.Z,Fraction)*100+60);
 FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(RingRecovery),true,this);
 if(RoadSurface->LineTraceComponent(Hit,Position+FVector(0,0,1000),Position-FVector(0,0,1000),Query))Position.Z=Hit.ImpactPoint.Z+60;
 FVector Forward(N.CX-R.CX,N.CY-R.CY,N.Z-R.Z);
 Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);Chassis->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
 SetActorLocationAndRotation(Position,FRotationMatrix::MakeFromXZ(Forward,Hit.bBlockingHit?Hit.Normal:FVector::UpVector).Rotator(),false,nullptr,ETeleportType::TeleportPhysics);
 Dynamics=Ring::Dynamics();ApplyDrivingSettings();Pilot->Reset();Chassis->WakeAllRigidBodies();
}
