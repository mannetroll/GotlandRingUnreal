#include "RingCar.h"
#include "RingVersion.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#if PLATFORM_MAC
#include <pwd.h>
#include <unistd.h>
#endif
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlobj.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif
FString ARingCar::ScreenshotInfo()const{
 return FString::Printf(TEXT("%s\n%s / %s\nSpeed %.0f km/h · Gear %s\n%.0f RPM · Boost %.2f bar · Handling %s\nAutopilot %s · Mouse steering %s\nSide slip %.1f°\nTarget %.0f km/h · Gas %.0f%%\nLap %d · Time %.2f · Best %.2f\n%s\n%.0f FPS / %.1f ms"),Training?TEXT("TRAINING AREA / SLIDE PRACTICE"):TEXT("GOTLAND RING / OPEN PRACTICE"),*Models[SelectedCar].DisplayName,*PaintName(),SpeedKmh,*(Dynamics.Throttle<0?FString(TEXT("R")):FString::FromInt(Dynamics.Gear)),Settings.AwdMode?Dynamics.Motor.Rpm():ArcadeRpm,Dynamics.Boost*.9f,Settings.AwdMode?TEXT("AWD"):TEXT("Arcade"),Autopilot?TEXT("ON"):TEXT("OFF"),MouseControl?TEXT("ON"):TEXT("OFF"),FMath::Abs(SideslipDegrees),PilotTarget,Dynamics.Throttle*100,Laps.Lap,Laps.Seconds,Laps.Best,Training?TEXT("800 × 800 m / ASPHALT"):TEXT("Track 7.214 km"),1/GetWorld()->GetDeltaSeconds(),GetWorld()->GetDeltaSeconds()*1000);
}
void ARingCar::SaveDesktopScreenshot(){
 if(DesktopCapture.IsValid())return;
 DesktopCapture=UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&ARingCar::CaptureDesktopScreenshot);
 FScreenshotRequest::RequestScreenshot(true);
}
void ARingCar::CaptureDesktopScreenshot(int32 Width,int32 Height,const TArray<FColor>& Colors){
 UGameViewportClient::OnScreenshotCaptured().Remove(DesktopCapture);DesktopCapture.Reset();
 TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(Width,Height,Colors,Png);
 TArray64<uint8> Output;Output.Append(Png.GetData(),Png.Num()-12);
 auto UInt=[&](uint32 V){for(int Shift=24;Shift>=0;Shift-=8)Output.Add(uint8(V>>Shift));};
 auto Text=[&](const ANSICHAR* Keyword,const FString& Value){
  TArray<uint8> Data;Data.Append(reinterpret_cast<const uint8*>(Keyword),FCStringAnsi::Strlen(Keyword));Data.Append({0,0,0,0,0});FTCHARToUTF8 Utf8(*Value);Data.Append(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length());
  UInt(Data.Num());const uint8 Tag[]={'i','T','X','t'};Output.Append(Tag,4);Output.Append(Data.GetData(),Data.Num());
  uint32 Crc=0xffffffff;for(int64 I=Output.Num()-Data.Num()-4;I<Output.Num();I++){Crc^=Output[I];for(int Bit=0;Bit<8;Bit++)Crc=(Crc>>1)^((Crc&1)?0xedb88320u:0u);}UInt(~Crc);
 };
 Text("Software",TEXT("Gotland Ring ")+Ring::VersionLabel());Text("Creation Time",FDateTime::UtcNow().ToHttpDate());Text("Description",ScreenshotInfo());Text("HUD",HideHUD?TEXT("Hidden"):TEXT("Visible"));Text("Vehicle",Models[SelectedCar].DisplayName+TEXT(" · ")+PaintName());
 const auto R=Camera->GetComponentRotation();Text("Camera",FString::Printf(TEXT("View %d · Heading %.3f° · Pitch %.3f° · Roll %.3f° · FOV %.3f°"),ViewMode,R.Yaw,R.Pitch,R.Roll,Camera->FieldOfView));
 Output.Append(Png.GetData()+Png.Num()-12,12);
 FString Desktop;
#if PLATFORM_WINDOWS
 wchar_t* Path=nullptr;SHGetKnownFolderPath(FOLDERID_Desktop,0,nullptr,&Path);Desktop=Path;CoTaskMemFree(Path);
#else
 Desktop=FPaths::Combine(UTF8_TO_TCHAR(getpwuid(getuid())->pw_dir),TEXT("Desktop"));
#endif
 const FString Filename=Desktop/(TEXT("GotlandRing-")+FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S-"))+FGuid::NewGuid().ToString()+TEXT(".png"));
 const bool Saved=FFileHelper::SaveArrayToFile(Output,*Filename);
 UE_LOG(LogTemp,Display,TEXT("Desktop screenshot %s: %s"),Saved?TEXT("saved"):TEXT("failed"),*Filename);
 FFileHelper::SaveStringToFile(Saved?Filename:TEXT("FAILED"),*(FPaths::ProjectSavedDir()/TEXT("desktop_screenshot.txt")));
}
