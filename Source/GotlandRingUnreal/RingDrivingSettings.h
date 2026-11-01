#pragma once
#include "RingDynamics.h"
namespace Ring {
// AWD fields and limits from Unity DrivingSettings. The arcade controller is separate.
struct DrivingSettings {
 float Grip=1,FrontTorque=.5f,Steering=42,HighSpeedSteering=19,SteeringRate=90;
 float DroneHeight=30,DroneDistance=40,CockpitMovement=.5f;
 bool TractionControl=true,TyreSqueal=false;
 void ClampValues(){
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
