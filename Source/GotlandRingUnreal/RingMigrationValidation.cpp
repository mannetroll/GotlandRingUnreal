#include "RingCar.h"
#include "RingReferenceData.h"
#include "RingAudio.h"
#include "ProceduralMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "UnrealClient.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
void ARingCar::RunMigrationTest(float Dt){
 if(Elapsed<2)return;
 if(MigrationPhase==0){
  auto J=MakeShared<FJsonObject>();bool Samples=UE_ARRAY_COUNT(Ring::ReferenceFrames)==4464;float Previous=-BIG_NUMBER;
  int OffRoad=0;for(const auto& F:Ring::ReferenceFrames){Samples&=F.Distance>Previous&&F.Speed>0&&F.Rpm>=3800&&F.Rpm<=6800&&F.Gear>=1&&F.Gear<=5;Previous=F.Distance;FVector P=ReferencePoint(F.Distance);FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(ReferenceValidation),true,this);if(!RoadSurface->LineTraceComponent(H,P+FVector(0,0,3000),P-FVector(0,0,3000),Q))OffRoad++;}
  bool SkyCollisionFree=false;
  for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("RingPhotographicSky")))SkyCollisionFree=!It->GetActorEnableCollision()&&It->GetStaticMeshComponent()->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
  J->SetBoolField(TEXT("sky_has_no_collision"),SkyCollisionFree);
  bool Restore=true,Pause=true,End=true,Hands=true;
  const auto OriginalDynamics=Dynamics;const auto OriginalLap=Laps;
  for(bool Awd:{true,false})for(bool Paused:{false,true})for(bool PilotOn:{false,true}){
   if(Settings.AwdMode!=Awd)ToggleHandling();SetDrivePaused(false);Autopilot=PilotOn;
   ArcadeVelocity=FVector(100,20,0);if(Awd){Chassis->SetPhysicsLinearVelocity(FVector(1000,200,0));Chassis->SetPhysicsAngularVelocityInRadians(FVector(0,0,.12));}
   SetDrivePaused(Paused);FVector Velocity=Paused?PauseVelocity:Chassis->GetPhysicsLinearVelocity(),Angular=Paused?PauseAngular:Chassis->GetPhysicsAngularVelocityInRadians();const FTransform Pose=GetActorTransform();
   ToggleReferenceLap();SeekReference(90);TickReference(0);ReferencePaused=true;TickReference(3);Pause&=ReferenceSeconds==90&&EngineAudio->State->Muted.load();
   ReferencePaused=false;TickReference(.5f);Pause&=FMath::IsNearlyEqual(ReferenceSeconds,90.5f,.001f);
   SeekReference(186);TickReference(1);End&=FMath::IsNearlyEqual(ReferenceClock,175.88f,.002f)&&EngineAudio->State->Muted.load();
   ToggleReferenceLap();Restore&=GetActorTransform().Equals(Pose,.01)&&DrivePaused==Paused&&Autopilot==PilotOn&&Settings.AwdMode==Awd&&Laps.Seconds==OriginalLap.Seconds&&Dynamics.Gear==OriginalDynamics.Gear;
   if(Awd&&!Paused)Restore&=Chassis->GetPhysicsLinearVelocity().Equals(Velocity,.01)&&Chassis->GetPhysicsAngularVelocityInRadians().Equals(Angular,.001);
  }
  AnimateDriver(0);TArray<FVector> Rest;for(auto& V:DriverBody->GetProcMeshSection(0)->ProcVertexBuffer)Rest.Add(V.Position);
  AnimateDriver(20);float Motion=0;int K=0;for(auto& V:DriverBody->GetProcMeshSection(0)->ProcVertexBuffer){Motion=FMath::Max(Motion,float(FVector::Dist(Rest[K++],V.Position)));Hands&=!V.Position.ContainsNaN();}
  AnimateDriver(-20);Hands&=Motion>5;
  MakeSceneryViews();SetDrivePaused(false);Settings.AwdMode=true;Chassis->SetSimulatePhysics(true);Chassis->SetPhysicsLinearVelocity(FVector(1000,0,0));
  bool Views=true;FTransform Pose=GetActorTransform();for(int I=0;I<15;I++){NextSceneryView();Views&=SceneryView==I&&DrivePaused&&Camera->GetComponentLocation().Equals(SceneryViews[I].Position,.01);}
  NextSceneryView();Views&=SceneryView==-1&&!DrivePaused&&GetActorTransform().Equals(Pose,.01)&&Chassis->GetPhysicsLinearVelocity().Equals(FVector(1000,0,0),.01);
  bool Arcade=true;SetDrivePaused(true);Settings.AwdMode=true;ToggleHandling();SetDrivePaused(false);ArcadeVelocity=FVector::ZeroVector;ResetCar();
  float Fraction,Offset;bool OnRoad;FVector Start=GetActorLocation()/100;Pilot->Nearest({float(Start.X),float(Start.Y)},Fraction,Offset,OnRoad);J->SetBoolField(TEXT("arcade_start_on_road"),OnRoad);
  for(int I=0;I<360;I++)StepArcade(1.f/60,1,0,0,false);float Peak=SpeedKmh;for(int I=0;I<180;I++)StepArcade(1.f/60,0,1,0,false);J->SetNumberField(TEXT("arcade_peak_kmh"),Peak);J->SetNumberField(TEXT("arcade_braking_kmh"),SpeedKmh);Arcade&=Peak>25&&SpeedKmh<1&&!Chassis->IsSimulatingPhysics();ToggleHandling();Arcade&=Settings.AwdMode&&Chassis->IsSimulatingPhysics();
  OpenMenu();ToggleReferenceLap();TickReference(0);ToggleReferenceLap();bool MenuModes=DrivePaused&&MenuWidget.IsValid();
  NextSceneryView();ExitSceneryViews();MenuModes&=DrivePaused&&MenuWidget.IsValid();CloseMenu();MenuModes&=!DrivePaused;
  J->SetBoolField(TEXT("menu_modes_restore_pause"),MenuModes);
  ResetCar();Autopilot=false;Settings.AwdMode=true;SetDrivePaused(false);ToggleReferenceLap();SeekReference(90);ReferencePaused=true;TickReference(0);
  J->SetNumberField(TEXT("reference_frames"),UE_ARRAY_COUNT(Ring::ReferenceFrames));J->SetNumberField(TEXT("off_road_reference_frames"),OffRoad);J->SetNumberField(TEXT("driver_vertices"),Rest.Num());J->SetNumberField(TEXT("hand_motion_cm"),Motion);
  J->SetBoolField(TEXT("eight_drive_restoration_cases"),Restore);J->SetBoolField(TEXT("pause_and_resume"),Pause);J->SetBoolField(TEXT("completion_clock_and_audio"),End);J->SetBoolField(TEXT("animated_driver"),Hands);J->SetBoolField(TEXT("fifteen_views_restore_drive"),Views);J->SetBoolField(TEXT("arcade_acceleration_braking_and_switch"),Arcade);
  J->SetBoolField(TEXT("passed"),Samples&&OffRoad==0&&Restore&&Pause&&End&&Hands&&Views&&Arcade&&SkyCollisionFree&&MenuModes);
  FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("migration_validation.json")));MigrationPhase++;
 }else if(MigrationPhase==1&&Elapsed>4){TickReference(0);FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ReferenceLap.png"),true,false);MigrationPhase++;}
 else if(MigrationPhase==2&&Elapsed>5){ToggleReferenceLap();SetDrivePaused(true);ViewMode=0;for(int I=0;I<120;I++)UpdateView(1.f/60,nullptr);AnimateCockpitSteering(3);MigrationPhase++;}
 else if(MigrationPhase==3&&Elapsed>6){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("DriverHands.png"),true,false);MigrationPhase++;}
 else if(MigrationPhase==4&&Elapsed>7){NextSceneryView();MigrationPhase++;}
 else if(MigrationPhase==5&&Elapsed>8){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SceneryView01.png"),true,false);MigrationPhase++;}
 else if(MigrationPhase==6&&Elapsed>9){FGenericPlatformMisc::RequestExit(false);}
}
