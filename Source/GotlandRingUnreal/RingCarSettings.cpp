#include "RingCar.h"
#include "RingAudio.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"
namespace {
struct FSettingField {const TCHAR* Key;const TCHAR* Label;float Ring::DrivingSettings::*Member;float Min,Max;};
const FSettingField Fields[]={
 {TEXT("Grip"),TEXT("Tyre grip multiplier"),&Ring::DrivingSettings::Grip,.7f,1.3f},
 {TEXT("FrontTorque"),TEXT("Front torque share"),&Ring::DrivingSettings::FrontTorque,.1f,.9f},
 {TEXT("Steering"),TEXT("Low-speed steering (degrees)"),&Ring::DrivingSettings::Steering,25,55},
 {TEXT("HighSpeedSteering"),TEXT("High-speed steering (degrees)"),&Ring::DrivingSettings::HighSpeedSteering,8,25},
 {TEXT("SteeringRate"),TEXT("Steering speed (degrees/s)"),&Ring::DrivingSettings::SteeringRate,40,150},
 {TEXT("DroneHeight"),TEXT("Drone height (m)"),&Ring::DrivingSettings::DroneHeight,5,60},
 {TEXT("DroneDistance"),TEXT("Drone following distance (m)"),&Ring::DrivingSettings::DroneDistance,5,120},
 {TEXT("CockpitMovement"),TEXT("Cockpit movement"),&Ring::DrivingSettings::CockpitMovement,0,1}
};
}
FString ARingCar::DrivingSettingsPath()const{return FPaths::ProjectSavedDir()/(SettingsTest?TEXT("DrivingSettingsTest.json"):TEXT("DrivingSettings.json"));}
Ring::DrivingSettings ARingCar::LoadDrivingSettings()const{
 Ring::DrivingSettings Result;FString Text;
 if(!FFileHelper::LoadFileToString(Text,*DrivingSettingsPath()))return Result;
 TSharedPtr<FJsonObject> Json;auto Reader=TJsonReaderFactory<>::Create(Text);
 if(!FJsonSerializer::Deserialize(Reader,Json)||!Json.IsValid()){UE_LOG(LogTemp,Warning,TEXT("Cannot read driving settings JSON"));return Result;}
 for(const auto& Field:Fields){double Value;if(Json->TryGetNumberField(Field.Key,Value)&&FMath::IsFinite(Value))Result.*(Field.Member)=float(Value);}
 Json->TryGetBoolField(TEXT("TractionControl"),Result.TractionControl);Json->TryGetBoolField(TEXT("TyreSqueal"),Result.TyreSqueal);
 Result.ClampValues();return Result;
}
bool ARingCar::SaveDrivingSettings(const Ring::DrivingSettings& Values)const{
 auto Json=MakeShared<FJsonObject>();for(const auto& Field:Fields)Json->SetNumberField(Field.Key,Values.*(Field.Member));
 Json->SetBoolField(TEXT("TractionControl"),Values.TractionControl);Json->SetBoolField(TEXT("TyreSqueal"),Values.TyreSqueal);
 FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Json,Writer);
 bool Saved=FFileHelper::SaveStringToFile(Text,*DrivingSettingsPath());
 if(!Saved)UE_LOG(LogTemp,Error,TEXT("Could not save driving settings to %s"),*DrivingSettingsPath());
 return Saved;
}
void ARingCar::ApplyDrivingSettings(){Settings.Apply(Dynamics);TyreSquealEnabled=TestMode?true:Settings.TyreSqueal;}
void ARingCar::SetDrivePaused(bool Paused){
 if(DrivePaused==Paused)return;DrivePaused=Paused;
 if(Paused){PauseVelocity=Chassis->GetPhysicsLinearVelocity();PauseAngular=Chassis->GetPhysicsAngularVelocityInRadians();Chassis->SetSimulatePhysics(false);}
 else{Chassis->SetSimulatePhysics(true);Chassis->SetPhysicsLinearVelocity(PauseVelocity);Chassis->SetPhysicsAngularVelocityInRadians(PauseAngular);}
}
void ARingCar::OpenSettings(){
 SettingsWasPaused=DrivePaused;SetDrivePaused(true);SettingsOpen=true;DraftSettings=Settings;SettingsStatus.Empty();
 auto Panel=SNew(SVerticalBox);
 Panel->AddSlot().AutoHeight().Padding(0,0,0,10)[SNew(STextBlock).Text(FText::FromString(TEXT("DRIVING & CAMERA SETTINGS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",24))];
 Panel->AddSlot().AutoHeight().Padding(0,0,0,20)[SNew(STextBlock).Text(FText::FromString(TEXT("Driving is paused. Apply saves your setup for the next launch.")))];
 for(const auto Field:Fields){
  Panel->AddSlot().AutoHeight().Padding(0,6)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Field.Label))]
   +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(160)[SNew(SSpinBox<float>)
    .MinValue(Field.Min).MaxValue(Field.Max).MinSliderValue(Field.Min).MaxSliderValue(Field.Max)
    .Value_Lambda([this,Field](){return DraftSettings.*(Field.Member);})
    .OnValueChanged_Lambda([this,Field](float Value){DraftSettings.*(Field.Member)=Value;})]]];
 }
 auto Toggle=[this,&Panel](const TCHAR* Label,bool Ring::DrivingSettings::*Member){
  Panel->AddSlot().AutoHeight().Padding(0,10)[SNew(SCheckBox)
   .IsChecked_Lambda([this,Member](){return DraftSettings.*Member?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
   .OnCheckStateChanged_Lambda([this,Member](ECheckBoxState State){DraftSettings.*Member=State==ECheckBoxState::Checked;})
   [SNew(STextBlock).Text(FText::FromString(Label))]];
 };
 Toggle(TEXT("Traction control assistance"),&Ring::DrivingSettings::TractionControl);
 Toggle(TEXT("Tyre squeal"),&Ring::DrivingSettings::TyreSqueal);
 Panel->AddSlot().AutoHeight().Padding(0,10)[SNew(STextBlock).Text(FText::FromString(TEXT("Five-speed AWD. ABS is enabled. Tyres, suspension and clutch are approximate."))).AutoWrapText(true)];
 Panel->AddSlot().AutoHeight()[SNew(STextBlock).ColorAndOpacity(FLinearColor(1,.4f,.3f)).Text_Lambda([this](){return FText::FromString(SettingsStatus);})];
 Panel->AddSlot().AutoHeight().Padding(0,18,0,0)[SNew(SHorizontalBox)
  +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[SNew(SButton).Text(FText::FromString(TEXT("Restore defaults"))).OnClicked_Lambda([this](){DraftSettings=Ring::DrivingSettings();return FReply::Handled();})]
  +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[SNew(SButton).Text(FText::FromString(TEXT("Cancel"))).OnClicked_Lambda([this](){CloseSettings(false);return FReply::Handled();})]
  +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(FText::FromString(TEXT("Apply & close"))).OnClicked_Lambda([this](){CloseSettings(true);return FReply::Handled();})]];
 SettingsWidget=SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
  [SNew(SBox).WidthOverride(700)[SNew(SBorder).Padding(28).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(.025f,.035f,.05f,.98f))[Panel]]];
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(SettingsWidget.ToSharedRef(),100);
 auto* PC=Cast<APlayerController>(GetController());FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
}
void ARingCar::CloseSettings(bool Apply){
 if(Apply){DraftSettings.ClampValues();if(!SaveDrivingSettings(DraftSettings)){SettingsStatus=TEXT("Could not save settings. Check that the Saved folder is writable.");return;}Settings=DraftSettings;ApplyDrivingSettings();}
 GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(SettingsWidget.ToSharedRef());SettingsWidget.Reset();SettingsOpen=false;
 SetDrivePaused(SettingsWasPaused);IgnoreMouseFrames=2;HadMouseCapture=false;
 auto* PC=Cast<APlayerController>(GetController());PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
}
void ARingCar::EndPlay(const EEndPlayReason::Type Reason){
 if(SettingsWidget.IsValid()&&GetWorld()->GetGameViewport())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(SettingsWidget.ToSharedRef());
 SettingsWidget.Reset();Super::EndPlay(Reason);
}
void ARingCar::RunSettingsTest(){
 if(Elapsed<1)return;
 if(SettingsTestPhase==0){
  const auto Original=Settings;OpenSettings();DraftSettings.Grip=1.2f;DraftSettings.Steering=48;DraftSettings.TyreSqueal=true;DraftSettings.DroneHeight=55;DraftSettings.CockpitMovement=0;CloseSettings(false);
  bool Cancelled=Settings.Grip==Original.Grip&&Settings.Steering==Original.Steering&&!Settings.TyreSqueal&&Settings.DroneHeight==Original.DroneHeight&&Settings.CockpitMovement==Original.CockpitMovement&&!DrivePaused;
  OpenSettings();DraftSettings.Grip=1.2f;DraftSettings.FrontTorque=.6f;DraftSettings.Steering=48;DraftSettings.HighSpeedSteering=22;DraftSettings.SteeringRate=120;DraftSettings.TractionControl=false;DraftSettings.TyreSqueal=true;DraftSettings.DroneHeight=55;DraftSettings.DroneDistance=85;DraftSettings.CockpitMovement=.73f;CloseSettings(true);
  auto Loaded=LoadDrivingSettings();bool Persisted=Loaded.Grip==1.2f&&Loaded.FrontTorque==.6f&&Loaded.Steering==48&&Loaded.HighSpeedSteering==22&&Loaded.SteeringRate==120&&!Loaded.TractionControl&&Loaded.TyreSqueal&&Loaded.DroneHeight==55&&Loaded.DroneDistance==85&&Loaded.CockpitMovement==.73f;
  ResetCar();bool ResetPreserved=Dynamics.GripSetting==1.2f&&Dynamics.LowSpeedSteering==48&&Dynamics.FrontShare==.6f&&!Dynamics.TractionControl&&TyreSquealEnabled;
  SetDrivePaused(true);
  const FRotator OriginalRotation=GetActorRotation();SetActorRotation(FRotator(12,37,-9),ETeleportType::TeleportPhysics);
  ViewMode=3;LookYaw=25;LookPitch=7;UpdateView(1.f/60,nullptr);
  const FVector Expected=GetActorLocation()+FVector(0,0,5500)-FRotator(0,62,0).Vector()*8500;
  const FRotator ActualRotation=Boom->GetComponentRotation();
  bool DronePose=Boom->GetComponentLocation().Equals(Expected,.1f)&&FMath::IsNearlyEqual(ActualRotation.Yaw,62.f,.01f)&&FMath::IsNearlyEqual(ActualRotation.Roll,0.f,.01f)&&FMath::IsNearlyEqual(ActualRotation.Pitch,-FMath::RadiansToDegrees(FMath::Atan2(55.f,85.f))*.5f-7,.01f);
  ViewMode=0;LookYaw=LookPitch=0;SetActorRotation(OriginalRotation,ETeleportType::TeleportPhysics);Settings.CockpitMovement=0;SetDrivePaused(false);UpdateView(1.f/60,nullptr);
  bool MovementDisabled=Boom->GetRelativeLocation().Equals(FVector(-12.4041891,-38.8264,68.379341),.001f);
  SetDrivePaused(true);OpenSettings();CloseSettings(false);bool PausePreserved=DrivePaused;SetDrivePaused(false);
  OpenSettings();DraftSettings=Ring::DrivingSettings();CloseSettings(true);Loaded=LoadDrivingSettings();bool Defaults=Loaded.Grip==1&&Loaded.Steering==42&&Loaded.TractionControl&&!Loaded.TyreSqueal&&Loaded.DroneHeight==30&&Loaded.DroneDistance==40&&Loaded.CockpitMovement==.5f;
  auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("cancel_preserves_settings"),Cancelled);J->SetBoolField(TEXT("apply_persists"),Persisted);J->SetBoolField(TEXT("reset_preserves_setup"),ResetPreserved);J->SetBoolField(TEXT("restores_prior_pause"),PausePreserved);J->SetBoolField(TEXT("defaults_persist"),Defaults);J->SetBoolField(TEXT("drone_world_pose"),DronePose);J->SetBoolField(TEXT("cockpit_movement_disabled"),MovementDisabled);J->SetBoolField(TEXT("passed"),Cancelled&&Persisted&&ResetPreserved&&PausePreserved&&Defaults&&DronePose&&MovementDisabled);
  FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(J,Writer);FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("settings_validation.json")));
  OpenSettings();SettingsTestPhase=1;
 }else if(SettingsTestPhase==1&&Elapsed>2){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingSettings.png"),true,false);SettingsTestPhase=2;}
 else if(SettingsTestPhase==2&&Elapsed>4){CloseSettings(false);SetDrivePaused(true);ViewMode=3;UpdateView(1.f/60,nullptr);SettingsTestPhase=3;}
 else if(SettingsTestPhase==3&&Elapsed>5){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingDrone.png"),true,false);SettingsTestPhase=4;}
 else if(SettingsTestPhase==4&&Elapsed>7){IFileManager::Get().Delete(*DrivingSettingsPath());FGenericPlatformMisc::RequestExit(false);SettingsTestPhase=5;}
}
