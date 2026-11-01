#include "RingCar.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
void ARingCar::InitializeCarVisuals(){
 for(auto V:Visuals)V->DestroyComponent();Visuals.Empty();Dashboard=nullptr;PaintMaterial=nullptr;
 auto& Model=Models[SelectedCar];VisualMeshes=Model.Meshes;VisualWheels=Model.Wheels;WheelCenters=Model.Centers;
 for(int I=0;I<VisualMeshes.Num();I++){
  auto* V=NewObject<UStaticMeshComponent>(this);V->SetupAttachment(Chassis);V->SetStaticMesh(VisualMeshes[I]);
  float Trip;if(V->GetMaterial(0)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("TripKm")),Trip))Dashboard=V->CreateDynamicMaterialInstance(0);
  if(I==Model.PaintSlot){PaintMaterial=V->CreateDynamicMaterialInstance(0);PaintMaterial->SetVectorParameterValue(TEXT("BodyPaint"),Model.PaintColors[PaintSelections[SelectedCar]]);}
  V->SetCollisionEnabled(ECollisionEnabled::NoCollision);V->RegisterComponent();AddInstanceComponent(V);Visuals.Add(V);
  V->SetRelativeLocation((VisualWheels[I]==-3?Model.SteeringPivot:(VisualWheels[I]<0?FVector::ZeroVector:WheelCenters[VisualWheels[I]]))-FVector(0,0,48));
  if(VisualWheels[I]==-4){V->SetVisibility(false);if(DriverBody)DriverBody->SetMaterial(0,V->GetMaterial(0));}
 }
}
void ARingCar::SwitchCarModel(){
 ViewAcceleration=ViewLean=ViewTilt=FVector::ZeroVector;RoadMix=LooseMix=0;
 ModelTrips[SelectedCar]=TripKm;SelectedCar=(SelectedCar+1)%Models.Num();TripKm=ModelTrips[SelectedCar];InitializeCarVisuals();AnimateCockpitSteering(Dynamics.SteeringDegrees);AnimateDriver(Dynamics.SteeringDegrees);UpdateInstruments(Dynamics.Motor.Rpm(),SpeedKmh/3.6f,Dynamics.Throttle<0?-1:Dynamics.Gear);UpdateView(1.f/60,nullptr);
 if(auto* PC=Cast<APlayerController>(GetController()))PC->PlayerCameraManager->SetGameCameraCutThisFrame();
}
void ARingCar::CyclePaint(){
 const auto& Model=Models[SelectedCar];if(Model.PaintColors.Num()==1)return;auto& Index=PaintSelections[SelectedCar];Index=(Index+1)%Model.PaintColors.Num();PaintMaterial->SetVectorParameterValue(TEXT("BodyPaint"),Model.PaintColors[Index]);
 if(auto* PC=Cast<APlayerController>(GetController()))PC->PlayerCameraManager->SetGameCameraCutThisFrame();
}
FString ARingCar::PaintName()const{return Models[SelectedCar].PaintNames[PaintSelections[SelectedCar]];}
