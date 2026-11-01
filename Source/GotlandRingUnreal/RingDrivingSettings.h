#pragma once
#include "RingDynamics.h"
namespace Ring {
// Unity AWD and arcade handling settings.
struct DrivingSettings {
 float Grip=1,FrontTorque=.5f,Steering=42,HighSpeedSteering=19,SteeringRate=90;
 float SurfaceSound=.65f;
 float DroneHeight=30,DroneDistance=40,CockpitMovement=.5f;
 bool AwdMode=true,TractionControl=true,TyreSqueal=false;
 float ArcadeGrip=28,LateralGrip=38,Response=5,Acceleration=1,Braking=12,OffRoadGrip=8;
 void ClampValues(){
  SurfaceSound=Clamp(SurfaceSound,0,1);ArcadeGrip=Clamp(ArcadeGrip,10,40);LateralGrip=Clamp(LateralGrip,10,60);Response=Clamp(Response,1,9);Acceleration=Clamp(Acceleration,.5f,1.8f);Braking=Clamp(Braking,6,20);OffRoadGrip=Clamp(OffRoadGrip,3,12);
  DroneHeight=Clamp(DroneHeight,5,60);DroneDistance=Clamp(DroneDistance,5,120);CockpitMovement=Clamp(CockpitMovement,0,1);
  Grip=Clamp(Grip,.7f,1.3f);FrontTorque=Clamp(FrontTorque,.1f,.9f);
  Steering=Clamp(Steering,25,55);HighSpeedSteering=Clamp(HighSpeedSteering,8,25);SteeringRate=Clamp(SteeringRate,40,150);
 }
 void Apply(Dynamics& d)const{
  d.GripSetting=Grip;d.FrontShare=FrontTorque;d.LowSpeedSteering=Steering;
  d.HighSpeedSteering=HighSpeedSteering;d.SteeringRate=SteeringRate;d.TractionControl=TractionControl;
 }
};
}
