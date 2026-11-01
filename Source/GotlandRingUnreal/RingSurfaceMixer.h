#pragma once
#include "RingDynamics.h"
#include <cstdint>
namespace Ring {
struct SurfaceMixer {
 float Asphalt=0,Kerb=0,Loose=0,Speed=0,Volume=0,Low=0,GritLow=0,GritHigh=0;
 double Phase=0;uint32_t Random=927451;
 void Render(float* Data,int Count,int Channels,int Rate,float TargetSpeed,float Road,float PaintedKerb,float Ground,bool Muted){
  const float Smooth=1-float(std::exp(-1.0/(Rate*.06))),LowFilter=1-float(std::exp(-2*3.141592653589793*180/Rate));
  const float GritLowFilter=1-float(std::exp(-2*3.141592653589793*120/Rate)),GritHighFilter=1-float(std::exp(-2*3.141592653589793*1800/Rate));
  Road=Clamp(Road,0,1);PaintedKerb=Clamp(PaintedKerb,0,1);Ground=Clamp(Ground,0,1);
  for(int I=0;I<Count;I+=Channels){
   Asphalt+=(Road-Asphalt)*Smooth;Kerb+=(PaintedKerb-Kerb)*Smooth;Loose+=(Ground-Loose)*Smooth;Speed+=(std::max(0.f,TargetSpeed)-Speed)*Smooth;
   Volume=Muted?std::max(0.f,Volume-1.f/(Rate*.012f)):std::min(1.f,Volume+1.f/(Rate*.025f));
   Random^=Random<<13;Random^=Random>>17;Random^=Random<<5;float White=Random/float(UINT32_MAX)*2-1;
   Low+=(White-Low)*LowFilter;GritLow+=(White-GritLow)*GritLowFilter;GritHigh+=(White-GritHigh)*GritHighFilter;
   Phase+=2*3.141592653589793*(28+Speed*1.4)/Rate;if(Phase>=2*3.141592653589793)Phase-=2*3.141592653589793;
   float Rolling=Clamp((Speed-.5f)/25,0,1),Rumble=float(std::sin(Phase)+.3*std::sin(2*Phase));
   float Value=(Low*Asphalt*.075f+(Rumble*.15f+Low*.09f)*Kerb+(GritHigh-GritLow)*Loose*.18f)*Rolling*Volume;
   for(int C=0;C<Channels;C++)Data[I+C]+=Value;
  }
 }
};
}
