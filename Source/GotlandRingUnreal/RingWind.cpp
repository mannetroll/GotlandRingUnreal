#include "RingCar.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
void ARingCar::InitializeWindRotors(){
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("RingRotor"))){
  It->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);WindRotors.Add(*It);
 }
}
void ARingCar::AnimateWindRotors(float Dt){
 if(DrivePaused)return;
 // Unity's -60 degrees/s around world +X maps to +60 around Unreal +Y.
 const FQuat Delta(FVector::YAxisVector,FMath::DegreesToRadians(60.f*Dt));
 for(auto& Rotor:WindRotors)Rotor->AddActorWorldRotation(Delta);
}

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
void ARingCar::RunWindTest(float Dt){
 if(Elapsed<2)return;
 if(WindPhase==0){
  WindPassed=WindRotors.Num()==12;
  for(auto& R:WindRotors){WindStartRotations.Add(R->GetActorQuat());WindHubPositions.Add(R->GetActorLocation());}
  Chassis->SetSimulatePhysics(false);
  const FVector Hub=WindHubPositions[3],Position=Hub+FVector(0,-16000,-2000);
  Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
  Camera->SetWorldLocationAndRotation(Position,(Hub-Position).Rotation());
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("WindRotorStart.png"),false,false);
  WindPhase=1;WindSeconds=0;return;
 }
 if(WindPhase==4){FGenericPlatformMisc::RequestExit(false);return;}
 AnimateWindRotors(Dt);WindSeconds+=Dt;
 if(WindSeconds<1)return;
 const float Angle=WindPhase==2?0:60*WindSeconds;
 const FQuat Delta(FVector::YAxisVector,FMath::DegreesToRadians(Angle));
 float MaxAngleError=0,MaxHubError=0;
 for(int32 I=0;I<WindRotors.Num();I++){
  MaxHubError=FMath::Max(MaxHubError,float(FVector::Dist(WindRotors[I]->GetActorLocation(),WindHubPositions[I])));
  WindPassed&=WindRotors[I]->GetActorLocation().Equals(WindHubPositions[I],.001);
  const float Error=FMath::RadiansToDegrees(WindRotors[I]->GetActorQuat().AngularDistance(Delta*WindStartRotations[I]));
  MaxAngleError=FMath::Max(MaxAngleError,Error);WindPassed&=Error<.05f;
  WindStartRotations[I]=WindRotors[I]->GetActorQuat();
 }
 UE_LOG(LogTemp,Display,TEXT("Wind phase %d: angle error %f deg, hub error %f cm, seconds %f"),WindPhase,MaxAngleError,MaxHubError,WindSeconds);
 if(WindPhase==1){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("WindRotorMoving.png"),false,false);SetDrivePaused(true);}
 if(WindPhase==2)SetDrivePaused(false);
 if(WindPhase==3){
  FString Report=FString::Printf(TEXT("{\"rotors\":%d,\"rpm\":10,\"fixed_hubs_rotation_direction_pause_resume\":%s,\"passed\":%s}"),WindRotors.Num(),WindPassed?TEXT("true"):TEXT("false"),WindPassed?TEXT("true"):TEXT("false"));
  FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("wind_validation.json")));
 }
 WindPhase++;WindSeconds=0;
}
