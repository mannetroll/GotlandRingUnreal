#include "RingCar.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "UnrealClient.h"

void ARingCar::UpdateInstruments(float Rpm,float MetresPerSecond,int32 Gear,float Fuel){
 if(!Dashboard)return;
 Dashboard->SetScalarParameterValue(TEXT("Rpm"),Rpm);
 Dashboard->SetScalarParameterValue(TEXT("Speed"),FMath::Abs(MetresPerSecond)*3.6f);
 Dashboard->SetScalarParameterValue(TEXT("Gear"),Gear);
 Dashboard->SetScalarParameterValue(TEXT("Fuel"),Fuel);
 Dashboard->SetScalarParameterValue(TEXT("TripKm"),TripKm);
}
void ARingCar::RunInstrumentsTest(){
 if(Elapsed<2)return;
 if(InstrumentsTestPhase==0){
  SetDrivePaused(true);ViewMode=0;LookPitch=15;UpdateView(1.f/60,nullptr);
  auto Report=MakeShared<FJsonObject>();bool Passed=Dashboard!=nullptr;
  Report->SetBoolField(TEXT("live_material"),Passed);
  bool SteeringChecked=false,SteeringCorrect=true;
  for(int32 I=0;I<Visuals.Num();I++)if(VisualWheels[I]==-3){
   SteeringChecked=true;auto* Wheel=Visuals[I].Get();const FVector Centre=Wheel->GetRelativeLocation();
   const FVector Axis=FVector(.982749939,0,-.184939444).GetSafeNormal();
   const FVector Top=(FVector::UpVector-Axis*FVector::DotProduct(FVector::UpVector,Axis)).GetSafeNormal();
   AnimateCockpitSteering(1);SteeringCorrect&=Wheel->GetRelativeRotation().Quaternion().RotateVector(Top).Y>.1;
   AnimateCockpitSteering(-1);SteeringCorrect&=Wheel->GetRelativeRotation().Quaternion().RotateVector(Top).Y<-.1;
   AnimateCockpitSteering(0);SteeringCorrect&=Wheel->GetRelativeRotation().IsNearlyZero(.01)&&Wheel->GetRelativeLocation().Equals(Centre,.001);
  }
  AnimateCockpitSteering(Dynamics.SteeringDegrees);
  Passed&=SteeringChecked&&SteeringCorrect;
  Report->SetBoolField(TEXT("cockpit_steering_direction_and_centre"),SteeringChecked&&SteeringCorrect);

  if(Dashboard){
   auto* Target=NewObject<UTextureRenderTarget2D>(this);
   Target->ClearColor=FLinearColor::Black;Target->InitCustomFormat(1200,400,PF_B8G8R8A8,false);Target->UpdateResourceImmediate(true);
   auto Capture=[&](const TCHAR* Name){
    UKismetRenderingLibrary::ClearRenderTarget2D(this,Target,FLinearColor::Black);
    UKismetRenderingLibrary::DrawMaterialToRenderTarget(this,Target,Dashboard);
    TArray<FColor> Pixels;Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
    TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(1200,400,Pixels,Png);
    FFileHelper::SaveArrayToFile(Png,*(FPaths::ProjectSavedDir()/Name));return Pixels;
   };
   auto Needle=[&](const TArray<FColor>& Pixels,FVector2D Centre,float Length,float Angle,const TCHAR* Label){
    const float Radians=FMath::DegreesToRadians(Angle);FVector2D Direction(FMath::Cos(Radians),FMath::Sin(Radians));int32 Hits=0;
    for(int32 I=0;I<10;I++){
     FVector2D P=Centre+Direction*FMath::Lerp(Length>70?26.f:17.f,Length*.82f,I/9.f);
     const FColor C=Pixels[(399-FMath::RoundToInt(P.Y))*1200+FMath::RoundToInt(P.X)];
     if(C.R>153&&C.R>C.G*1.6f&&C.R>C.B*2)Hits++;
    }
    Report->SetNumberField(Label,Hits);Passed&=Hits>=7;
   };
   TripKm=12.3f;UpdateInstruments(900,0,1);
   auto Idle=Capture(TEXT("InstrumentsIdle.png"));
   bool TripLeadingZeros=true;
   for(int32 Column=0;Column<3;Column++){
    const int32 X=563+Column*14;
    TripLeadingZeros&=Idle[297*1200+X].G>Idle[288*1200+X].G+5;
   }
   Passed&=TripLeadingZeros;Report->SetBoolField(TEXT("trip_leading_zero_pixels"),TripLeadingZeros);
   Needle(Idle,{898,169},112,198,TEXT("idle_rpm_hits"));Needle(Idle,{600,183},125,225,TEXT("stopped_speed_hits"));Needle(Idle,{230,181},43,80,TEXT("full_fuel_hits"));
   UpdateInstruments(6500,30,4,.05f);auto Fast=Capture(TEXT("InstrumentsFast.png"));
   Needle(Fast,{898,169},112,30,TEXT("fast_rpm_hits"));Needle(Fast,{600,183},125,63,TEXT("fast_speed_hits"));Needle(Fast,{230,181},43,194,TEXT("low_fuel_hits"));
   UpdateInstruments(2400,-5,-1);auto Reverse=Capture(TEXT("InstrumentsReverse.png"));
   const bool ReverseState=Dashboard->K2_GetScalarParameterValue(TEXT("Gear"))==-1&&FMath::IsNearlyEqual(Dashboard->K2_GetScalarParameterValue(TEXT("Speed")),18.f)&&FMath::IsNearlyEqual(Dashboard->K2_GetScalarParameterValue(TEXT("TripKm")),12.3f);
   Passed&=ReverseState;Report->SetBoolField(TEXT("reverse_and_trip_state"),ReverseState);
   int32 Changed=0;for(int32 Y=315;Y<339;Y++)for(int32 X=366;X<385;X++)if(Reverse[Y*1200+X]!=Idle[Y*1200+X])Changed++;
   Passed&=Changed>20;Report->SetNumberField(TEXT("reverse_gear_changed_pixels"),Changed);
   UpdateInstruments(4500,30,3);
  }
  Report->SetBoolField(TEXT("passed"),Passed);FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Report,Writer);
  FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("instruments_validation.json")));InstrumentsTestPhase=1;
 }else if(InstrumentsTestPhase==1&&Elapsed>4){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("GotlandRingInstruments.png"),true,false);InstrumentsTestPhase=2;}
 else if(InstrumentsTestPhase==2&&Elapsed>6){FGenericPlatformMisc::RequestExit(false);InstrumentsTestPhase=3;}
}
