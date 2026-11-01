#include "RingCar.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
namespace {
struct FControl {const TCHAR* Name;const TCHAR* Help;FKey Primary,Alternate;};
const FControl Controls[]={
 {TEXT("Accelerator"),TEXT("Drive forward"),EKeys::W,EKeys::Up},
 {TEXT("Brake"),TEXT("Slow down or stop"),EKeys::S,EKeys::Down},
 {TEXT("Steer left"),TEXT("Turn left"),EKeys::A,EKeys::Left},
 {TEXT("Steer right"),TEXT("Turn right"),EKeys::D,EKeys::Right},
 {TEXT("Reverse"),TEXT("Release the accelerator first"),EKeys::X,{}},
 {TEXT("Handbrake"),TEXT("Lock the rear wheels"),EKeys::SpaceBar,{}},
 {TEXT("Steering input"),TEXT("Keyboard / mouse steering"),EKeys::U,{}},
 {TEXT("Camera"),TEXT("Cockpit / bonnet / chase / drone"),EKeys::C,{}},
 {TEXT("Autopilot"),TEXT("Follow the Gotland Ring circuit"),EKeys::P,{}},
 {TEXT("Recover car"),TEXT("Return to the nearest road section"),EKeys::R,{}},
 {TEXT("Restart lap"),TEXT("Return to the starting position"),EKeys::Home,{}},
 {TEXT("Driving settings"),TEXT("Dynamics, view, sound and drone"),EKeys::F3,EKeys::F1},
 {TEXT("Fullscreen"),TEXT("Fullscreen / windowed"),EKeys::F,{}},
 {TEXT("Show / hide HUD"),TEXT("Hide the driving overlays"),EKeys::J,{}},
 {TEXT("Mute sound"),TEXT("Toggle driving sound"),EKeys::M,{}},
 {TEXT("Tyre squeal"),TEXT("Toggle tyre feedback"),EKeys::T,{}},
 {TEXT("Screenshot"),TEXT("Save the game view to Saved"),EKeys::F2,{}}
};
FString KeyFile(bool Test){return FPaths::ProjectSavedDir()/(Test?TEXT("KeyboardBindingsTest.json"):TEXT("KeyboardBindings.json"));}
const FSlateRoundedBoxBrush PanelBrush(FLinearColor(.012f,.017f,.026f,1),14.f);
class SRingMenuRoot:public SCompoundWidget {
public:
 SLATE_BEGIN_ARGS(SRingMenuRoot){} SLATE_ARGUMENT(TFunction<FReply(FKey)>,KeyHandler) SLATE_DEFAULT_SLOT(FArguments,Content) SLATE_END_ARGS()
 void Construct(const FArguments& Args){Handler=Args._KeyHandler;ChildSlot[Args._Content.Widget];}
 bool SupportsKeyboardFocus()const override{return true;}
 FReply OnKeyDown(const FGeometry&,const FKeyEvent& Event)override{return Handler(Event.GetKey());}
 TFunction<FReply(FKey)> Handler;
};
}
FKey ARingCar::ResolveKey(FKey Original)const{const auto* Key=KeyBindings.Find(Original);return Key?*Key:Original;}
FString ARingCar::KeyHint(FKey Original)const{FKey Key=ResolveKey(Original);return Key.IsValid()?Key.GetDisplayName().ToString():TEXT("Unassigned");}
bool ARingCar::Held(FKey Original)const{auto* PC=Cast<APlayerController>(GetController());return PC&&ResolveKey(Original).IsValid()&&PC->IsInputKeyDown(ResolveKey(Original));}
bool ARingCar::Pressed(FKey Original)const{auto* PC=Cast<APlayerController>(GetController());return PC&&ResolveKey(Original).IsValid()&&PC->WasInputKeyJustPressed(ResolveKey(Original));}
void ARingCar::LoadKeys(){
 FString Text;if(!FFileHelper::LoadFileToString(Text,*KeyFile(PresentationTest)))return;
 TSharedPtr<FJsonObject> Json;auto Reader=TJsonReaderFactory<>::Create(Text);if(!FJsonSerializer::Deserialize(Reader,Json)||!Json)return;
 TMap<FKey,FKey> Loaded;TSet<FKey> Used;
 for(const auto& C:Controls)for(FKey Original:{C.Primary,C.Alternate})if(Original.IsValid()){
  FString Name;FKey Key=Original;
  if(Json->TryGetStringField(Original.GetFName().ToString(),Name))Key=Name.IsEmpty()?FKey():FKey(FName(*Name));
  if(Key==EKeys::Escape||Key.IsModifierKey()||Key.IsGamepadKey()||Key.IsMouseButton()||(Key.IsValid()&&Used.Contains(Key))){UE_LOG(LogTemp,Warning,TEXT("Invalid keyboard setup; using defaults"));return;}
  if(Key.IsValid())Used.Add(Key);Loaded.Add(Original,Key);
 }
 KeyBindings=MoveTemp(Loaded);
}
bool ARingCar::SaveKeys(){
 auto Json=MakeShared<FJsonObject>();for(const auto& C:Controls)for(FKey Original:{C.Primary,C.Alternate})if(Original.IsValid()){
  const FKey* Key=KeyDraft.Find(Original);FKey Value=Key?*Key:Original;
  Json->SetStringField(Original.GetFName().ToString(),Value.IsValid()?Value.GetFName().ToString():TEXT(""));
 }
 FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Json,Writer);
 if(!FFileHelper::SaveStringToFile(Text,*KeyFile(PresentationTest))){KeyboardStatus=TEXT("Unable to save keyboard setup.");return false;}
 KeyBindings=KeyDraft;IgnoreMouseFrames=2;return true;
}
bool ARingCar::BindKey(FKey Original,FKey Replacement){
 if(Replacement==EKeys::Escape||Replacement.IsModifierKey()||Replacement.IsGamepadKey()||Replacement.IsMouseButton()){KeyboardStatus=TEXT("Choose a keyboard key. Escape is reserved for menus.");return false;}
 for(const auto& C:Controls)for(FKey Slot:{C.Primary,C.Alternate})if(Slot.IsValid()&&Slot!=Original){
  const FKey* Existing=KeyDraft.Find(Slot);
  if(Replacement.IsValid()&&(Existing?*Existing:Slot)==Replacement){KeyboardStatus=FString::Printf(TEXT("%s is already assigned to %s. Clear it first."),*Replacement.GetDisplayName().ToString(),C.Name);return false;}
 }
 KeyDraft.Add(Original,Replacement);KeyboardStatus.Empty();return true;
}
void ARingCar::ToggleFullscreen(){auto* SettingsObject=UGameUserSettings::GetGameUserSettings();SettingsObject->SetFullscreenMode(SettingsObject->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);SettingsObject->ApplyResolutionSettings(false);}
void ARingCar::OpenMenu(){SetDrivePaused(true);MenuPage=0;BuildMenu();}
void ARingCar::CloseMenu(){
 if(MenuWidget.IsValid())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());MenuWidget.Reset();CapturingKey=FKey();
 SetDrivePaused(false);IgnoreMouseFrames=2;HadMouseCapture=false;
 auto* PC=Cast<APlayerController>(GetController());PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;
 FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
void ARingCar::OpenKeyboard(){KeyDraft=KeyBindings;KeyboardStatus.Empty();CapturingKey=FKey();MenuPage=1;BuildMenu();}
void ARingCar::MenuBack(){
 if(SettingsOpen){CloseSettings(false);return;}
 if(MenuWidget.IsValid()){
  if(CapturingKey.IsValid()){CapturingKey=FKey();KeyboardStatus.Empty();BuildMenu();}
  else if(MenuPage==1){MenuPage=0;BuildMenu();}else CloseMenu();
 }else if(DrivePaused)SetDrivePaused(false);else OpenMenu();
}
void ARingCar::BuildMenu(){
 if(MenuWidget.IsValid())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
 auto Panel=SNew(SVerticalBox);
 auto Label=[&](const FString& Text,int Size,FLinearColor Color){Panel->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle(Size>=24?"Bold":"Regular",Size)).ColorAndOpacity(Color).AutoWrapText(true)];};
 Label(TEXT("GOTLAND RING"),14,FLinearColor(.2f,.46f,1));
 Label(MenuPage==0?TEXT("Practice paused"):TEXT("Keyboard help & setup"),32,FLinearColor::White);
 Label(MenuPage==0?TEXT("Circuit driving and timed laps. Resume when you are ready."):TEXT("Click a key, then press its replacement. Save applies to future launches."),16,FLinearColor(.45f,.5f,.6f));
 auto Button=[&](const FString& Text,TFunction<void()> Action,bool Primary=false){
  Panel->AddSlot().AutoHeight().Padding(0,4)[SNew(SBox).HeightOverride(42)[SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center).ButtonColorAndOpacity(Primary?FLinearColor(.07f,.24f,.6f):FLinearColor(.07f,.09f,.13f)).OnClicked_Lambda([Action](){Action();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular",16))]]];
 };
 if(MenuPage==0){
  Button(TEXT("Resume driving [Esc]"),[this](){CloseMenu();},true);
  Button(TEXT("Restart lap"),[this](){CloseMenu();ResetCar();});
  auto SettingsAction=[this](int32 Page){CloseMenu();SetDrivePaused(true);ReturnToMenu=true;SettingsPage=Page;OpenSettings();};
  Button(FString::Printf(TEXT("Driving dynamics [%s]"),*KeyHint(EKeys::F3)),[SettingsAction](){SettingsAction(0);});
  Button(TEXT("View & sound"),[SettingsAction](){SettingsAction(1);});
  Button(TEXT("Drone camera"),[SettingsAction](){SettingsAction(2);});
  Button(TEXT("Keyboard help & setup"),[this](){OpenKeyboard();});
  Button(TEXT("Quit"),[](){FGenericPlatformMisc::RequestExit(false);});
 }else{
  auto Rows=SNew(SVerticalBox);
  for(const auto& C:Controls){auto Row=SNew(SHorizontalBox);
   Row->AddSlot().FillWidth(1).VAlign(VAlign_Center)[SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(C.Name)).Font(FCoreStyle::GetDefaultFontStyle("Bold",15))]
    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(C.Help)).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).ColorAndOpacity(FLinearColor(.45,.5,.6))]];
   for(FKey Original:{C.Primary,C.Alternate}){
    auto Cell=SNew(SHorizontalBox);
    if(Original.IsValid()){
     const FKey* Value=KeyDraft.Find(Original);const FKey Key=Value?*Value:Original;
     Cell->AddSlot().FillWidth(1)[SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center).OnClicked_Lambda([this,Original](){CapturingKey=Original;KeyboardStatus.Empty();BuildMenu();return FReply::Handled();})
      [SNew(STextBlock).Text(FText::FromString(CapturingKey==Original?TEXT("Press key…"):Key.IsValid()?Key.GetDisplayName().ToString():TEXT("—"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",13))]];
     Cell->AddSlot().AutoWidth()[SNew(SButton).VAlign(VAlign_Center).OnClicked_Lambda([this,Original](){BindKey(Original,FKey());CapturingKey=FKey();BuildMenu();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("×"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",13))]];
    }
    Row->AddSlot().AutoWidth().Padding(5,0)[SNew(SBox).WidthOverride(125)[Cell]];
   }
   Rows->AddSlot().AutoHeight().Padding(0,5)[Row];
  }
  Panel->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(325)[SNew(SScrollBox)+SScrollBox::Slot()[Rows]]];
  Label(KeyboardStatus.IsEmpty()?TEXT("Mouse: look · Right-click: centre · Escape: menu / back"):KeyboardStatus,14,FLinearColor(.5,.65,.9));
  Button(TEXT("Restore defaults"),[this](){KeyDraft.Empty();CapturingKey=FKey();BuildMenu();});
  Button(TEXT("Cancel [Esc]"),[this](){MenuPage=0;BuildMenu();});
  Button(TEXT("Save & back"),[this](){if(SaveKeys())MenuPage=0;BuildMenu();},true);
 }
 Panel->AddSlot().AutoHeight().Padding(0,16,0,0)[SNew(STextBlock).Text(FText::FromString(TEXT("Impreza Rally · Copyright © Mannetroll Solutions AB"))).Font(FCoreStyle::GetDefaultFontStyle("Regular",12)).ColorAndOpacity(FLinearColor(.4,.45,.55))];
 MenuWidget=SNew(SRingMenuRoot).KeyHandler([this](FKey Key){
  if(Key==EKeys::Escape){MenuBack();return FReply::Handled();}
  if(CapturingKey.IsValid()){if(BindKey(CapturingKey,Key))CapturingKey=FKey();BuildMenu();return FReply::Handled();}
  return FReply::Unhandled();
 })[SNew(SOverlay)
  +SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,.65f))]
  +SOverlay::Slot().Padding(20)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
   [SNew(SBox).WidthOverride(MenuPage==1?900:760)[SNew(SBorder).BorderImage(&PanelBrush).Padding(32)[Panel]]]]];
 GetWorld()->GetGameViewport()->AddViewportWidgetContent(MenuWidget.ToSharedRef(),110);
 auto* PC=Cast<APlayerController>(GetController());FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(MenuWidget);Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
 FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
}
