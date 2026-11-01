#include "RingCar.h"
#include "RingAudio.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
void ARingCar::ToggleHandling(){
 Settings.AwdMode=!Settings.AwdMode;
 Chassis->SetSimulatePhysics(Settings.AwdMode&&!DrivePaused);
 Laps=Ring::LapClock();ResetCar();SaveDrivingSettings(Settings);
}
void ARingCar::StepArcade(float Dt,float Throttle,float Brake,float Steering,bool Reverse){
 FVector Forward=GetActorForwardVector();Forward.Z=0;Forward.Normalize();FVector Right=FVector::CrossProduct(FVector::UpVector,Forward);
 float Speed=ArcadeVelocity.Size(),Long=FVector::DotProduct(ArcadeVelocity,Forward),Lat=FVector::DotProduct(ArcadeVelocity,Right);
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(RingArcade),true,this);const FVector P=GetActorLocation();
 bool Road=RoadSurface->LineTraceComponent(Hit,P+FVector(0,0,10000),P-FVector(0,0,10000),Q);
 FVector Normal=Road?Hit.Normal:FVector::UpVector;
 ArcadeSteer=FMath::FInterpConstantTo(ArcadeSteer,Steering,Dt,Settings.Response);
 static const float Ratios[]={3.45f,1.95f,1.37f,1.03f,.78f};
 float Target=FMath::Max(900.f,FMath::Abs(Long)/(.32f*2*PI)*60*Ratios[Dynamics.Gear-1]*4.11f);
 if(Target>6400&&Dynamics.Gear<5){Dynamics.Gear++;Target*=.72f;}else if(Target<2200&&Dynamics.Gear>1){Dynamics.Gear--;Target*=1.3f;}
 ArcadeRpm=FMath::Lerp(ArcadeRpm,Target+Throttle*350,FMath::Min(1.f,Dt*8));ArcadeBoost=FMath::FInterpConstantTo(ArcadeBoost,Throttle*FMath::Clamp((ArcadeRpm-2100)/1900,0.f,1.f),Dt,.7f);
 if(Reverse&&Throttle<=0){Throttle=1;if(Long>.3f){Brake=1;Throttle=0;}else ArcadeReverse=true;}else if(Throttle>0||Long>=-.3f)ArcadeReverse=false;
 float Drive=Throttle*Settings.Acceleration*(ArcadeReverse?-3:2.6f+ArcadeBoost*3.3f)*FMath::Clamp(((ArcadeReverse?8:72)-Speed)/(ArcadeReverse?2.f:15.f),0.f,1.f);
 float Drag=.10f+Speed*Speed*.0012f+(Road?0:FMath::Min(2.5f,Speed*.4f));Long=FMath::FInterpConstantTo(Long,0,Dt,Drag+Brake*Settings.Braking)+Drive*Dt;
 float Scale=Throttle==0?2:1,Angle=ArcadeSteer*Ring::Pilot::SteeringLimit(Speed),YawRate=Long/2.52f*FMath::Tan(FMath::DegreesToRadians(Angle));
 float Grip=(Road?Settings.ArcadeGrip:Settings.OffRoadGrip)*Scale,Bank=FVector::DotProduct(FVector::VectorPlaneProject(FVector(0,0,-9.81f),Normal),Right);
 float TravelBank=Long<0?-Bank:Bank;YawRate=FMath::Clamp(YawRate,(TravelBank-Grip)/FMath::Max(Speed,3.f),(TravelBank+Grip)/FMath::Max(Speed,3.f));
 Lat=FMath::FInterpConstantTo(Lat,0,Dt,(Road?Settings.LateralGrip:Settings.OffRoadGrip)*Scale);
 float Yaw=GetActorRotation().Yaw+FMath::RadiansToDegrees(YawRate)*Dt;Forward=FRotator(0,Yaw,0).Vector();Right=FVector::CrossProduct(FVector::UpVector,Forward);ArcadeVelocity=Forward*Long+Right*Lat;
 FVector Position=P+ArcadeVelocity*(100*Dt);if(GetWorld()->LineTraceSingleByChannel(Hit,Position+FVector(0,0,10000),Position-FVector(0,0,10000),ECC_Visibility,Q)){Position.Z=Hit.ImpactPoint.Z+48;Normal=Hit.Normal;}
 SetActorLocationAndRotation(Position,FRotationMatrix::MakeFromXZ(Forward,Normal).Rotator());
 SpeedKmh=ArcadeVelocity.Size()*3.6f;Dynamics.SteeringDegrees=Angle;Dynamics.Throttle=ArcadeReverse?-Throttle:Throttle;Dynamics.Boost=ArcadeBoost;
 TripKm+=ArcadeVelocity.Size()*Dt/1000;UpdateInstruments(ArcadeRpm,ArcadeVelocity.Size(),ArcadeReverse?-1:Dynamics.Gear);AnimateCockpitSteering(Angle);
 for(int I=0;I<Visuals.Num();I++)if(VisualWheels[I]>=0){int W=VisualWheels[I];Dynamics.Wheels[W].Rotation+=Long*Dt/.33865723f;Visuals[I]->SetRelativeRotation(FRotator(0,W<2?Angle:0,0).Quaternion()*FRotator(-FMath::RadiansToDegrees(Dynamics.Wheels[W].Rotation),0,0).Quaternion());}
 const FVector Contacts=SurfaceContacts()*Settings.SurfaceSound;EngineAudio->SetSurface(Contacts.X,Contacts.Y,Contacts.Z);
 EngineAudio->SetEngine(ArcadeRpm,Throttle,ArcadeVelocity.Size(),0,Muted);
}
