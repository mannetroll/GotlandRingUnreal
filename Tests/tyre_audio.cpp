#include "RingEngineMixer.h"
#include <cassert>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 assert(argc==2);std::ifstream reference(argv[1],std::ios::binary);assert(reference.good());
 float worst=0,peak=0;double errorEnergy=0,signalEnergy=0;
 for(int rate:{44100,48000,96000}){
  Ring::EngineMixer mixer;float samples[960],expected[960];
  for(int n=0;n<400;n++){
   mixer.Render(samples,960,2,rate,3500,.6f,n<25?0:30,0,n>=350);
   reference.read(reinterpret_cast<char*>(expected),sizeof(expected));assert(reference.gcount()==sizeof(expected));
   for(int i=0;i<960;i++){
    assert(std::isfinite(samples[i])&&std::abs(samples[i])<1);
    float error=samples[i]-expected[i];worst=std::max(worst,std::abs(error));errorEnergy+=error*error;signalEnergy+=expected[i]*expected[i];
    peak=std::max(peak,std::abs(samples[i]));if(n>360)assert(samples[i]==0);
   }
  }
 }
 double relative=std::sqrt(errorEnergy/signalEnergy);assert(worst<.0001f&&relative<.0001);
 Ring::Dynamics car;
 for(int i=0;i<4;i++){auto& w=car.Wheels[i];w.Grounded=true;w.Asphalt=true;w.Load=Ring::Mass*9.81f*(i<2?.58f:.42f)*.5f;}
 assert(car.TyreSqueal()==0);
 for(auto& w:car.Wheels)w.LateralSlip=.2f;
 assert(std::abs(car.TyreSqueal()-1)<1e-6f);
 for(auto& w:car.Wheels)w.Load*=.25f;
 assert(std::abs(car.TyreSqueal()-.5f)<1e-6f);
 for(auto& w:car.Wheels)w.Grounded=false;
 assert(car.TyreSqueal()==0);
 for(auto& w:car.Wheels){w.Grounded=true;w.Asphalt=false;}
 assert(car.TyreSqueal()==0);
 for(auto& w:car.Wheels){w.Asphalt=true;w.LateralSlip=0;w.ForwardSlip=.22f;}
 assert(std::abs(car.TyreSqueal()-.5f)<1e-6f);
 std::cout<<"PASS source mixer parity at 44100/48000/96000 Hz; max_error="<<worst<<" relative_rms="<<relative<<" peak="<<peak<<"; mute, loaded slip, airborne and loose-surface gates\n";
}
