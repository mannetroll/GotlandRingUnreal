#include "RingCar.h"
#include "RingAudio.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

ARingCar::ARingCar() {
 PrimaryActorTick.bCanEverTick=true;
 PrimaryActorTick.TickGroup=TG_PrePhysics;
 Chassis=CreateDefaultSubobject<UBoxComponent>(TEXT("Chassis"));
 SetRootComponent(Chassis); Chassis->SetBoxExtent(FVector(195,80,24));
 Chassis->SetCollisionProfileName(TEXT("PhysicsActor"));
 Chassis->SetSimulatePhysics(true); Chassis->SetMassOverrideInKg(NAME_None,1360,true);
 Chassis->SetLinearDamping(0); Chassis->SetAngularDamping(.12f);
 Chassis->SetCenterOfMass(FVector((.58f-.5f)*252,0,0));
 Chassis->BodyInstance.bUseCCD=true;
 Boom=CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseBoom")); Boom->SetupAttachment(Chassis);
 Boom->TargetArmLength=650; Boom->SetRelativeLocation(FVector(0,0,110));
 Boom->SetRelativeRotation(FRotator(-12,0,0)); Boom->bEnableCameraLag=true; Boom->CameraLagSpeed=6;
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));Camera->SetupAttachment(Boom);
 Camera->FieldOfView=75;
 Camera->PostProcessSettings.bOverride_AutoExposureMethod=true;
 Camera->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
 Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
 Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
 EngineAudio=CreateDefaultSubobject<URingAudio>(TEXT("EngineAudio"));EngineAudio->SetupAttachment(Chassis);
 AutoPossessPlayer=EAutoReceiveInput::Player0;
 WheelCenters={FVector(126,-73,30.57),FVector(126,73,30.57),FVector(-126,-73,30.57),FVector(-126,73,30.57)};
}
void ARingCar::BeginPlay() {
 Super::BeginPlay(); StartPose=GetActorTransform(); TestStart=GetActorLocation();
 DynamicsTest=FParse::Param(FCommandLine::Get(),TEXT("RingDynamicsTest"));
 TestMode=DynamicsTest||FParse::Param(FCommandLine::Get(),TEXT("RingDrivingTest"));
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It){
  auto* Mesh=It->GetStaticMeshComponent();
  if(Mesh->GetStaticMesh()&&Mesh->GetStaticMesh()->GetName()==TEXT("Road"))RoadSurface=Mesh;
 }
 checkf(RoadSurface.IsValid(),TEXT("The driving level must contain the migrated Road mesh"));
 if(TestMode)EngineAudio->CaptureTestAudio();
 EngineAudio->Start();
 for(int32 I=0;I<VisualMeshes.Num();I++) {
  auto* V=NewObject<UStaticMeshComponent>(this); V->SetupAttachment(Chassis);
  V->SetStaticMesh(VisualMeshes[I]);V->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  V->RegisterComponent();AddInstanceComponent(V);Visuals.Add(V);
  V->SetRelativeLocation((VisualWheels[I]<0?FVector::ZeroVector:WheelCenters[VisualWheels[I]])-FVector(0,0,48));
 }
 if(TestMode) {
  FString Data;FFileHelper::LoadFileToString(Data,*(FPaths::ProjectDir()/TEXT("Validation/collision_probes.json")));
  TArray<TSharedPtr<FJsonValue>> Probes;auto Reader=TJsonReaderFactory<>::Create(Data);FJsonSerializer::Deserialize(Reader,Probes);
  int32 Misses=0;float Worst=0;FCollisionQueryParams Query(SCENE_QUERY_STAT(RingRoadValidation),true,this);
  for(auto P:Probes){auto A=P->AsArray();FVector V(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());FHitResult H;
   if(!GetWorld()->LineTraceSingleByChannel(H,V+FVector(0,0,50),V-FVector(0,0,100),ECC_Visibility,Query))Misses++;
   else Worst=FMath::Max(Worst,FMath::Abs(H.ImpactPoint.Z-V.Z));
  }
  FString Result=FString::Printf(TEXT("{\"probes\":%d,\"misses\":%d,\"max_height_error_cm\":%.5f,\"passed\":%s}"),Probes.Num(),Misses,Worst,(Probes.Num()==7218&&Misses==0&&Worst<5)?TEXT("true"):TEXT("false"));
  FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("collision_validation.json")));
 }
 if(DynamicsTest)SetupDynamicsTest();
 if(auto* PC=Cast<APlayerController>(GetController())) {PC->SetViewTarget(this);PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;}
}
void ARingCar::ResetCar() {
 Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
 SetActorTransform(StartPose,false,nullptr,ETeleportType::TeleportPhysics);Chassis->WakeAllRigidBodies();Dynamics=Ring::Dynamics();
}
void ARingCar::Tick(float Dt) {
 Super::Tick(Dt);Elapsed+=Dt;
 auto* PC=Cast<APlayerController>(GetController());
 float Throttle=0, Brake=0, Steering=0;bool Handbrake=false,ReverseRequest=false;
 if(PC) {
  Throttle=(PC->IsInputKeyDown(EKeys::W)||PC->IsInputKeyDown(EKeys::Up))?1.f:0.f;
  ReverseRequest=PC->IsInputKeyDown(EKeys::X);
  Brake=(PC->IsInputKeyDown(EKeys::S)||PC->IsInputKeyDown(EKeys::Down))?1.f:0.f;
  Steering=(PC->IsInputKeyDown(EKeys::D)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::A)?1.f:0.f);
  Handbrake=PC->IsInputKeyDown(EKeys::SpaceBar);
  if(PC->WasInputKeyJustPressed(EKeys::R))ResetCar();
  if(PC->WasInputKeyJustPressed(EKeys::M))Muted=!Muted;
 }
 if(DynamicsTest)RunDynamicsTest(Throttle,Brake,Steering);
 // A reproducible standalone physics smoke test uses the same force path as keyboard driving.
 if(TestMode&&!DynamicsTest) {
  Throttle=(Elapsed>2&&Elapsed<4)?1.f:((Elapsed>9&&Elapsed<12)?.35f:0.f);
  Brake=(Elapsed>=4&&Elapsed<8)?1:0;
  Steering=(Elapsed>9&&Elapsed<12)?.2f:0;
  if(Elapsed>=4&&BrakeStart==0)BrakeStart=SpeedKmh;
  if(Elapsed>=8&&!SteeringTestStarted){BrakeEnd=SpeedKmh;TestDistance=FVector::Dist(TestStart,GetActorLocation())/100;ResetCar();SteeringTestStarted=true;}
  if(Elapsed>=12&&!TestFinished) {
   SteerYaw=FMath::Abs(FMath::FindDeltaAngleDegrees(StartPose.Rotator().Yaw,GetActorRotation().Yaw));
   float Distance=TestDistance+FVector::Dist(TestStart,GetActorLocation())/100;
   int32 Contacts=GroundedWheels;ResetCar();TestFinished=true;
   auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("max_speed_kmh"),MaxSpeed);
   J->SetNumberField(TEXT("brake_start_kmh"),BrakeStart);J->SetNumberField(TEXT("brake_end_kmh"),BrakeEnd);
   J->SetNumberField(TEXT("distance_m"),Distance);J->SetNumberField(TEXT("steering_yaw_change_deg"),SteerYaw);
   J->SetNumberField(TEXT("grounded_wheels_before_reset"),Contacts);
   J->SetNumberField(TEXT("reset_position_error_cm"),FVector::Dist(StartPose.GetLocation(),GetActorLocation()));
   J->SetBoolField(TEXT("passed"),MaxSpeed>25&&BrakeStart>25&&BrakeEnd<5&&SteerYaw>2&&Contacts>=3&&FVector::Dist(StartPose.GetLocation(),GetActorLocation())<1);
   FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);
   FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("driving_validation.json")));
  }
 }
 if(TestMode&&Elapsed>(DynamicsTest?33:14)&&!ScreenshotTaken){
  float Peak=0;int32 Samples=0;bool Ok=EngineAudio->SaveCapture(FPaths::ProjectSavedDir()/TEXT("GotlandRingEngine.wav"),Peak,Samples);
  FString AudioReport=FString::Printf(TEXT("{\"passed\":%s,\"samples\":%d,\"sample_rate\":%d,\"peak\":%.8f}"),Ok?TEXT("true"):TEXT("false"),Samples,EngineAudio->State->Rate.load(),Peak);
  FFileHelper::SaveStringToFile(AudioReport,*(FPaths::ProjectSavedDir()/TEXT("audio_validation.json")));
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingDriving.png"),true,false);ScreenshotTaken=true;}
 if(TestMode&&Elapsed>(DynamicsTest?35:16))FGenericPlatformMisc::RequestExit(false);
 FVector Forward=GetActorForwardVector(),Up=GetActorUpVector();
 FVector Velocity=Chassis->GetPhysicsLinearVelocity();float Speed=FVector::DotProduct(Velocity,Forward)/100;
 SpeedKmh=Velocity.Size()*.036f;MaxSpeed=FMath::Max(MaxSpeed,SpeedKmh);
 if(ReverseRequest&&Throttle==0){if(Speed>.5f)Brake=1;else Throttle=-1;}
 SideslipDegrees=FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Velocity,GetActorRightVector())/100,FMath::Max(1.f,FMath::Abs(Speed))));
 GroundedWheels=0;
 float Extensions[4]={},Travel[4]={},LongForces[4]={},LatForces[4]={};
 FVector Anchors[4],Contacts[4],Normals[4];
 for(int32 I=0;I<4;I++) {
  bool Front=I<2;
  // Physical wheelbase/track/radius are the factory setup. Visual hubs retain their source mesh pivots.
  FVector Hub(Front?126:-126,I%2==0?-73:73,30.57f);
  Anchors[I]=GetActorTransform().TransformPosition(Hub+FVector(0,0,10-48));
  FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(RingSuspension),true,this);
  auto& W=Dynamics.Wheels[I];
  W.Grounded=GetWorld()->LineTraceSingleByChannel(Hit,Anchors[I],Anchors[I]-Up*50.57f,ECC_Visibility,Query);
  Travel[I]=W.Grounded?FMath::Max(0.f,Hit.Distance-30.57f):20;
  Extensions[I]=10-Travel[I];
  if(!W.Grounded){W.Load=0;continue;}
  GroundedWheels++;Contacts[I]=Hit.ImpactPoint;Normals[I]=Hit.Normal;
  // Unity's grip classification follows the circuit footprint, not scenery materials.
  FHitResult RoadHit;
  W.Asphalt=Hit.GetActor()->ActorHasTag(TEXT("ValidationAsphalt"))||RoadSurface->LineTraceComponent(RoadHit,Hit.ImpactPoint+FVector(0,0,10000),Hit.ImpactPoint-FVector(0,0,10000),Query);
  FVector PointVelocity=Chassis->GetPhysicsLinearVelocityAtPoint(Anchors[I])/100;
  float Load=1360*9.81f*(Front?.58f:.42f)/2;
  W.Load=FMath::Clamp(Load+(Front?34000:29000)*Extensions[I]/100-(Front?3800:3200)*FVector::DotProduct(PointVelocity,Up),0.f,Load*4);
  Chassis->AddForceAtLocation(Hit.Normal*W.Load*100,Anchors[I]);
 }
 // Engine and wheel inertia use <=2 ms steps; accumulated contact forces act on Chaos.
 int32 Steps=FMath::Max(1,FMath::CeilToInt(Dt/.002f));float H=Dt/Steps;
 for(int32 Step=0;Step<Steps;Step++) {
  Dynamics.Step(H,Throttle,Brake,Steering,Speed,Velocity.Size()/100,Handbrake);
  for(int32 I=0;I<4;I++) {
   auto& W=Dynamics.Wheels[I];
   if(W.Grounded){
    FVector TireForward=GetActorTransform().TransformVectorNoScale(FRotator(0,I<2?Dynamics.SteeringDegrees:0,0).Vector());
    TireForward=FVector::VectorPlaneProject(TireForward,Normals[I]).GetSafeNormal();
    FVector TireRight=FVector::CrossProduct(Normals[I],TireForward).GetSafeNormal();
    FVector PV=Chassis->GetPhysicsLinearVelocityAtPoint(Contacts[I])/100;
    W.LongVelocity=FVector::DotProduct(PV,TireForward);W.LatVelocity=FVector::DotProduct(PV,TireRight);
   }
   Dynamics.SolveWheel(I,H);LongForces[I]+=W.LongForce/Steps;LatForces[I]+=W.LatForce/Steps;
  }
 }
 for(int32 I=0;I<4;I++)if(Dynamics.Wheels[I].Grounded){
  FVector TireForward=GetActorTransform().TransformVectorNoScale(FRotator(0,I<2?Dynamics.SteeringDegrees:0,0).Vector());
  TireForward=FVector::VectorPlaneProject(TireForward,Normals[I]).GetSafeNormal();
  FVector TireRight=FVector::CrossProduct(Normals[I],TireForward).GetSafeNormal();
  float StopLateral=FMath::Abs(Dynamics.Wheels[I].LatVelocity)*1360*.25f/Dt;
  LatForces[I]=FMath::Clamp(LatForces[I],-StopLateral,StopLateral);
  float Height=Throttle==0?31.5f:15.f;
  Chassis->AddForceAtLocation((TireForward*LongForces[I]+TireRight*LatForces[I])*100,Contacts[I]+Up*Height);
 }
 for(int32 Axle=0;Axle<2;Axle++){
  int32 L=Axle*2,R=L+1;float Force=(Travel[L]-Travel[R])/100*(Axle==0?4500:3200)*100;
  if(Dynamics.Wheels[L].Grounded)Chassis->AddForceAtLocation(-Up*Force,Anchors[L]);
  if(Dynamics.Wheels[R].Grounded)Chassis->AddForceAtLocation(Up*Force,Anchors[R]);
 }
 Chassis->AddForce(-Velocity*(Velocity.Size()/100)*.42f);
 if(GroundedWheels)Chassis->AddForce(-Forward*FMath::Clamp(Speed,-1.f,1.f)*.012f*1360*981);
 EngineAudio->SetEngine(Dynamics.Motor.Rpm(),FMath::Abs(Dynamics.Throttle),Velocity.Size()/100,Muted);
 for(int32 I=0;I<Visuals.Num();I++)if(VisualWheels[I]>=0) {
  int32 W=VisualWheels[I];Visuals[I]->SetRelativeLocation(WheelCenters[W]-FVector(0,0,48-Extensions[W]));
  Visuals[I]->SetRelativeRotation(FRotator(0,W<2?Dynamics.SteeringDegrees:0,0).Quaternion()*FRotator(-FMath::RadiansToDegrees(Dynamics.Wheels[W].Rotation),0,0).Quaternion());
 }

}
void ARingHUD::DrawHUD() {
 Super::DrawHUD();auto* Car=Cast<ARingCar>(GetOwningPawn());if(!Car)return;
 DrawRect(FLinearColor(0,0,0,.65f),20,20,1100,80);
 DrawText(FString::Printf(TEXT("GOTLANDRING | %.0f km/h | %d RPM | gear %d | contacts %d/4 | slip %.1f deg"),Car->SpeedKmh,int32(Car->Dynamics.Motor.Rpm()),Car->Dynamics.Throttle<0?-1:Car->Dynamics.Gear,Car->GroundedWheels,Car->SideslipDegrees),FLinearColor::White,30,30,nullptr,1.5f);
 DrawText(TEXT("W accelerate   S brake   X reverse   A D steer   SPACE handbrake   R reset   M mute"),FLinearColor::White,30,65,nullptr,1.2f);
}
ARingGameMode::ARingGameMode(){DefaultPawnClass=nullptr;HUDClass=ARingHUD::StaticClass();}
