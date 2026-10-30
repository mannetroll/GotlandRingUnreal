#pragma once
#include "RingDynamics.h"
#include <vector>
namespace Ring {
struct V2 {float X=0,Y=0;V2 operator+(V2 b)const{return {X+b.X,Y+b.Y};} V2 operator-(V2 b)const{return {X-b.X,Y-b.Y};} V2 operator*(float k)const{return {X*k,Y*k};} float Dot(V2 b)const{return X*b.X+Y*b.Y;} float Length()const{return std::sqrt(Dot(*this));} V2 Unit()const{float l=Length();return l>0?*this*(1/l):V2{};}};
struct RouteRow {float CX,CY,RX,RY,Left,Right,LX,LY,Z,Speed,Distance;};
struct PilotControls {float Steering=0,Throttle=0,Brake=0,Target=0,Error=0,Curvature=0;};
struct Pilot {
 const RouteRow* Rows;int Count;std::vector<float> Lengths;
 float Pedal=0,YawError=0;bool Primed=false;
 Pilot(const RouteRow* rows,int count):Rows(rows),Count(count){for(int i=0;i<Count;i++)Lengths.push_back((Line(Next(i))-Line(i)).Length());}
 int Next(int n)const{return (n+1)%Count;}
 V2 Line(int n)const{return {Rows[n].LX,Rows[n].LY};}
 V2 Center(int n)const{return {Rows[n].CX,Rows[n].CY};}
 void Reset(){Primed=false;YawError=0;}
 static float SteeringLimit(float speed){return Lerp(42,19,Clamp(std::abs(speed)/65,0,1));}
 int Nearest(V2 p,float& fraction,float& offset,bool& road)const{
  float best=1e30f;int index=0;fraction=0;
  for(int i=0;i<Count;i++){V2 d=Center(Next(i))-Center(i);float t=Clamp((p-Center(i)).Dot(d)/d.Dot(d),0,1);V2 error=p-(Center(i)+d*t);float sq=error.Dot(error);if(sq<best){best=sq;index=i;fraction=t;}}
  V2 side{Rows[index].RX,Rows[index].RY};offset=(p-Center(index)).Dot(side);road=offset>=-Rows[index].Left&&offset<=Rows[index].Right;return index;
 }
 V2 Ahead(int segment,float fraction,float distance)const{
  distance+=fraction*Lengths[segment];for(int n=0;n<Count&&distance>Lengths[segment];n++){distance-=Lengths[segment];segment=Next(segment);}
  return Line(segment)+(Line(Next(segment))-Line(segment))*(distance/Lengths[segment]);
 }
 PilotControls Drive(V2 position,float yaw,V2 velocity,int roadSegment,bool onRoad,float dt,float magnitude=-1){
  int nearest=roadSegment;float fraction=0,lineDistance=1e30f;
  for(int delta=-4;delta<=4;delta++){
   int i=(roadSegment+Count+delta)%Count;V2 segment=Line(Next(i))-Line(i);
   float along=Clamp((position-Line(i)).Dot(segment)/segment.Dot(segment),0,1);V2 error=position-(Line(i)+segment*along);float dist=error.Dot(error);
   if(dist<lineDistance){lineDistance=dist;nearest=i;fraction=along;}
  }
  float speed=magnitude>=0?magnitude:velocity.Length();float radians=yaw*Pi/180;V2 forward{std::cos(radians),std::sin(radians)};
  if(velocity.Dot(forward)>8)forward=velocity.Unit();
  float look=Clamp(6+speed*.45f,10,32);V2 aim=Ahead(nearest,fraction,look)-position;
  float angle=std::atan2(forward.X*aim.Y-forward.Y*aim.X,forward.Dot(aim));
  float curvature=2*std::sin(angle)/std::max(aim.Length(),1.f);
  float steering=std::atan(2.52f*curvature)*180/Pi/SteeringLimit(speed);
  if(std::abs(angle)>Pi*.5f)steering=Sign(angle);
  float target=Lerp(Rows[nearest].Speed,Rows[Next(nearest)].Speed,fraction);
  constexpr float response=90.f/42.f;
  float preview=speed*(.22f+.12f/response),remaining=(1-fraction)*Lengths[nearest];int p=nearest;
  for(int n=0;n<Count&&remaining<preview;n++){p=Next(p);target=std::min(target,Rows[p].Speed);remaining+=Lengths[p];}
  float heading=std::abs(angle)*180/Pi;if(heading>50)target=std::min(target,Lerp(12,4,Inverse(50,140,heading)));
  float error=std::sqrt(lineDistance);target*=Lerp(1,.45f,Inverse(1.5f,8,error));
  if(!onRoad)target=std::min(target,std::sqrt(3.5f/std::max(std::abs(curvature),.01f))*.8f);
  float planned=(Rows[Next(nearest)].Speed*Rows[Next(nearest)].Speed-Rows[nearest].Speed*Rows[nearest].Speed)/(2*Lengths[nearest]);
  float desired=(target-speed)*3+std::min(0.f,planned);
  float drag=.10f+speed*speed*.0012f+(onRoad?0:std::min(2.5f,speed*.4f));
  float demand=Clamp(desired+drag,-6.5f,.7f*5.9f);
  Pedal=Primed?Lerp(Pedal,demand,1-std::exp(-dt/.12f)):demand;Primed=true;
  float brake=Clamp((-Pedal-.2f)/6.5f,0,1),throttle=Clamp((Pedal-.2f)/(.7f*5.9f),0,1);
  if(velocity.Dot(forward)<-.3f){brake=1;throttle=0;}
  return {Clamp(steering,-1,1),throttle,brake,target,error,curvature};
 }
 void Correct(PilotControls& c,float forwardSpeed,float sideslip,float yawRate,float speed,float dt){
  if(forwardSpeed<8){YawError=0;return;}
  float error=c.Curvature*forwardSpeed-yawRate;
  YawError=Lerp(YawError,error,1-std::exp(-dt/.08f));
  float recovery=Inverse(8,10,std::abs(sideslip));
  float correction=sideslip*Lerp(1.05f,1.2f,recovery)+YawError*180/Pi*.3f;
  c.Steering=Clamp(c.Steering+correction/SteeringLimit(speed),-1,1);
  c.Throttle*=1-.8f*Inverse(7,12,std::abs(sideslip));c.Brake*=1-.8f*Inverse(6,10,std::abs(sideslip));
 }
};
}
