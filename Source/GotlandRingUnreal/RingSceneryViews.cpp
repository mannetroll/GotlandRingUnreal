#include "RingCar.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/StaticMeshComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
void ARingCar::MakeSceneryViews(){
 auto Add=[this](const TCHAR* Name,FVector Position,FVector Target,float Size){SceneryViews.Add({Name,Position,Size>0?FRotator(-90,0,0):(Target-Position).Rotation(),Size});};
 #include "RingSceneryViews.inl"
}
void ARingCar::NextSceneryView(){
 if(SceneryView<0){if(SceneryViews.IsEmpty())MakeSceneryViews();SceneryWasPaused=DrivePaused;if(MenuWidget.IsValid())GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());SetDrivePaused(true);}
 if(++SceneryView==SceneryViews.Num()){ExitSceneryViews();return;}
 const auto& Shot=SceneryViews[SceneryView];Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
 Camera->SetWorldLocationAndRotation(Shot.Position,Shot.Rotation);Camera->SetFieldOfView(91.49285f);
 Camera->ProjectionMode=Shot.Size>0?ECameraProjectionMode::Orthographic:ECameraProjectionMode::Perspective;Camera->OrthoWidth=Shot.Size;
 for(int I=0;I<Visuals.Num();I++)if(VisualWheels[I]==-2)Visuals[I]->SetVisibility(true);
 if(auto* PC=Cast<APlayerController>(GetController())){PC->bShowMouseCursor=true;PC->bEnableClickEvents=true;PC->SetInputMode(FInputModeGameAndUI());FSlateApplication::Get().SetAllUserFocusToGameViewport();}
}
void ARingCar::ExitSceneryViews(){
 SceneryView=-1;Camera->ProjectionMode=ECameraProjectionMode::Perspective;Camera->AttachToComponent(Boom,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
 SetDrivePaused(SceneryWasPaused);UpdateView(1.f/60,nullptr);
 if(auto* PC=Cast<APlayerController>(GetController())){PC->bShowMouseCursor=DrivePaused;PC->bEnableClickEvents=true;PC->SetInputMode(FInputModeGameOnly());}
 if(MenuWidget.IsValid())BuildMenu();
}
