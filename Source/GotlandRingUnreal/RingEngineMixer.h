#pragma once
#include "RingDynamics.h"
#include "EngineCycles.h"
#include <vector>
#include <cstdint>
// Audio-thread-owned port of ImprezaEngineMixer. All loops stay phase-locked.
namespace Ring {
struct EngineMixer {
 std::vector<float> Loops[3];double Positions[3]={};float Correlation[2]={};
 float Rpm=900,Load=0,Speed=0,Volume=0,PreviousLoad=0,Lift=0;
 float EngineLow=0,EngineBody=0,AirLow=0,AirBody=0,DcInput=0,DcOutput=0;
 float Squeal=0,TyreFast=0,TyreSlow=0,TyreWander=0;
 double TyrePhase=0,TyreOvertone=0;
 uint32_t Random=1234567;
 EngineMixer(){
  auto copy=[](auto& out,const auto& data){for(auto v:data)out.push_back(v/32768.f);};
  copy(Loops[0],ImprezaLow);copy(Loops[1],ImprezaMid);copy(Loops[2],ImprezaHigh);
  double profiles[3][1024],energy[3]={};
  for(int n=0;n<3;n++){
   auto& samples=Loops[n];for(float s:samples)energy[n]+=s*s;energy[n]/=samples.size();
   for(int phase=0;phase<1024;phase++){double p=phase/1024.0*samples.size();int i=int(p),next=(i+1)%samples.size();profiles[n][phase]=samples[i]+(samples[next]-samples[i])*(p-i);}
  }
  for(int n=0;n<2;n++){double shared=0;for(int p=0;p<1024;p++)shared+=profiles[n][p]*profiles[n+1][p];Correlation[n]=float(shared/1024/std::sqrt(energy[n]*energy[n+1]));}
 }
 static float Follow(float value,float target,float c){return value+(target-value)*c;}
 void Render(float* data,int count,int channels,int rate,float targetRpm,float targetLoad,float targetSpeed,float targetSqueal,bool muted){
  float rpmSmooth=1-float(std::exp(-1.0/(rate*.045))),loadSmooth=1-float(std::exp(-1.0/(rate*.065)));
  float airSmooth=1-float(std::exp(-2*3.141592653589793*650/rate)),dcPole=float(std::exp(-2*3.141592653589793*18/rate));
  float liftDecay=float(std::exp(-1.0/(rate*.10)));
  float squealAttack=1-float(std::exp(-1.0/(rate*.035))),squealRelease=1-float(std::exp(-1.0/(rate*.16)));
  constexpr double pi=3.141592653589793;
  float tyreFastFilter=1-float(std::exp(-2*pi*3200/rate)),tyreSlowFilter=1-float(std::exp(-2*pi*900/rate));
  float tyreWanderFilter=1-float(std::exp(-2*pi*12/rate));targetSqueal=Clamp(targetSqueal,0,1);
  targetRpm=Clamp(targetRpm,700,8000);targetLoad=Clamp(targetLoad,0,1);targetSpeed=std::max(0.f,targetSpeed);
  if(PreviousLoad>.65f&&targetLoad<.15f)Lift=.035f*Clamp((Rpm-2500)/2500,0,1);PreviousLoad=targetLoad;
  float tone=1-float(std::exp(-2*3.141592653589793*(850+targetLoad*750)/rate));
  constexpr float refs[]={2530,3060,4500};
  for(int i=0;i<count;i+=channels){
   Rpm=Follow(Rpm,targetRpm,rpmSmooth);Load=Follow(Load,targetLoad,loadSmooth);Speed=Follow(Speed,targetSpeed,loadSmooth);
   Squeal=Follow(Squeal,targetSqueal,targetSqueal>Squeal?squealAttack:squealRelease);
   Volume=muted?std::max(0.f,Volume-1.f/(rate*.012f)):std::min(1.f,Volume+1.f/(rate*.025f));
   int lower=Rpm<refs[1]?0:1;float blend=Clamp((Rpm-refs[lower])/(refs[lower+1]-refs[lower]),0,1);
   float lowWeight=std::sqrt(1-blend),highWeight=std::sqrt(blend),blendGain=1/std::sqrt(1+2*lowWeight*highWeight*Correlation[lower]),engine=0;
   for(int n=0;n<3;n++){
    auto& samples=Loops[n];double p=Positions[n];int index=int(p),next=(index+1)%samples.size();
    float sample=samples[index]+(samples[next]-samples[index])*float(p-index);
    if(n==lower)engine+=sample*lowWeight;else if(n==lower+1)engine+=sample*highWeight;
    p+=double(samples.size())*Rpm/(120.0*rate);if(p>=samples.size())p-=samples.size();Positions[n]=p;
   }
   EngineLow=Follow(EngineLow,engine*blendGain,tone);EngineBody=Follow(EngineBody,EngineLow,tone);
   Random^=Random<<13;Random^=Random>>17;Random^=Random<<5;
   float white=Random/float(UINT32_MAX)*2-1;
   AirLow=Follow(AirLow,white,airSmooth);AirBody=Follow(AirBody,AirLow,airSmooth);Lift*=liftDecay;
   float rolling=Clamp(Speed/65,0,1),air=AirBody*(rolling*.045f+Lift);
   TyreFast=Follow(TyreFast,white,tyreFastFilter);TyreSlow=Follow(TyreSlow,white,tyreSlowFilter);
   TyreWander=Follow(TyreWander,white,tyreWanderFilter);
   float tyrePitch=1330+180*Squeal+70*rolling+TyreWander*700;
   TyrePhase+=2*pi*tyrePitch/rate;TyreOvertone+=2*pi*(tyrePitch*1.43+17)/rate;
   if(TyrePhase>=2*pi)TyrePhase-=2*pi;if(TyreOvertone>=2*pi)TyreOvertone-=2*pi;
   float tyreTone=float(std::sin(TyrePhase)+.3*std::sin(TyreOvertone))*.20f;
   float tyres=(tyreTone+(TyreFast-TyreSlow)*.65f)*Squeal*.30f*Clamp((Speed-3)/12,0,1);
   float gain=(.52f+.30f*Load)*(.8f+.2f*Clamp((Rpm-900)/4000,0,1));
   float value=EngineBody*gain+air+tyres,filtered=value-DcInput+dcPole*DcOutput;DcInput=value;DcOutput=filtered;
   value=float(std::tanh(double(filtered)))*Volume;
   for(int c=0;c<channels;c++)data[i+c]=value;
  }
 }
};
}
