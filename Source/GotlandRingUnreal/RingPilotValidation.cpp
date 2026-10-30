#include "RingCar.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
void ARingCar::RunPilotTest(float Dt){
 FVector P=GetActorLocation()/100;float T,Offset;bool Road;
 int I=Pilot->Nearest({float(P.X),float(P.Y)},T,Offset,Road),J=Pilot->Next(I);
 float D=Pilot->Rows[I].Distance+T*(J?Pilot->Rows[J].Distance-Pilot->Rows[I].Distance:7214.397f-Pilot->Rows[I].Distance);
 if(Elapsed>Dt*1.5f){float Delta=D-LastRouteDistance;if(Delta<-3600)Delta+=7214.397f;if(Delta>3600)Delta-=7214.397f;PilotTravel+=Delta;}
 LastRouteDistance=D;MaxPilotError=FMath::Max(MaxPilotError,PilotError);if(!Road)OffRoadTime+=Dt;
 if(Elapsed>=NextPilotLog){
  NextPilotLog+=1;
  if(PilotTelemetry.IsEmpty())PilotTelemetry=TEXT("time_s,distance_m,speed_kmh,target_kmh,line_error_m,on_road,contacts,segment\n");
  PilotTelemetry+=FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d\n"),Elapsed,PilotTravel,SpeedKmh,PilotTarget,PilotError,Road,GroundedWheels,I);
  FFileHelper::SaveStringToFile(PilotTelemetry,*(FPaths::ProjectSavedDir()/TEXT("pilot_telemetry.csv")));
 }
 if(CaptureView<3&&Elapsed>3+CaptureView*3){
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("GotlandRingView%d.png"),CaptureView),true,false);CaptureView++;
 }else if(CaptureView<3)ViewMode=CaptureView;else ViewMode=0;
 if(PilotTravel>=7214.397f||Elapsed>600){
  FString Report=FString::Printf(TEXT("{\"passed\":%s,\"distance_m\":%.3f,\"seconds\":%.3f,\"max_line_error_m\":%.4f,\"offroad_seconds\":%.3f}"),(PilotTravel>=7214.397f&&OffRoadTime<5)?TEXT("true"):TEXT("false"),PilotTravel,Elapsed,MaxPilotError,OffRoadTime);
  FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("pilot_validation.json")));FGenericPlatformMisc::RequestExit(false);
 }
}
