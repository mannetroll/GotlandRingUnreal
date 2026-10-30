#pragma once
#include <algorithm>
#include <cmath>
// SI units. Port of SubaruEngine and SubaruAwdController; the contact solver
// replaces Unity WheelCollider with implicit rotating raycast tires.
namespace Ring {
constexpr float Pi=3.14159265358979323846f;
inline float Clamp(float x,float lo,float hi){return std::max(lo,std::min(hi,x));}
inline float Lerp(float a,float b,float t){return a+(b-a)*t;}
inline float Inverse(float a,float b,float x){return Clamp((x-a)/(b-a),0,1);}
inline float Move(float a,float b,float d){return a+Clamp(b-a,-d,d);}
inline float Sign(float x){return x>0?1.f:x<0?-1.f:0.f;}
constexpr float Radius=.3057f, Mass=1360, Efficiency=.88f, FinalDrive=4.111f;
constexpr float Ratios[]={3.454f,1.947f,1.366f,.972f,.738f};
inline float TorqueCurve(float rpm){
 constexpr float x[]={900,2000,3000,4000,5000,5600,6200,6800};
 constexpr float y[]={105,175,250,290,280,272.837f,230,175};
 if(rpm<=x[0])return y[0];
 for(int i=1;i<8;i++)if(rpm<x[i]){float t=(rpm-x[i-1])/(x[i]-x[i-1]);return Lerp(y[i-1],y[i],t*t*(3-2*t));}
 return y[7];
}
struct Engine {
 float Omega=900*(2*Pi/60),NetTorque=0;
 float Rpm()const{return Omega/(2*Pi/60);}
 float DriveTorque(float dt,float pedal,float shaftRpm,bool open){
  float drag=25*std::max(1.f,Rpm()/4000);
  float limiter=Clamp((6800-Rpm())/150,0,1);
  float combustion=(std::max(0.f,TorqueCurve(Rpm()))+drag)*pedal*limiter;
  float idle=Clamp((900-Rpm())*.4f+drag,0,100)*(1-pedal);
  NetTorque=combustion+idle-drag;
  if(open)return 0;
  float target=Lerp(900,4500,pedal);
  float capacity=Clamp(std::max(NetTorque*.15f,NetTorque+(Rpm()-target)*.16f),0,360);
  float slip=Omega-shaftRpm*(2*Pi/60);
  return Clamp((slip+NetTorque*dt/.28f)/(dt/.28f+1/20.f),0,capacity);
 }
 void Integrate(float dt,float reaction){Omega=std::max(0.f,Omega+(NetTorque-reaction)*dt/.28f);}
};
struct Wheel {
 float Omega=0,Rotation=0,Load=0,LongVelocity=0,LatVelocity=0,ForwardSlip=0,LateralSlip=0;
 float Drive=0,Brake=0,LongForce=0,LatForce=0,Grip=1,LateralBudget=1,SlidingGrip=.98f;
 bool Grounded=false,Asphalt=true;
};
inline float Friction(float slip,float peak,float tail,float tailValue){
 float x=std::abs(slip),v;
 if(x<peak){float t=x/peak;v=t*t*(3-2*t);}
 else if(x<tail){float t=(x-peak)/(tail-peak);v=Lerp(1,tailValue,t*t*(3-2*t));}
 else v=tailValue;
 return Sign(slip)*v;
}
struct Dynamics {
 Engine Motor;Wheel Wheels[4];int Gear=1,Shifts=0;
 float Throttle=0,ShiftDelay=0,ShiftCooldown=0,SteeringDegrees=0,ClutchTorque=0,Boost=0;
 bool TractionControl=true;
 float FrontShare=.5f,GripSetting=1;
 void Step(float dt,float pedal,float brake,float steering,float speed,float magnitude,bool handbrake){
  SteeringDegrees=Move(SteeringDegrees,Clamp(steering,-1,1)*Lerp(42,19,Clamp(magnitude/65,0,1)),90*dt);
  if(pedal==0||pedal*Throttle<0)Throttle=0;
  Throttle=Move(Throttle,Clamp(pedal,-1,1),dt*3);
  bool reverse=Throttle<0;float absSpeed=std::abs(speed);
  float frontRpm=(std::abs(Wheels[0].Omega)+std::abs(Wheels[1].Omega))*.5f*60/(2*Pi);
  float rearRpm=(std::abs(Wheels[2].Omega)+std::abs(Wheels[3].Omega))*.5f*60/(2*Pi);
  float driven=Lerp(rearRpm,frontRpm,FrontShare),road=absSpeed/Radius*60/(2*Pi);
  ShiftDelay=std::max(0.f,ShiftDelay-dt);ShiftCooldown=std::max(0.f,ShiftCooldown-dt);
  if(!reverse&&ShiftCooldown==0){
   int selected=Gear;float roadRpm=road*FinalDrive*Ratios[selected-1],coupled=driven*FinalDrive*Ratios[selected-1];
   float up=pedal>.7f&&std::abs(steering)>.35f?6700:6300;
   if(coupled>up&&roadRpm>up&&selected<5)selected++;
   else {float down=Lerp(2400,4200,brake>0?0:Inverse(.25f,.85f,pedal));
    while(selected>1&&coupled<down){float lower=FinalDrive*Ratios[selected-2],rpm=driven*lower;
     if(rpm>5900||road*lower>6600)break;selected--;coupled=rpm;}
   }
   if(selected!=Gear){Gear=selected;ShiftDelay=.22f;ShiftCooldown=.8f;Shifts++;}
  }
  float ratio=(reverse?3.333f:Ratios[Gear-1])*FinalDrive;
  bool open=brake>0||ShiftDelay>0||Throttle==0||(reverse&&absSpeed>8);
  float demand=brake>0?0:ShiftDelay>0?Clamp((driven*ratio-Motor.Rpm())/600,0,1):std::abs(Throttle);
  float clutch=Motor.DriveTorque(dt,demand,driven*ratio,open),direction=reverse?-1.f:1.f;
  float torque=clutch*ratio*Efficiency*direction;ClutchTorque=0;
  for(int i=0;i<4;i++){
   auto& w=Wheels[i];bool front=i<2;
   w.Drive=torque*(front?FrontShare:1-FrontShare)*.5f;
   w.Brake=brake*(front?1500:850)+(handbrake&&!front?1800:0);
   w.Grip=GripSetting*(pedal==0?2:1);w.LateralBudget=1;float use=0;
   if(w.Grounded){
    if(!w.Asphalt)w.Grip*=.48f;
    float slip=std::abs(w.ForwardSlip);
    if(TractionControl&&slip>.22f)w.Drive*=.22f/slip;
    if(absSpeed>3&&slip>.22f)w.Brake*=.22f/slip;
    use=Clamp((std::abs(w.Drive)+w.Brake)/(std::max(500.f,w.Load)*Radius*1.05f*w.Grip),0,.9f);
    w.LateralBudget=std::sqrt(1-use*use);
   }
   w.SlidingGrip=front?.98f:Lerp(.98f,.50f,w.Brake==0?Inverse(.05f,.15f,use):0);
   ClutchTorque+=w.Drive*direction/(ratio*Efficiency);
  }
  Motor.Integrate(dt,ClutchTorque);
  Boost=Move(Boost,std::abs(Throttle)*Inverse(1800,3600,Motor.Rpm()),dt*1.5f);
 }
 void SolveWheel(int i,float dt){
  auto& w=Wheels[i];constexpr float inertia=.5f*18*Radius*Radius;
  float denom=std::max(2.f,std::abs(w.LongVelocity));
  auto force=[&](float omega){return w.Grounded?Friction((omega*Radius-w.LongVelocity)/denom,.22f,.65f,.75f)*w.Load*1.05f*w.Grip:0.f;};
  // Implicit wheel inertia couples motor, road reaction, bearing drag and brakes.
  // Static brake torque can hold zero angular speed without crossing through it.
  float initial=w.Omega;
  auto residual=[&](float omega){return inertia*(omega-initial)/dt-w.Drive+force(omega)*Radius+.3f*omega;};
  float atZero=residual(0),omega=0;
  if(std::abs(atZero)>w.Brake){
   float direction=atZero<0?1.f:-1.f;
   float lo=direction>0?0:-2000,hi=direction>0?2000:0;
   for(int n=0;n<32;n++){float mid=(lo+hi)*.5f;if(residual(mid)+direction*w.Brake>0)hi=mid;else lo=mid;}
   omega=(lo+hi)*.5f;
  }
  w.Omega=omega;w.Rotation+=omega*dt;
  w.ForwardSlip=(omega*Radius-w.LongVelocity)/denom;
  w.LateralSlip=w.LatVelocity/std::max(1.f,std::abs(w.LongVelocity));
  w.LongForce=force(omega);
  float lateral=-Friction(w.LateralSlip,.20f,.60f,w.SlidingGrip)*w.Load*(i<2?1.05f:1.20f)*w.Grip*w.LateralBudget;
  // Prevent a low-speed contact force from reversing lateral velocity in one step.
  float stop=std::abs(w.LatVelocity)*Mass*.25f/dt;
  w.LatForce=w.Grounded?Clamp(lateral,-stop,stop):0;
 }
};
}
