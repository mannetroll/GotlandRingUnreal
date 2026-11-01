#include "RingCar.h"
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
 for(TActorIterator<ATextRenderActor> It(GetWorld());It;++It){auto* Text=It->GetTextRender();const FString Value=Text->Text.ToString().Replace(TEXT("<br>"),TEXT("\n"));
  for(const auto& Layout:Ring::SignLayouts)if(Value==Layout.Text){
   Text->SetText(FText::FromString(Value.Replace(TEXT("\n"),TEXT("<br>"))));Text->SetWorldSize(100);
   Text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);Text->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
   const FVector Size=Text->GetTextLocalSize();const float Factor=FMath::Min(Layout.Width/Size.Y,Layout.Height/Size.Z);
   Text->SetWorldSize(100*Factor);
   FVector Center,Extent;It->GetActorBounds(false,Center,Extent);
   It->SetActorLocation(It->GetActorLocation()+FVector(Layout.X,Layout.Y,Layout.Z)-Center);
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
 Super::DrawHUD();auto* Car=Cast<ARingCar>(GetOwningPawn());if(!Car||Car->HideHUD||!Canvas)return;
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
 Panel(25,H-165,365,140);Text(FString::Printf(TEXT("%03d"),FMath::RoundToInt(Car->SpeedKmh)),42,H-161,55,White);
 Text(TEXT("km/h"),175,H-110,19,White);Text(Car->Dynamics.Throttle<0?TEXT("R"):FString::FromInt(Car->Dynamics.Gear),285,H-161,55,White);
 Rect(43,H-84,315,5,Muted);Rect(43,H-84,315*FMath::Clamp(Car->Dynamics.Motor.Rpm()/7000,0.f,1.f),5,FLinearColor(1,.22,.1));
 Text(FString::Printf(TEXT("%d RPM    BOOST %.2f bar"),int(Car->Dynamics.Motor.Rpm()),Car->Dynamics.Boost*.9f),43,H-74,13,White);
 Text(TEXT("AWD PHYSICS"),43,H-45,13,White);
 const float InfoX=W-450;Panel(InfoX,H-215,425,190);
 Text(TEXT("Gotland Ring · Open practice"),InfoX+18,H-200,22,White);Text(TEXT("Impreza Rally"),InfoX+18,H-171,14,Muted);
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
