#include "RingCar.h"
#include "RingVersion.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RingReferenceData.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "RingRouteData.h"
#include "RingPresentationData.h"
#include "Components/TextRenderComponent.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
static_assert(UE_ARRAY_COUNT(Ring::CircuitBanking)==UE_ARRAY_COUNT(Ring::RouteData));
namespace {
FString ClockText(float Seconds){int32 Centis=FMath::Max(0,FMath::FloorToInt(Seconds*100));return FString::Printf(TEXT("%02d:%02d.%02d"),Centis/6000,(Centis/100)%60,Centis%100);}
const FColor BankColors[]={FColor(59,76,192),FColor(78,104,216),FColor(98,130,234),FColor(119,154,247),FColor(141,176,254),FColor(162,193,255),FColor(185,208,249),FColor(203,216,238),FColor(220,221,221),FColor(235,211,198),FColor(244,197,173),FColor(247,177,148),FColor(244,154,123),FColor(236,127,99),FColor(222,97,77),FColor(203,62,56),FColor(180,4,38)};
struct FMapLayout {
 FVector2D Min{BIG_NUMBER,BIG_NUMBER},Max{-BIG_NUMBER,-BIG_NUMBER};float BankLimit=0;
 FMapLayout(){for(int I=0;I<UE_ARRAY_COUNT(Ring::RouteData);I++){const auto& R=Ring::RouteData[I];Min.X=FMath::Min(Min.X,double(R.CY));Max.X=FMath::Max(Max.X,double(R.CY));Min.Y=FMath::Min(Min.Y,double(R.CX));Max.Y=FMath::Max(Max.Y,double(R.CX));BankLimit=FMath::Max(BankLimit,FMath::Abs(Ring::CircuitBanking[I]));}}
 FVector2D Project(FVector2D UnrealMetres)const{FVector2D Unity(UnrealMetres.Y,UnrealMetres.X);auto Q=(Unity-(Min+Max)*.5)*(232/FMath::Max(Max.X-Min.X,Max.Y-Min.Y));return {128+Q.X,128-Q.Y};}
 FLinearColor Color(float Banking)const{float P=FMath::Clamp((Banking+BankLimit)/(2*BankLimit)*16,0.f,16.f);int I=FMath::FloorToInt(P);return FMath::Lerp(FLinearColor::FromSRGBColor(BankColors[I]),FLinearColor::FromSRGBColor(BankColors[FMath::Min(I+1,16)]),P-I);}
};
const FMapLayout Map;
}
void ARingCar::InitializeSigns(){
 auto* SignMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Migration/Scenery/M_SignText.M_SignText"));
 for(TActorIterator<ATextRenderActor> It(GetWorld());It;++It){auto* Text=It->GetTextRender();const FString Value=Text->Text.ToString().Replace(TEXT("<br>"),TEXT("\n"));
  for(const auto& Layout:Ring::SignLayouts)if(Value==Layout.Text){
   Text->SetText(FText::FromString(Value.Replace(TEXT("\n"),TEXT("<br>"))));Text->SetWorldSize(100);
   Text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);Text->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
   const FVector Size=Text->GetTextLocalSize();const float Factor=FMath::Min(Layout.Width/Size.Y,Layout.Height/Size.Z);
   Text->SetWorldSize(100*Factor);Text->SetTextRenderColor(FColor::White);Text->SetTextMaterial(SignMaterial);
   const FVector Anchor(Layout.X,Layout.Y,Layout.Z);
   It->SetActorLocation(Anchor);Text->UpdateBounds();
   SignFitError=FMath::Max(SignFitError,float(FVector::Dist(Text->Bounds.Origin,Anchor)));
   const auto Actual=Text->GetTextLocalSize();SignFitError=FMath::Max(SignFitError,float(FMath::Max(Actual.Y-Layout.Width,Actual.Z-Layout.Height)));
   SignLabelsFitted++;break;
  }
 }
}
void ARingCar::UpdateLap(float Dt){
 float Fraction,Offset;bool OnRoad;auto P=GetActorLocation()/100;
 int Index=Pilot->Nearest({float(P.X),float(P.Y)},Fraction,Offset,OnRoad);
 Laps.Step(Dt,Index,Ring::CircuitStart,UE_ARRAY_COUNT(Ring::RouteData),OnRoad);
}
void ARingHUD::DrawHUD(){
 Super::DrawHUD();auto* Car=Cast<ARingCar>(GetOwningPawn());if(!Car||!Canvas||(Car->HideHUD&&!Car->ReferenceActive&&Car->SceneryView<0))return;
 const float S=FMath::Min(Canvas->SizeX/1600.f,Canvas->SizeY/900.f),W=Canvas->SizeX/S,H=Canvas->SizeY/S;
 const FLinearColor White=FLinearColor::White,Muted=FLinearColor::FromSRGBColor(FColor(156,167,185));
 auto Rect=[&](float X,float Y,float Width,float Height,FLinearColor Color){DrawRect(Color,X*S,Y*S,Width*S,Height*S);};
 auto Text=[&](const FString& Value,float X,float Y,float Size,FLinearColor Color){FCanvasTextItem Item(FVector2D(X*S,Y*S),FText::FromString(Value),FSlateFontInfo(GEngine->GetMediumFont(),FMath::RoundToInt(Size*.75f*S),Size>=22?FName("Bold"):FName("Regular")),Color);Canvas->DrawItem(Item);};
 auto Panel=[&](float X,float Y,float Width,float Height){
  FLinearColor Color=FLinearColor::FromSRGBColor(FColor(12,25,37,245));const float R=13;
  Rect(X+R,Y,Width-2*R,Height,Color);Rect(X,Y+R,R,Height-2*R,Color);Rect(X+Width-R,Y+R,R,Height-2*R,Color);
  TArray<FCanvasUVTri> Tris;
  for(int Corner=0;Corner<4;Corner++){FVector2D C(X+(Corner==0||Corner==3?R:Width-R),Y+(Corner<2?R:Height-R));
   float Start=180+Corner*90;
   for(int I=0;I<8;I++){float A=FMath::DegreesToRadians(Start+I*90.f/8),B=FMath::DegreesToRadians(Start+(I+1)*90.f/8);FCanvasUVTri T;
    T.V0_Pos=C*S;T.V1_Pos=(C+FVector2D(FMath::Cos(A),FMath::Sin(A))*R)*S;T.V2_Pos=(C+FVector2D(FMath::Cos(B),FMath::Sin(B))*R)*S;
    T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;T.V0_Color=T.V1_Color=T.V2_Color=Color;Tris.Add(T);
   }
  }FCanvasTriangleItem Item(Tris,GWhiteTexture);Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);
 };
 if(Car->ReferenceActive){
  const float Scale=FMath::Min(Canvas->SizeX/1920.f,Canvas->SizeY/1080.f),OX=(Canvas->SizeX-1920*Scale)/2,OY=(Canvas->SizeY-1080*Scale)/2;
  DrawRect(FLinearColor::FromSRGBColor(FColor(9,14,18)),0,0,Canvas->SizeX,Canvas->SizeY);
  DrawTexture(Car->BonnetTexture,OX,OY,1920*Scale,708*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
  DrawTexture(Car->CockpitTexture,OX,OY+708*Scale,960*Scale,372*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
  auto Label=[&](const FString& Value,float X,float Y,float Width,float Size,FLinearColor Color,bool Right=false){
   FSlateFontInfo Font(GEngine->GetMediumFont(),FMath::RoundToInt(Size*.75f*Scale),Size==32?FName("Bold"):FName("Regular"));
   FCanvasTextItem Item(FVector2D::ZeroVector,FText::FromString(Value),Font,Color);
   const FVector2D Extent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value,Font);
   Item.Position=FVector2D(OX+(X+Width*(Right?1:.5f))*Scale-Extent.X*(Right?1:.5f),OY+Y*Scale);Canvas->DrawItem(Item);
  };
  Label(TEXT("GOTLAND RING / ")+Ring::VersionLabel(),1100,24,770,32,White,true);
  Label(TEXT("SADAIR’S SPEAR • REFERENCE LAP"),1000,75,870,21,White,true);
  DrawRect(FLinearColor(.012,.025,.037),OX+16*Scale,OY+722*Scale,224*Scale,40*Scale);
  Label(TEXT("GAME COCKPIT"),16,729,224,21,White);
  Label(Car->ReferenceSeconds>=186?TEXT("LAP COMPLETE"):Car->ReferencePaused?TEXT("REPLAY PAUSED"):TEXT("REFERENCE LAP TIME"),990,753,900,21,FLinearColor(.66,.83,.79));
  int Hundredths=FMath::RoundToInt(Car->ReferenceClock*100);
  Label(FString::Printf(TEXT("%02d:%02d.%02d"),Hundredths/6000,Hundredths/100%60,Hundredths%100),990,805,900,120,White);
  Label(TEXT("POSITION-MATCHED GAME REPLAY"),990,948,900,21,White);
  DrawRect(FLinearColor(.24,.33,.36),OX+1030*Scale,OY+981*Scale,820*Scale,3*Scale);
  DrawRect(FLinearColor(.85,.62,.37),OX+1030*Scale,OY+981*Scale,820*Scale*Car->ReferenceClock/Ring::ReferenceLapDuration,3*Scale);
  Label(FString::Printf(TEXT("%s  Return to driving    Space / Esc  Pause    Home  Restart    %s  Sound"),*Car->KeyHint(EKeys::O),*Car->KeyHint(EKeys::M)),985,1000,910,17,White);
  Label(TEXT("Rally Car: SpatialNeglect / CC BY-NC 4.0"),990,1030,900,17,Muted);
  Label(TEXT("Track: © OpenStreetMap contributors • © Lantmäteriet"),990,1052,900,17,Muted);
  return;
 }
 if(Car->SceneryView>=0){
  Panel(25,25,700,122);Text(TEXT("GOTLAND RING / SCENERY"),45,35,20,White);
  Text(FString::Printf(TEXT("%d / 15   %s"),Car->SceneryView+1,*Car->SceneryViews[Car->SceneryView].Name),45,74,20,White);
  Text(FString::Printf(TEXT("%s: next view    %s / Escape: return to car    F2: screenshot"),*Car->KeyHint(EKeys::V),*Car->KeyHint(EKeys::C)),45,114,13,White);
  Panel(25,160,205,36);Panel(245,160,205,36);
  Text(Car->SceneryView==14?TEXT("Return to car [V]"):TEXT("Next scenery [V]"),40,169,15,White);Text(TEXT("Return to car [C]"),260,169,15,White);
  AddHitBox(FVector2D(25,160)*S,FVector2D(205,36)*S,TEXT("NextScenery"),true);AddHitBox(FVector2D(245,160)*S,FVector2D(205,36)*S,TEXT("ExitScenery"),true);
  Panel(25,H-81,700,56);Text(TEXT("Drive paused while viewing / Mannetroll Solutions AB"),45,H-72,13,White);
  Text(TEXT("Rally Car: SpatialNeglect / CC BY-NC 4.0"),45,H-49,13,Muted);return;
 }
 Panel(25,H-165,365,140);Text(FString::Printf(TEXT("%03d"),FMath::RoundToInt(Car->SpeedKmh)),42,H-161,55,White);
 Text(TEXT("km/h"),175,H-110,19,White);Text(Car->Dynamics.Throttle<0?TEXT("R"):FString::FromInt(Car->Dynamics.Gear),285,H-161,55,White);
 Rect(43,H-84,315,5,Muted);Rect(43,H-84,315*FMath::Clamp((Car->Settings.AwdMode?Car->Dynamics.Motor.Rpm():Car->ArcadeRpm)/7000,0.f,1.f),5,FLinearColor(1,.22,.1));
 Text(FString::Printf(TEXT("%d RPM    BOOST %.2f bar"),int(Car->Settings.AwdMode?Car->Dynamics.Motor.Rpm():Car->ArcadeRpm),Car->Dynamics.Boost*.9f),43,H-74,13,White);
 Text(Car->Settings.AwdMode?TEXT("AWD PHYSICS / H FOR ARCADE"):TEXT("ARCADE HANDLING / H FOR AWD"),43,H-45,13,White);
 const float InfoX=W-450;Panel(InfoX,H-215,425,190);
 Text(TEXT("Gotland Ring · Open practice"),InfoX+18,H-200,22,White);Text(TEXT("Impreza Rally · ")+Ring::VersionLabel(),InfoX+18,H-171,14,Muted);
 Text(FString::Printf(TEXT("Lap %d · %s"),Car->Laps.Lap,*ClockText(Car->Laps.Seconds)),InfoX+18,H-145,21,White);
 Text(Car->Laps.Best>0?TEXT("Best ")+ClockText(Car->Laps.Best):FString::Printf(TEXT("%.3f km"),Ring::CircuitLength/1000),InfoX+255,H-140,13,Muted);
 Text(FString::Printf(TEXT("%.0f FPS / %.1f ms · Side slip %.1f°"),1/FMath::Max(GetWorld()->GetDeltaSeconds(),.001f),GetWorld()->GetDeltaSeconds()*1000,FMath::Abs(Car->SideslipDegrees)),InfoX+18,H-112,13,Muted);
 Text(FString::Printf(TEXT("Dynamics [%s]    Camera [%s]    Menu [Esc]"),*Car->KeyHint(EKeys::F3),*Car->KeyHint(EKeys::C)),InfoX+18,H-85,13,White);
 Text(FString::Printf(TEXT("Autopilot %s [%s]   %s [%s]"),Car->Autopilot?TEXT("on"):TEXT("off"),*Car->KeyHint(EKeys::P),Car->MouseControl?TEXT("Mouse"):TEXT("Keyboard"),*Car->KeyHint(EKeys::U)),InfoX+18,H-62,13,White);
 Text(TEXT("Copyright © Mannetroll Solutions AB"),InfoX+18,H-40,12,Muted);
 if(Car->ViewMode!=0&&Car->Dashboard)DrawMaterial(Car->Dashboard,(W/2-215)*S,(H-155)*S,430*S,143*S,0,0,1,1);
 const float MX=W-260,MY=25,MapSize=235;Rect(MX,MY,MapSize,MapSize,FLinearColor(.015,.03,.04,.88));
 auto MP=[&](FVector2D P){return FVector2D(MX,MY)+Map.Project(P)*(MapSize/256);};
 for(int I=0;I<UE_ARRAY_COUNT(Ring::RouteData);I++){const auto&A=Ring::RouteData[I];const auto&B=Ring::RouteData[(I+1)%UE_ARRAY_COUNT(Ring::RouteData)];auto P=MP({A.CX,A.CY}),Q=MP({B.CX,B.CY});DrawLine(P.X*S,P.Y*S,Q.X*S,Q.Y*S,Map.Color(Ring::CircuitBanking[I]),2.4f*S);}
 auto F=Ring::RouteData[Ring::CircuitStart];auto Finish=MP({F.CX,F.CY});for(int I=0;I<4;I++)for(int J=0;J<3;J++)Rect(Finish.X+I*2,Finish.Y+J*2,2,2,(I+J)%2?FLinearColor::Black:White);
 FVector P=Car->GetActorLocation()/100;auto Marker=MP({P.X,P.Y});Marker.X=FMath::Clamp(Marker.X,double(MX+5),double(MX+230));Marker.Y=FMath::Clamp(Marker.Y,double(MY+5),double(MY+230));
 Rect(Marker.X-5,Marker.Y-5,10,10,FLinearColor::Black);Rect(Marker.X-3,Marker.Y-3,6,6,White);
 Text(TEXT("BANKING / + right edge higher"),MX+10,MY+7,12,White);
 Text(FString::Printf(TEXT("−%.1f°"),Map.BankLimit),MX+10,MY+204,12,White);Text(TEXT("0°"),MX+110,MY+204,12,White);Text(FString::Printf(TEXT("+%.1f°"),Map.BankLimit),MX+188,MY+204,12,White);
 for(int I=0;I<211;I++)Rect(MX+12+I,MY+225,1,4,Map.Color(FMath::Lerp(-Map.BankLimit,Map.BankLimit,I/210.f)));
 if(Car->Autopilot)Text(FString::Printf(TEXT("Pilot target %.0f km/h · Line error %.1f m"),Car->PilotTarget,Car->PilotError),25,25,16,White);
}
void ARingCar::RunPresentationTest(){
 if(Elapsed<2)return;
 if(PresentationTestPhase==0){
  SetDrivePaused(true);ViewMode=2;UpdateView(1.f/60,nullptr);
  auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("sign_labels"),SignLabelsFitted);J->SetNumberField(TEXT("sign_overflow_cm"),SignFitError);
  J->SetNumberField(TEXT("bank_limit_deg"),Map.BankLimit);J->SetNumberField(TEXT("map_sections"),UE_ARRAY_COUNT(Ring::CircuitBanking));
  const auto Before=KeyBindings;KeyDraft=KeyBindings;
  bool Reserved=!BindKey(EKeys::W,EKeys::Escape),Conflict=!BindKey(EKeys::W,ResolveKey(EKeys::S));
  bool Bind=BindKey(EKeys::W,EKeys::B)&&SaveKeys();KeyBindings.Empty();LoadKeys();bool Persist=ResolveKey(EKeys::W)==EKeys::B;
  KeyBindings=Before;IFileManager::Get().Delete(* (FPaths::ProjectSavedDir()/TEXT("KeyboardBindingsTest.json")));
  OpenMenu();bool Paused=DrivePaused;OpenKeyboard();MenuBack();bool Back=MenuPage==0;CloseMenu();bool Resumed=!DrivePaused;SetDrivePaused(true);
  bool MapFits=true;for(const auto& R:Ring::RouteData){auto Q=Map.Project({R.CX,R.CY});MapFits&=Q.X>=11&&Q.X<=245&&Q.Y>=11&&Q.Y<=245;}
  J->SetBoolField(TEXT("key_conflict_rejected"),Conflict);J->SetBoolField(TEXT("escape_reserved"),Reserved);J->SetBoolField(TEXT("keys_persist"),Bind&&Persist);J->SetBoolField(TEXT("menu_pause_back_resume"),Paused&&Back&&Resumed);J->SetBoolField(TEXT("map_fits"),MapFits);
  J->SetBoolField(TEXT("passed"),SignLabelsFitted==84&&SignFitError<.1&&MapFits&&Reserved&&Conflict&&Bind&&Persist&&Paused&&Back&&Resumed);
  FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("presentation_validation.json")));
  PresentationTestPhase=1;
 }else if(PresentationTestPhase==1&&Elapsed>3){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingHUD.png"),true,false);PresentationTestPhase=2;}
 else if(PresentationTestPhase==2&&Elapsed>4){OpenMenu();PresentationTestPhase=3;}
 else if(PresentationTestPhase==3&&Elapsed>5){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingMenu.png"),true,false);PresentationTestPhase=4;}
 else if(PresentationTestPhase==4&&Elapsed>6){OpenKeyboard();PresentationTestPhase=5;}
 else if(PresentationTestPhase==5&&Elapsed>7){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingKeys.png"),true,false);PresentationTestPhase=6;}
 else if(PresentationTestPhase==6&&Elapsed>8){FGenericPlatformMisc::RequestExit(false);}
}

#include "Camera/CameraComponent.h"
#include "Materials/Material.h"
void ARingCar::RunSignsTest(){
 if(Elapsed<2+SignsPhase)return;
 const TCHAR* Numbers[]={TEXT("07"),TEXT("03"),TEXT("11"),TEXT("31"),TEXT("35"),TEXT("42")};
 if(SignsPhase==0){
  SetDrivePaused(true);Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
  int32 UnlitWhite=0;
  for(TActorIterator<ATextRenderActor> It(GetWorld());It;++It){auto* T=It->GetTextRender();
   if(T->GetMaterial(0)&&T->GetMaterial(0)->GetPathName().Contains(TEXT("M_SignText"))&&T->GetMaterial(0)->GetMaterial()->GetShadingModels().HasShadingModel(MSM_Unlit)&&T->TextRenderColor==FColor::White)UnlitWhite++;
  }
  auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("boards"),42);J->SetNumberField(TEXT("labels"),SignLabelsFitted);J->SetNumberField(TEXT("max_centre_or_overflow_error_cm"),SignFitError);J->SetNumberField(TEXT("unlit_white_labels"),UnlitWhite);
  J->SetBoolField(TEXT("passed"),SignLabelsFitted==84&&UnlitWhite==84&&SignFitError<.1);
  FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("signs_validation.json")));
 }
 if(SignsPhase>=12){FGenericPlatformMisc::RequestExit(false);return;}
 const int32 I=SignsPhase/2;
 if(SignsPhase%2==0){
  for(TActorIterator<ATextRenderActor> It(GetWorld());It;++It)if(It->GetTextRender()->Text.ToString()==Numbers[I]){
   const FVector Number=It->GetActorLocation();FVector Name=Number;double Best=BIG_NUMBER;
   for(const auto& L:Ring::SignLayouts)if(!FString(L.Text).IsNumeric()){
    FVector P(L.X,L.Y,L.Z);double D=FVector::DistSquared(P,Number);if(D<Best){Best=D;Name=P;}
   }
   const FVector Centre=Number+(Name-Number)*(3.45/4.2),Position=Centre+It->GetActorForwardVector()*1400+FVector(0,0,60);
   Camera->SetWorldLocationAndRotation(Position,(Centre-Position).Rotation());break;
  }
 }else FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("TrackSign%s.png"),Numbers[I]),false,false);
 SignsPhase++;
}

void ARingHUD::NotifyHitBoxClick(FName BoxName){
 Super::NotifyHitBoxClick(BoxName);auto* Car=Cast<ARingCar>(GetOwningPawn());if(!Car||Car->SceneryView<0)return;
 if(BoxName==TEXT("NextScenery"))Car->NextSceneryView();else if(BoxName==TEXT("ExitScenery"))Car->ExitSceneryViews();
}
