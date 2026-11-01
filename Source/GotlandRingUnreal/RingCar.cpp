#include "RingCar.h"
#include "RingAudio.h"
#include "RingRouteData.h"
#include "Materials/MaterialInstanceDynamic.h"
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
 Chassis->SetSimulatePhysics(true);
 Chassis->SetLinearDamping(0); Chassis->SetAngularDamping(.12f);
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
 Super::BeginPlay(); Chassis->SetMassOverrideInKg(NAME_None,1360,true);Chassis->SetCenterOfMass(FVector((.58f-.5f)*252,0,0));Pilot=MakeUnique<Ring::Pilot>(Ring::RouteData,UE_ARRAY_COUNT(Ring::RouteData)); StartPose=GetActorTransform(); TestStart=GetActorLocation();
 PilotTest=FParse::Param(FCommandLine::Get(),TEXT("RingPilotTest"));Autopilot=PilotTest;
 DynamicsTest=FParse::Param(FCommandLine::Get(),TEXT("RingDynamicsTest"));
 TestMode=DynamicsTest||FParse::Param(FCommandLine::Get(),TEXT("RingDrivingTest"));
 InstrumentsTest=FParse::Param(FCommandLine::Get(),TEXT("RingInstrumentsTest"));
 SettingsTest=FParse::Param(FCommandLine::Get(),TEXT("RingSettingsTest"));
 Settings=(TestMode||PilotTest||SettingsTest||InstrumentsTest)?Ring::DrivingSettings():LoadDrivingSettings();ApplyDrivingSettings();if(!Settings.AwdMode)Chassis->SetSimulatePhysics(false);
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It){
  auto* Mesh=It->GetStaticMeshComponent();
  if(Mesh->GetStaticMesh()&&Mesh->GetStaticMesh()->GetName()==TEXT("Road"))RoadSurface=Mesh;
 }
 checkf(RoadSurface.IsValid(),TEXT("The driving level must contain the migrated Road mesh"));
 if(TestMode){TyreSquealEnabled=true;EngineAudio->CaptureTestAudio();}
 EngineAudio->Start();
 for(int32 I=0;I<VisualMeshes.Num();I++) {
  auto* V=NewObject<UStaticMeshComponent>(this); V->SetupAttachment(Chassis);
  V->SetStaticMesh(VisualMeshes[I]);
  float InstrumentTrip;
  if(V->GetMaterial(0)&&V->GetMaterial(0)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("TripKm")),InstrumentTrip))Dashboard=V->CreateDynamicMaterialInstance(0);
  V->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  V->RegisterComponent();AddInstanceComponent(V);Visuals.Add(V);
  V->SetRelativeLocation((VisualWheels[I]==-3?FVector(28.58618,-38.799202,83.9761047):(VisualWheels[I]<0?FVector::ZeroVector:WheelCenters[VisualWheels[I]]))-FVector(0,0,48));
 }
 InitializeDriver();
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
 PresentationTest=FParse::Param(FCommandLine::Get(),TEXT("RingPresentationTest"));
 SignsTest=FParse::Param(FCommandLine::Get(),TEXT("RingSignsTest"));WindTest=FParse::Param(FCommandLine::Get(),TEXT("RingWindTest"));InitializeSigns();InitializeWindRotors();if(!PresentationTest)LoadKeys();
 MigrationTest=FParse::Param(FCommandLine::Get(),TEXT("RingMigrationTest"));
 if(DynamicsTest)SetupDynamicsTest();
 if(auto* PC=Cast<APlayerController>(GetController())) {PC->SetViewTarget(this);PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;}
}
void ARingCar::ResetCar() {
 ArcadeVelocity=FVector::ZeroVector;ArcadeSteer=ArcadeBoost=0;ArcadeRpm=900;ArcadeReverse=false;
 Laps.Restart();PauseVelocity=PauseAngular=FVector::ZeroVector;
 Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
 SetActorTransform(StartPose,false,nullptr,ETeleportType::TeleportPhysics);Chassis->WakeAllRigidBodies();Dynamics=Ring::Dynamics();ApplyDrivingSettings();Pilot->Reset();PreviousVelocity=ViewAcceleration=ViewLean=ViewTilt=FVector::ZeroVector;
}
void ARingCar::Tick(float Dt) {
 Super::Tick(Dt);Elapsed+=Dt;if(MigrationTest){RunMigrationTest(Dt);return;}if(WindTest){RunWindTest(Dt);return;}if(!ReferenceActive)AnimateWindRotors(Dt);
 if(SignsTest){RunSignsTest();return;}
 if(InstrumentsTest){RunInstrumentsTest();return;}
 if(SettingsTest)RunSettingsTest();
 if(PresentationTest){RunPresentationTest();return;}
 auto* PC=Cast<APlayerController>(GetController());
 if(!SettingsOpen&&(!MenuWidget.IsValid()||MenuPage==0)&&Pressed(EKeys::O)){
  if(SceneryView>=0)ExitSceneryViews();ToggleReferenceLap();
 }
 if(ReferenceActive){
  if(Pressed(EKeys::Escape)||Pressed(EKeys::SpaceBar))ReferencePaused=!ReferencePaused;
  if(Pressed(EKeys::Home)){SeekReference(0);ReferencePaused=false;}
  if(Pressed(EKeys::M))Muted=!Muted;
  TickReference(Dt);
  if(Pressed(EKeys::F2))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ReferenceLap.png"),true,false);
  return;
 }
 if(!SettingsOpen&&(!MenuWidget.IsValid()||MenuPage==0)&&Pressed(EKeys::V))NextSceneryView();
 if(SceneryView>=0){
  if(Pressed(EKeys::C)||Pressed(EKeys::Escape)){ExitSceneryViews();return;}
  else {if(Pressed(EKeys::F2))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Scenery%02d.png"),SceneryView+1),true,false);EngineAudio->SetEngine(900,0,0,0,true);return;}
 }
 if(PC&&Pressed(EKeys::Escape)){MenuBack();}
 if(Pressed(EKeys::F1)||Pressed(EKeys::F3)){if(SettingsOpen)CloseSettings(false);else if(!MenuWidget.IsValid())OpenSettings();}
 if(Pressed(EKeys::F))ToggleFullscreen();
 if(MenuWidget.IsValid()){EngineAudio->SetEngine(Dynamics.Motor.Rpm(),0,0,0,true);return;}
 if(SettingsOpen){EngineAudio->SetEngine(Dynamics.Motor.Rpm(),0,0,0,true);return;}
 float Throttle=0, Brake=0, Steering=0;bool Handbrake=false,ReverseRequest=false;
 if(PC) {
  Throttle=(Held(EKeys::W)||Held(EKeys::Up))?1.f:0.f;
  ReverseRequest=Held(EKeys::X);
  Brake=(Held(EKeys::S)||Held(EKeys::Down))?1.f:0.f;
  Steering=((Held(EKeys::D)||Held(EKeys::Right))?1.f:0.f)-((Held(EKeys::A)||Held(EKeys::Left))?1.f:0.f);
  Handbrake=Held(EKeys::SpaceBar);
  if(Pressed(EKeys::H))ToggleHandling();
  if(Pressed(EKeys::R))RecoverCar();
  if(Pressed(EKeys::Home))ResetCar();
  if(Pressed(EKeys::P)){Autopilot=!Autopilot;Pilot->Reset();UE_LOG(LogTemp,Display,TEXT("Ring autopilot %s via P"),Autopilot?TEXT("ON"):TEXT("OFF"));}
  if(Pressed(EKeys::C)){ViewMode=(ViewMode+1)%4;LookYaw=LookPitch=0;}
  if(Pressed(EKeys::U)){MouseControl=!MouseControl;MouseSteering=0;LookYaw=LookPitch=0;}
  if(Pressed(EKeys::F2))FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingCockpit.png"),true,false);
  if(Pressed(EKeys::J))HideHUD=!HideHUD;
  if(Pressed(EKeys::M))Muted=!Muted;
  if(Pressed(EKeys::T)){TyreSquealEnabled=!TyreSquealEnabled;Settings.TyreSqueal=TyreSquealEnabled;SaveDrivingSettings(Settings);}
 }
 UpdateView(Dt,PC);
 if(DrivePaused){EngineAudio->SetEngine(Dynamics.Motor.Rpm(),0,0,0,true);return;}
 UpdateLap(Dt);
 if(MouseControl)Steering=MouseSteering;
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
  FString AudioReport=FString::Printf(TEXT("{\"passed\":%s,\"samples\":%d,\"sample_rate\":%d,\"peak\":%.8f,\"peak_tyre_squeal\":%.8f}"),(Ok&&PeakTyreSqueal>0)?TEXT("true"):TEXT("false"),Samples,EngineAudio->State->Rate.load(),Peak,PeakTyreSqueal);
  FFileHelper::SaveStringToFile(AudioReport,*(FPaths::ProjectSavedDir()/TEXT("audio_validation.json")));
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingDriving.png"),true,false);ScreenshotTaken=true;}
 if(TestMode&&Elapsed>(DynamicsTest?35:16))FGenericPlatformMisc::RequestExit(false);
 FVector Forward=GetActorForwardVector(),Up=GetActorUpVector();
 FVector Velocity=Settings.AwdMode?Chassis->GetPhysicsLinearVelocity():ArcadeVelocity*100;float Speed=FVector::DotProduct(Velocity,Forward)/100;
 SpeedKmh=Velocity.Size()*.036f;MaxSpeed=FMath::Max(MaxSpeed,SpeedKmh);
 if(ReverseRequest&&Throttle==0){if(Speed>.5f)Brake=1;else Throttle=-1;}
 SideslipDegrees=FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Velocity,GetActorRightVector())/100,FMath::Max(1.f,FMath::Abs(Speed))));
 if(Autopilot){
  FVector P=GetActorLocation()/100;float Fraction,Offset;bool OnRoad;
  int Segment=Pilot->Nearest({float(P.X),float(P.Y)},Fraction,Offset,OnRoad);
  auto C=Pilot->Drive({float(P.X),float(P.Y)},GetActorRotation().Yaw,{float(Velocity.X/100),float(Velocity.Y/100)},Segment,OnRoad,Dt,Velocity.Size()/100);
  if(Settings.AwdMode)Pilot->Correct(C,Speed,SideslipDegrees,FVector::DotProduct(Chassis->GetPhysicsAngularVelocityInRadians(),Up),Velocity.Size()/100,Dt);
  Throttle=C.Throttle;Brake=C.Brake;Steering=C.Steering;Handbrake=false;PilotTarget=C.Target*3.6f;PilotError=C.Error;
 }
 if(!Settings.AwdMode){StepArcade(Dt,Throttle,Brake,Steering,ReverseRequest);return;}
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
 const FVector Surface=SurfaceContacts()*Settings.SurfaceSound;EngineAudio->SetSurface(Surface.X,Surface.Y,Surface.Z);
 float Squeal=TyreSquealEnabled?Dynamics.TyreSqueal():0;PeakTyreSqueal=FMath::Max(PeakTyreSqueal,Squeal);
 EngineAudio->SetEngine(Dynamics.Motor.Rpm(),FMath::Abs(Dynamics.Throttle),Velocity.Size()/100,Squeal,Muted);
 TripKm+=Velocity.Size()/100*Dt/1000;
 UpdateInstruments(Dynamics.Motor.Rpm(),Velocity.Size()/100,Dynamics.Throttle<0?-1:Dynamics.Gear);
 AnimateCockpitSteering(Dynamics.SteeringDegrees);
 for(int32 I=0;I<Visuals.Num();I++)if(VisualWheels[I]>=0) {
  int32 W=VisualWheels[I];Visuals[I]->SetRelativeLocation(WheelCenters[W]-FVector(0,0,48-Extensions[W]));
  Visuals[I]->SetRelativeRotation(FRotator(0,W<2?Dynamics.SteeringDegrees:0,0).Quaternion()*FRotator(-FMath::RadiansToDegrees(Dynamics.Wheels[W].Rotation),0,0).Quaternion());
 }

 if(TestMode&&!DynamicsTest){
  DynamicsTelemetry+=FString::Printf(TEXT("%.4f,%.5f,%.3f,%.3f,%.3f,%d,%.1f,%d,%d,%d,%d,%.3f,%.3f,%.3f\n"),Elapsed,Dt,SpeedKmh,Throttle,Brake,GroundedWheels,Dynamics.Motor.Rpm(),Dynamics.Wheels[0].Asphalt,Dynamics.Wheels[1].Asphalt,Dynamics.Wheels[2].Asphalt,Dynamics.Wheels[3].Asphalt,GetActorLocation().X,GetActorLocation().Y,GetActorLocation().Z);
  if(Elapsed>14&&Elapsed-Dt<=14)FFileHelper::SaveStringToFile(DynamicsTelemetry,*(FPaths::ProjectSavedDir()/TEXT("driving_telemetry.csv")));
 }
 if(PilotTest)RunPilotTest(Dt);
}
ARingGameMode::ARingGameMode(){DefaultPawnClass=nullptr;HUDClass=ARingHUD::StaticClass();}

void ARingCar::AnimateCockpitSteering(float RoadDegrees){
 AnimateDriver(RoadDegrees);
 for(int32 I=0;I<Visuals.Num();I++)if(VisualWheels[I]==-3)
  Visuals[I]->SetRelativeRotation(FQuat(FVector(.982749939,0,-.184939444),FMath::DegreesToRadians(-RoadDegrees*12)));
}
