#include "RingCar.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
void ARingCar::SetupDynamicsTest(){
 RoadPose=StartPose;
 auto* Pad=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(300000,300000,-50),FRotator::ZeroRotator);
 Pad->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Pad->SetActorScale3D(FVector(1000,1000,1));Pad->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));Pad->Tags.Add(TEXT("ValidationAsphalt"));
 StartPose=FTransform(FRotator::ZeroRotator,FVector(270000,300000,50));ResetCar();
 DynamicsTelemetry=TEXT("time_s,speed_kmh,rpm,gear,clutch_nm,steering_deg,sideslip_deg,front_slip,rear_slip,contacts,front_drive_nm,rear_drive_nm,grip\n");
}
void ARingCar::RunDynamicsTest(float& Throttle,float& Brake,float& Steering){
 Throttle=Elapsed>2&&Elapsed<12?1.f:Elapsed>20&&Elapsed<24?1.f:Elapsed>28&&Elapsed<31?-.7f:0;
 Brake=Elapsed>=12&&Elapsed<19?1.f:0;
 Steering=Elapsed>20&&Elapsed<24?(int32((Elapsed-20)/.8f)%2==0?.8f:-.8f):0;
 MaxGear=FMath::Max(MaxGear,Dynamics.Gear);PeakRpm=FMath::Max(PeakRpm,Dynamics.Motor.Rpm());
 PeakRearSlip=FMath::Max(PeakRearSlip,FMath::Max(FMath::Abs(Dynamics.Wheels[2].LateralSlip),FMath::Abs(Dynamics.Wheels[3].LateralSlip)));
 if(Elapsed>12.05f&&Elapsed<19)for(auto& W:Dynamics.Wheels)BrakeDriveMax=FMath::Max(BrakeDriveMax,FMath::Abs(W.Drive));
 if(Elapsed>=12&&DynamicsPhase==0){BrakeStart=SpeedKmh;DynamicsPhase=1;}
 if(Elapsed>=19&&DynamicsPhase==1){BrakeEnd=SpeedKmh;ResetCar();DynamicsPhase=2;}
 if(Elapsed>=27&&DynamicsPhase==2){RecoverySlip=FMath::Abs(SideslipDegrees);SteerYaw=FMath::Abs(GetActorRotation().Yaw);ResetCar();DynamicsPhase=3;}
 if(Elapsed>=31&&DynamicsPhase==3){
  ReverseKmh=FVector::DotProduct(Chassis->GetPhysicsLinearVelocity(),GetActorForwardVector())*.036f;
  int32 Contacts=GroundedWheels;
  auto J=MakeShared<FJsonObject>();
  J->SetNumberField(TEXT("max_speed_kmh"),MaxSpeed);J->SetNumberField(TEXT("peak_rpm"),PeakRpm);J->SetNumberField(TEXT("max_gear"),MaxGear);
  J->SetNumberField(TEXT("brake_start_kmh"),BrakeStart);J->SetNumberField(TEXT("brake_end_kmh"),BrakeEnd);
  J->SetNumberField(TEXT("max_drive_torque_while_braking_nm"),BrakeDriveMax);J->SetNumberField(TEXT("steering_yaw_deg"),SteerYaw);
  J->SetNumberField(TEXT("peak_rear_lateral_slip"),PeakRearSlip);J->SetNumberField(TEXT("lift_recovery_sideslip_deg"),RecoverySlip);
  J->SetNumberField(TEXT("reverse_kmh"),ReverseKmh);J->SetNumberField(TEXT("final_contacts"),Contacts);
  J->SetBoolField(TEXT("passed"),MaxSpeed>85&&MaxGear>=2&&PeakRpm<7000&&BrakeEnd<2&&BrakeDriveMax<.01f&&SteerYaw>2&&PeakRearSlip>.2f&&RecoverySlip<5&&ReverseKmh< -3&&Contacts>=3);
  FString Out;auto Writer=TJsonWriterFactory<>::Create(&Out);FJsonSerializer::Serialize(J,Writer);
  FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("dynamics_validation.json")));
  FFileHelper::SaveStringToFile(DynamicsTelemetry,*(FPaths::ProjectSavedDir()/TEXT("dynamics_telemetry.csv")));
  StartPose=RoadPose;ResetCar();DynamicsPhase=4;
 }
 if(DynamicsPhase<4)DynamicsTelemetry+=FString::Printf(TEXT("%.4f,%.4f,%.3f,%d,%.4f,%.4f,%.4f,%.5f,%.5f,%d,%.4f,%.4f,%.3f\n"),Elapsed,SpeedKmh,Dynamics.Motor.Rpm(),Dynamics.Gear,Dynamics.ClutchTorque,Dynamics.SteeringDegrees,SideslipDegrees,FMath::Abs(Dynamics.Wheels[0].LateralSlip),FMath::Abs(Dynamics.Wheels[2].LateralSlip),GroundedWheels,Dynamics.Wheels[0].Drive+Dynamics.Wheels[1].Drive,Dynamics.Wheels[2].Drive+Dynamics.Wheels[3].Drive,Dynamics.Wheels[0].Grip);
}
