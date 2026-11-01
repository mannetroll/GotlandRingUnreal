#include "RingCar.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
void ARingCar::RunCompleteMigrationTest(){
 if(Elapsed<2)return;
 if(CompletePhase==0){
  CompleteReport=MakeShared<FJsonObject>();SetDrivePaused(true);
  bool ModelsOK=Models.Num()==2,PaintOK=true,Cameras=true;
  const FTransform Pose=GetActorTransform();const auto Lap=Laps;
  const FVector V(1200,300,0);PauseVelocity=V;Autopilot=true;
  for(int Model=0;Model<2;Model++){
   const int Paint=PaintSelections[SelectedCar];
   for(int C=0;C<Models[SelectedCar].PaintColors.Num();C++){CyclePaint();FLinearColor Actual;PaintOK&=PaintMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BodyPaint")),Actual)&&Actual.Equals(Models[SelectedCar].PaintColors[PaintSelections[SelectedCar]],.001f);}
   PaintOK&=PaintSelections[SelectedCar]==Paint;
   for(int View=0;View<4;View++){ViewMode=View;UpdateView(1.f/60,nullptr);for(int I=0;I<Visuals.Num();I++)if(VisualWheels[I]==-2)Cameras&=Visuals[I]->IsVisible()==(View!=0);}
   SwitchCarModel();ModelsOK&=GetActorTransform().Equals(Pose,.01)&&PauseVelocity.Equals(V,.01)&&Laps.Lap==Lap.Lap&&Laps.Seconds==Lap.Seconds&&Autopilot&&DrivePaused;
  }
  auto* Window=UGameUserSettings::GetGameUserSettings();const auto Mode=Window->GetFullscreenMode();ToggleFullscreen();bool Fullscreen=Window->GetFullscreenMode()!=Mode;ToggleFullscreen();Fullscreen&=Window->GetFullscreenMode()==Mode;
  CompleteReport->SetBoolField(TEXT("two_models_restore_motion_lap_pilot_and_pause"),ModelsOK);CompleteReport->SetBoolField(TEXT("all_paints_and_per_model_selection"),PaintOK);CompleteReport->SetBoolField(TEXT("both_models_four_cameras"),Cameras);CompleteReport->SetBoolField(TEXT("fullscreen_roundtrip"),Fullscreen);CompletePassed&=ModelsOK&&PaintOK&&Cameras&&Fullscreen;
  Autopilot=false;ViewMode=2;LookYaw=65;LookPitch=-5;UpdateView(1.f/60,nullptr);CompletePhase++;return;
 }
 if(CompletePhase==1&&Elapsed>3){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RallyPaintRed.png"),true,false);CompletePhase++;return;}
 if(CompletePhase==2&&Elapsed>4){CyclePaint();FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RallyPaintBlue.png"),true,false);CompletePhase++;return;}
 if(CompletePhase==3&&Elapsed>5){CyclePaint();FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RallyPaintWhite.png"),true,false);CompletePhase++;return;}
 if(CompletePhase==4&&Elapsed>6){SwitchCarModel();FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SubaruBlueGold.png"),true,false);CompletePhase++;return;}
 if(CompletePhase==5&&Elapsed>7){
  Laps.Best=123.4f;ToggleTrainingArea();
  bool Surface=Training&&Pilot->Count==128&&OnDrivingRoad(FVector(39999,600000,0))&&!OnDrivingRoad(FVector(40001,600000,0));
  FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(TrainingTest),true,this);Surface&=RoadSurface->LineTraceComponent(H,FVector(0,600000,100),FVector(0,600000,-100),Q);
  Settings.AwdMode=false;Chassis->SetSimulatePhysics(false);ArcadeVelocity=FVector::ZeroVector;for(int I=0;I<360;I++)StepArcade(1.f/60,1,0,0,false);bool Arcade=SpeedKmh>25;for(int I=0;I<180;I++)StepArcade(1.f/60,0,1,0,false);Arcade&=SpeedKmh<1;
  Settings.AwdMode=true;RecoverTrainingCar();SetDrivePaused(false);Autopilot=true;CompleteReport->SetBoolField(TEXT("training_pad_surface_and_route"),Surface);CompleteReport->SetBoolField(TEXT("training_arcade_acceleration_braking"),Arcade);CompletePassed&=Surface&&Arcade;CompletePhase++;return;
 }
 if(CompletePhase==6&&Elapsed>15){
  bool Awd=Training&&GroundedWheels>=3&&SpeedKmh>10&&OnDrivingRoad(GetActorLocation());CompleteReport->SetNumberField(TEXT("training_awd_speed_kmh"),SpeedKmh);CompleteReport->SetBoolField(TEXT("training_awd_autopilot_contact"),Awd);CompletePassed&=Awd;
  SetDrivePaused(true);const auto Pose=GetActorTransform();ToggleReferenceLap();TickReference(0);ToggleReferenceLap();bool Restore=Training&&DrivePaused&&GetActorTransform().Equals(Pose,.01);for(int I=0;I<16;I++)NextSceneryView();Restore&=Training&&DrivePaused&&GetActorTransform().Equals(Pose,.01);CompleteReport->SetBoolField(TEXT("training_replay_scenery_restore"),Restore);CompletePassed&=Restore;
  SaveDesktopScreenshot();CompletePhase++;return;
 }
 if(CompletePhase==7&&Elapsed>17){
  FString Desktop;FFileHelper::LoadFileToString(Desktop,*(FPaths::ProjectSavedDir()/TEXT("desktop_screenshot.txt")));bool Capture=FPaths::FileExists(Desktop);CompleteReport->SetStringField(TEXT("desktop_png"),Desktop);CompleteReport->SetBoolField(TEXT("desktop_screenshot_saved"),Capture);CompletePassed&=Capture;
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("TrainingArea.png"),true,false);CompletePhase++;return;
 }
 if(CompletePhase==8&&Elapsed>18){
  ToggleTrainingArea();bool Return=!Training&&Pilot->Count>2000&&RoadSurface==CircuitRoad;CompleteReport->SetBoolField(TEXT("return_to_circuit"),Return);const bool Best=FMath::IsNearlyEqual(Laps.Best,123.4f);CompleteReport->SetBoolField(TEXT("training_retains_circuit_best"),Best);CompletePassed&=Return&&Best;
  CompleteReport->SetBoolField(TEXT("passed"),CompletePassed);FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(CompleteReport.ToSharedRef(),Writer);FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("complete_validation.json")));FGenericPlatformMisc::RequestExit(false);
 }
}
