#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "RingDynamics.h"
#include "RingPilot.h"
#include "RingCar.generated.h"
class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;
class URingAudio;
class UMaterialInstanceDynamic;
UCLASS()
class GOTLANDRINGUNREAL_API ARingCar : public APawn {
 GENERATED_BODY()
public:
 ARingCar();
 virtual void BeginPlay() override;
 virtual void Tick(float Dt) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Chassis;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UStaticMesh>> VisualMeshes;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> VisualWheels;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector> WheelCenters;
 UPROPERTY(VisibleAnywhere) TObjectPtr<URingAudio> EngineAudio;
 Ring::Dynamics Dynamics;
 float SideslipDegrees=0;
 UFUNCTION(BlueprintCallable) void ResetCar();
 float SpeedKmh=0;
 int32 GroundedWheels=0;
 bool Autopilot=false;
 int32 ViewMode=0;
 float PilotTarget=0,PilotError=0;
private:
 TArray<TObjectPtr<UStaticMeshComponent>> Visuals;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Dashboard;
 TUniquePtr<Ring::Pilot> Pilot;
 void UpdateView(float Dt,APlayerController* PC);
 void RecoverCar();
 float LookYaw=0,LookPitch=0,MouseSteering=0,ChaseYaw=0;
 bool MouseControl=false,DrivePaused=false,HadMouseCapture=false;
 int32 IgnoreMouseFrames=2;
 FVector PauseVelocity,PauseAngular;
 FVector PreviousVelocity=FVector::ZeroVector,ViewAcceleration=FVector::ZeroVector,ViewLean=FVector::ZeroVector,ViewTilt=FVector::ZeroVector;
 float ViewDistance=0,RoadMix=0,LooseMix=0;
 TWeakObjectPtr<UStaticMeshComponent> RoadSurface;
 FTransform StartPose,RoadPose;
 bool PilotTest=false;
 float PilotTravel=0,LastRouteDistance=0,MaxPilotError=0,OffRoadTime=0,NextPilotLog=0;
 int32 CaptureView=0;
 FString PilotTelemetry;
 void RunPilotTest(float Dt);
 bool DynamicsTest=false;
 int32 DynamicsPhase=0,MaxGear=1;
 float PeakRpm=0,PeakRearSlip=0,BrakeDriveMax=0,ReverseKmh=0,RecoverySlip=0;
 FString DynamicsTelemetry;
 void SetupDynamicsTest();
 void RunDynamicsTest(float& Throttle,float& Brake,float& Steering);
 float Elapsed=0, MaxSpeed=0, BrakeStart=0, BrakeEnd=0, SteerYaw=0, TestDistance=0;
 FVector TestStart;
 bool TestMode=false, TestFinished=false, Muted=false, ScreenshotTaken=false, SteeringTestStarted=false;
};
UCLASS()
class GOTLANDRINGUNREAL_API ARingHUD : public AHUD {
 GENERATED_BODY()
 virtual void DrawHUD() override;
};
UCLASS()
class GOTLANDRINGUNREAL_API ARingGameMode : public AGameModeBase {
 GENERATED_BODY()
public: ARingGameMode();
};
