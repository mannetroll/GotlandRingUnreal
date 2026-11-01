#include "RingCar.h"
#include "ProceduralMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "RingDriverData.h"
void ARingCar::InitializeDriver(){
 DriverBody=NewObject<UProceduralMeshComponent>(this);DriverBody->SetupAttachment(Chassis);
 DriverBody->SetRelativeLocation(FVector(0,0,-48));DriverBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 DriverBody->RegisterComponent();AddInstanceComponent(DriverBody);
 TArray<FVector> Positions,Normals;TArray<FVector2D> UV;TArray<int32> Triangles;
 for(const auto& V:Ring::DriverVertices){Positions.Add(V.Position);Normals.Add(V.Normal);UV.Add(V.UV);}
 for(int I=0;I<UE_ARRAY_COUNT(Ring::DriverTriangles);I+=3){Triangles.Add(Ring::DriverTriangles[I]);Triangles.Add(Ring::DriverTriangles[I+2]);Triangles.Add(Ring::DriverTriangles[I+1]);}
 DriverBody->CreateMeshSection_LinearColor(0,Positions,Triangles,Normals,UV,{}, {},false);
 for(int32 I=0;I<Visuals.Num();I++)if(VisualMeshes[I]->GetName()==TEXT("Vehicle_12")){
  DriverBody->SetMaterial(0,Visuals[I]->GetMaterial(0));Visuals[I]->SetVisibility(false);
 }
 AnimateDriver(0);
}
void ARingCar::AnimateDriver(float RoadDegrees){
 if(!DriverBody)return;
 FQuat Rotations[7];FVector Origins[7],Targets[7];
 for(int I=0;I<7;I++){Rotations[I]=FQuat::Identity;Origins[I]=Targets[I]=FVector::ZeroVector;}
 const float WheelAngle=-RoadDegrees*12;
 for(int Side=0;Side<2;Side++){
  const FVector Shoulder=Ring::DriverJoints[Side*3],Elbow=Ring::DriverJoints[Side*3+1],Wrist=Ring::DriverJoints[Side*3+2];
  const float Offset=Side==0?20:-20,Cycle=FMath::FloorToFloat((WheelAngle-Offset)/150)*150;
  const float Release=FMath::Clamp((WheelAngle-Cycle-60-Offset)/30,0.f,1.f);
  const float Grip=WheelAngle-Cycle-150*(Release*Release*(3-2*Release));
  const FQuat Rotation(Ring::DriverAxis,FMath::DegreesToRadians(Grip));
  const FVector Hand=Ring::DriverCentre+Rotation.RotateVector(Wrist-Ring::DriverCentre)-Ring::DriverAxis*(3.5f*FMath::Square(FMath::Sin(Release*PI)));
  const FVector Reach=Hand-Shoulder,Direction=Reach.GetSafeNormal();const float Distance=Reach.Size();
  const float Upper=(Elbow-Shoulder).Size(),Lower=(Wrist-Elbow).Size();
  const float Along=(Upper*Upper-Lower*Lower+Distance*Distance)/(2*Distance);
  const FVector Pole=FVector::VectorPlaneProject(Elbow-Shoulder,(Wrist-Shoulder).GetSafeNormal());
  const FVector Bend=FVector::VectorPlaneProject(Pole,Direction).GetSafeNormal();
  const FVector NewElbow=Shoulder+Direction*Along+Bend*FMath::Sqrt(FMath::Max(0.f,Upper*Upper-Along*Along));
  const int B=Side*3+1;
  Origins[B]=Targets[B]=Shoulder;Rotations[B]=FQuat::FindBetweenVectors(Elbow-Shoulder,NewElbow-Shoulder);
  Origins[B+1]=Elbow;Targets[B+1]=NewElbow;Rotations[B+1]=FQuat::FindBetweenVectors(Wrist-Elbow,Hand-NewElbow);
  Origins[B+2]=Wrist;Targets[B+2]=Hand;Rotations[B+2]=Rotation;
 }
 TArray<FVector> Positions,Normals;TArray<FVector2D> UV;
 for(const auto& V:Ring::DriverVertices){
  FVector P=FVector::ZeroVector,N=FVector::ZeroVector;
  for(int I=0;I<4;I++){int B=V.Bones[I];P+=(Rotations[B].RotateVector(V.Position-Origins[B])+Targets[B])*V.Weights[I];N+=Rotations[B].RotateVector(V.Normal)*V.Weights[I];}
  Positions.Add(P);Normals.Add(N.GetSafeNormal());UV.Add(V.UV);
 }
 DriverBody->UpdateMeshSection_LinearColor(0,Positions,Normals,UV,{},{});
}
