#include "RingCar.h"
#include "RingRouteData.h"
#include "RingTrainingData.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"

bool ARingCar::OnDrivingRoad(FVector P)const{
 if(Training)return FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y-600000))<=40000;
 FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(RingSurface),true,this);
 return CircuitRoad->LineTraceComponent(H,P+FVector(0,0,10000),P-FVector(0,0,10000),Q);
}
void ARingCar::MakeTrainingArea(){
 if(TrainingGround)return;
 const FVector Origin(0,600000,0);
 TrainingGround=GetWorld()->SpawnActor<AActor>();
 auto* Root=NewObject<USceneComponent>(TrainingGround);Root->RegisterComponent();TrainingGround->SetRootComponent(Root);TrainingGround->SetActorLocation(Origin);
 auto Mesh=[&](UStaticMesh* Asset,FVector Position,FVector Scale,int Material,bool Collision=false){
  auto* C=NewObject<UStaticMeshComponent>(TrainingGround);C->SetupAttachment(Root);C->SetStaticMesh(Asset);C->SetMaterial(0,TrainingMaterials[Material]);C->SetRelativeLocation(Position);C->SetRelativeScale3D(Scale);C->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));C->RegisterComponent();TrainingGround->AddInstanceComponent(C);return C;
 };
 Mesh(TrainingPlaneMesh,FVector(0,0,-2),FVector(3000,3000,1),1);
 TrainingRoad=Mesh(TrainingPlaneMesh,FVector::ZeroVector,FVector(800,800,1),0,true);
 auto* Ground=NewObject<UBoxComponent>(TrainingGround);Ground->SetupAttachment(Root);Ground->SetBoxExtent(FVector(150000,150000,50));Ground->SetRelativeLocation(FVector(0,0,-50));Ground->SetCollisionProfileName(TEXT("BlockAll"));Ground->RegisterComponent();TrainingGround->AddInstanceComponent(Ground);
 TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;
 auto Quad=[&](FVector A,FVector B,FVector C,FVector D){int I=V.Num();V.Append({A,B,C,D});for(int K=0;K<4;K++){N.Add(FVector::UpVector);UV.Add(FVector2D::ZeroVector);}T.Append({I,I+2,I+1,I,I+3,I+2});};
 for(float Radius:{25.f,50.f,100.f})for(int I=0;I<256;I++){
  float A=I*2*PI/256,B=(I+1)*2*PI/256;float R=Radius*100,S=R+18;
  Quad(FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,1.6),FVector(FMath::Cos(A)*S,FMath::Sin(A)*S,1.6),FVector(FMath::Cos(B)*S,FMath::Sin(B)*S,1.6),FVector(FMath::Cos(B)*R,FMath::Sin(B)*R,1.6));
 }
 auto Stripe=[&](FVector P,float X,float Y){Quad(P+FVector(-X,-Y,0),P+FVector(X,-Y,0),P+FVector(X,Y,0),P+FVector(-X,Y,0));};
 for(float Side:{-1.f,1.f}){Stripe(FVector(Side*39800,0,1.5),10,39800);Stripe(FVector(0,Side*39800,1.5),39800,10);}
 Stripe(FVector(-10000,0,1.8),600,20);
 auto* Lines=NewObject<UProceduralMeshComponent>(TrainingGround);Lines->SetupAttachment(Root);Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);Lines->RegisterComponent();TrainingGround->AddInstanceComponent(Lines);Lines->CreateMeshSection_LinearColor(0,V,T,N,UV,{}, {},false);Lines->SetMaterial(0,TrainingMaterials[2]);
 for(float X:{-10700.f,-9300.f}){Mesh(TrainingCubeMesh,FVector(X,0,3.5),FVector(.65,.65,.07),4);Mesh(TrainingCubeMesh,FVector(X,0,35),FVector(.25,.25,.65),3);}
 TrainingRoute.Append(Ring::TrainingAwd,UE_ARRAY_COUNT(Ring::TrainingAwd));
}
void ARingCar::ToggleTrainingArea(){
 MakeTrainingArea();const bool Paused=DrivePaused;SetDrivePaused(true);
 if(!Training)CircuitStartPose=StartPose;
 Training=!Training;RoadSurface=Training?TrainingRoad:CircuitRoad;
 Pilot=Training?MakeUnique<Ring::Pilot>(TrainingRoute.GetData(),TrainingRoute.Num()):MakeUnique<Ring::Pilot>(Ring::RouteData,UE_ARRAY_COUNT(Ring::RouteData));
 const float Best=Laps.Best;Laps=Ring::LapClock();Laps.Best=Best;LookYaw=LookPitch=ChaseYaw=MouseSteering=0;
 if(Training){StartPose=FTransform(FRotator(0,90,0),FVector(-10000,600000,60));RecoverTrainingCar();}else{StartPose=CircuitStartPose;ResetCar();RecoverCar();}
 SetDrivePaused(Paused);UpdateView(1.f/60,nullptr);
}
void ARingCar::RecoverTrainingCar(){
 const bool Paused=DrivePaused;
 Chassis->SetSimulatePhysics(false);SetActorTransform(StartPose,false,nullptr,ETeleportType::TeleportPhysics);
 PauseVelocity=PauseAngular=ArcadeVelocity=PreviousVelocity=ViewAcceleration=ViewLean=ViewTilt=FVector::ZeroVector;
 Dynamics=Ring::Dynamics();ApplyDrivingSettings();ArcadeRpm=900;ArcadeSteer=ArcadeBoost=0;ArcadeReverse=false;Laps.Restart();Laps.Discontinuity();Pilot->Reset();SpeedKmh=0;
 Chassis->SetSimulatePhysics(Settings.AwdMode&&!Paused);Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);Chassis->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
}
