#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "RingDynamics.h"
#include "RingCar.generated.h"
class UBoxComponent;
class USpringArmComponent;
class UCameraComponent;
class URingAudio;
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
private:
 TArray<TObjectPtr<UStaticMeshComponent>> Visuals;
 TWeakObjectPtr<UStaticMeshComponent> RoadSurface;
 FTransform StartPose,RoadPose;
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
