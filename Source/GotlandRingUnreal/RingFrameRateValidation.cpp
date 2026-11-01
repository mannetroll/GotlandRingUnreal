#include "RingCar.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

void ARingCar::RunFrameRateTest(){
 if(Elapsed<3)return;
 const double Now=FPlatformTime::Seconds();
 if(FrameRateStart==0){FrameRateStart=Now;return;}
 FrameRateFrames++;
 const double Seconds=Now-FrameRateStart;
 if(Seconds<5)return;
 FrameRateSamples.Add(FrameRateFrames/Seconds);
 FrameRateFrames=0;FrameRateStart=Now;
 if(FrameRatePhase==0){OpenMenu();FrameRatePhase++;return;}
 if(FrameRatePhase==1){CloseMenu();FrameRatePhase++;return;}
 auto J=MakeShared<FJsonObject>();
 J->SetNumberField(TEXT("limit_fps"),GEngine->GetMaxFPS());
 J->SetNumberField(TEXT("driving_fps"),FrameRateSamples[0]);
 J->SetNumberField(TEXT("paused_menu_fps"),FrameRateSamples[1]);
 J->SetNumberField(TEXT("resumed_fps"),FrameRateSamples[2]);
 bool Passed=GEngine->GetMaxFPS()==60;
 for(double Fps:FrameRateSamples)Passed&=Fps>0&&Fps<=61;
 J->SetBoolField(TEXT("passed"),Passed);
 FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);
 FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("frame_rate_validation.json")));
 FGenericPlatformMisc::RequestExit(false);
}
