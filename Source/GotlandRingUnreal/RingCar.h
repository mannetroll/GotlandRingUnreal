#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "RingDynamics.h"
#include "RingPilot.h"
#include "RingDrivingSettings.h"
#include "RingLap.h"
#include "RingRide.h"
#include "RingCar.generated.h"
class UProceduralMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
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
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
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
 bool Autopilot=false,TyreSquealEnabled=false;
 int32 ViewMode=0;
 float PilotTarget=0,PilotError=0;
 Ring::LapClock Laps;
 bool HideHUD=false;
 bool IsDrivePaused()const{return DrivePaused;}
 bool IsMouseSteering()const{return MouseControl;}
 UMaterialInstanceDynamic* InstrumentMaterial()const{return Dashboard;}
 FString KeyHint(FKey Original)const;
 void OpenMenu();
 void CloseMenu();
 void ToggleFullscreen();
private:
 friend class ARingHUD;
 UPROPERTY() TObjectPtr<UProceduralMeshComponent> DriverBody;
 UPROPERTY() TObjectPtr<USceneCaptureComponent2D> ReplayBonnet;
 UPROPERTY() TObjectPtr<USceneCaptureComponent2D> ReplayCockpit;
 UPROPERTY() TObjectPtr<UTextureRenderTarget2D> BonnetTexture;
 UPROPERTY() TObjectPtr<UTextureRenderTarget2D> CockpitTexture;
 FRingRide ReplayRide;
 FVector SurfaceContacts()const;
 void InitializeDriver();
 void AnimateDriver(float RoadDegrees);
 void ToggleReferenceLap();
 void SeekReference(float Seconds);
 void TickReference(float Dt);
 FVector ReferencePoint(float Distance)const;
 bool ReferenceActive=false,ReferencePaused=false,ReferenceWasPaused=false;
 float ReferenceSeconds=0,ReferenceClock=0,ReferenceRoll=0;
 FTransform ReferenceDrivePose;
 TArray<FTransform> ReferenceVisualPoses;
 void NextSceneryView();
 void ExitSceneryViews();
 int32 SceneryView=-1;
 bool SceneryWasPaused=false;
 struct FSceneryView {FString Name;FVector Position;FRotator Rotation;float Size;};
 TArray<FSceneryView> SceneryViews;
 void MakeSceneryViews();
 void ToggleHandling();
 void StepArcade(float Dt,float Throttle,float Brake,float Steering,bool Reverse);
 bool ArcadeReverse=false;
 float ArcadeSteer=0,ArcadeRpm=900,ArcadeBoost=0;
 FVector ArcadeVelocity=FVector::ZeroVector;
 bool MigrationTest=false;
 int32 MigrationPhase=0;
 void RunMigrationTest(float Dt);

 TArray<TWeakObjectPtr<AActor>> WindRotors;
 TArray<FQuat> WindRotations;
 void InitializeWindRotors();
 void AnimateWindRotors(float Dt);
 void RunWindTest(float Dt);
 bool WindTest=false,WindPassed=true;
 int32 WindPhase=0;
 float WindSeconds=0;
 TArray<FQuat> WindStartRotations;
 TArray<FVector> WindHubPositions;
 TSharedPtr<class SWidget> MenuWidget;
 TMap<FKey,FKey> KeyBindings,KeyDraft;
 FKey CapturingKey;
 FString KeyboardStatus;
 int32 MenuPage=0,SettingsPage=0;
 bool ReturnToMenu=false;
 void BuildMenu();
 void OpenKeyboard();
 void LoadKeys();
 bool SaveKeys();
 bool BindKey(FKey Original,FKey Replacement);
 FKey ResolveKey(FKey Original)const;
 bool Held(FKey Original)const;
 bool Pressed(FKey Original)const;
 void MenuBack();
 void InitializeSigns();
 bool SignsTest=false;
 int32 SignsPhase=0;
 void RunSignsTest();
 void UpdateLap(float Dt);
 bool PresentationTest=false;
 int32 PresentationTestPhase=0,SignLabelsFitted=0;
 float SignFitError=0;
 void RunPresentationTest();
 Ring::DrivingSettings Settings,DraftSettings;
 TSharedPtr<class SWidget> SettingsWidget;
 bool SettingsOpen=false,SettingsWasPaused=false,SettingsTest=false;
 int32 SettingsTestPhase=0;
 FString SettingsStatus;
 void OpenSettings();
 void BuildSettings();
 void CloseSettings(bool Apply);
 void ApplyDrivingSettings();
 void SetDrivePaused(bool Paused);
 Ring::DrivingSettings LoadDrivingSettings()const;
 bool SaveDrivingSettings(const Ring::DrivingSettings& Values)const;
 FString DrivingSettingsPath()const;
 void RunSettingsTest();
 TArray<TObjectPtr<UStaticMeshComponent>> Visuals;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Dashboard;
 bool InstrumentsTest=false;
 int32 InstrumentsTestPhase=0;
 float TripKm=0;
 void AnimateCockpitSteering(float RoadDegrees);
 void UpdateInstruments(float Rpm,float MetresPerSecond,int32 Gear,float Fuel=1);
 void RunInstrumentsTest();
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
 float PeakTyreSqueal=0;
 float Elapsed=0, MaxSpeed=0, BrakeStart=0, BrakeEnd=0, SteerYaw=0, TestDistance=0;
 FVector TestStart;
 bool TestMode=false, TestFinished=false, Muted=false, ScreenshotTaken=false, SteeringTestStarted=false;
};
UCLASS()
class GOTLANDRINGUNREAL_API ARingHUD : public AHUD {
 GENERATED_BODY()
 virtual void DrawHUD() override;
 virtual void NotifyHitBoxClick(FName BoxName) override;
};
UCLASS()
class GOTLANDRINGUNREAL_API ARingGameMode : public AGameModeBase {
 GENERATED_BODY()
public: ARingGameMode();
};
