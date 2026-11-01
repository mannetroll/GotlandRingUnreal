#include "RingDynamics.h"
#include "RingDrivingSettings.h"
#include "RingEngineMixer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <iomanip>
int main(int argc,char** argv){
 std::string root=argv[1];Ring::Engine engine;std::ifstream ref(root+"/engine_reference.csv");assert(ref.good());std::string line;std::getline(ref,line);
 float rpmError=0,torqueError=0;int rows=0;
 while(std::getline(ref,line)){
  for(auto& c:line)if(c==',')c=' ';std::istringstream in(line);int n,open;float pedal,shaft,t,rpm;in>>n>>pedal>>shaft>>open>>t>>rpm;
  float actual=engine.DriveTorque(.01f,pedal,shaft,open!=0);engine.Integrate(.01f,actual);
  rpmError=std::max(rpmError,std::abs(engine.Rpm()-rpm));torqueError=std::max(torqueError,std::abs(t-actual));rows++;
 }
 assert(rows==4000&&rpmError<.1f&&torqueError<.01f);
 Ring::EngineMixer mixer;std::ifstream audio(root+"/audio_reference.f32",std::ios::binary);assert(audio.good());

 float buffer[960],reference[960],peak=0,error=0,mutePeak=0;double errorEnergy=0,signalEnergy=0;
 for(int n=0;n<1200;n++){
  float rpm=n<200?900:n<800?900+(n-200)*9:6300-(n-800)*13.5f;
  mixer.Render(buffer,960,2,48000,rpm,n>=200&&n<800?1:0,n<200?0:25,0,n>=1100);
  audio.read(reinterpret_cast<char*>(reference),sizeof(reference));assert(audio.gcount()==sizeof(reference));

  for(int i=0;i<960;i++){assert(std::isfinite(buffer[i]));float d=buffer[i]-reference[i];error=std::max(error,std::abs(d));errorEnergy+=d*d;signalEnergy+=reference[i]*reference[i];peak=std::max(peak,std::abs(buffer[i]));if(n>1120)mutePeak=std::max(mutePeak,std::abs(buffer[i]));}
 }
 double relative=std::sqrt(errorEnergy/signalEnergy);
 std::cout<<std::setprecision(10)<<"engine_rows="<<rows<<" max_rpm_error="<<rpmError<<" max_torque_error_nm="<<torqueError<<" audio_peak="<<peak<<" audio_max_sample_error="<<error<<" audio_relative_rms_error="<<relative<<" muted_peak="<<mutePeak<<"\n";
 assert(peak<1&&relative<.002&&mutePeak==0);
 for(int rate:{44100,48000,96000}){Ring::EngineMixer m;for(int n=0;n<100;n++){m.Render(buffer,960,2,rate,6800,1,40,1,false);for(int i=0;i<960;i++)assert(std::isfinite(buffer[i])&&std::abs(buffer[i])<1);}}
 Ring::Dynamics d;for(auto& w:d.Wheels){w.Grounded=true;w.Load=Ring::Mass*9.81f/4;}
 float speed=0,maxSlip=0;int maxGear=1;
 for(int n=0;n<15000;n++){
  for(auto& w:d.Wheels){w.LongVelocity=speed;w.LatVelocity=0;}
  d.Step(.002f,1,0,0,speed,std::abs(speed),false);float force=0;
  for(int i=0;i<4;i++){d.SolveWheel(i,.002f);force+=d.Wheels[i].LongForce;maxSlip=std::max(maxSlip,std::abs(d.Wheels[i].ForwardSlip));}
  force-=speed*std::abs(speed)*.42f+Ring::Clamp(speed,-1,1)*.012f*Ring::Mass*9.81f;
  speed+=force/Ring::Mass*.002f;maxGear=std::max(maxGear,d.Gear);assert(std::isfinite(speed)&&d.Motor.Rpm()<7000);
 }
 std::cout<<"30s_speed_kmh="<<speed*3.6f<<" gear="<<d.Gear<<" shifts="<<d.Shifts<<" max_slip="<<maxSlip<<"\n";assert(maxGear>=3&&speed>30);
 float brakeStart=speed;
 for(int n=0;n<4000;n++){
  for(auto& w:d.Wheels)w.LongVelocity=speed;
  d.Step(.002f,0,1,0,speed,std::abs(speed),false);float f=0;for(int i=0;i<4;i++){d.SolveWheel(i,.002f);f+=d.Wheels[i].LongForce;assert(d.Wheels[i].Drive==0);}
  speed+=f/Ring::Mass*.002f;
 }
 std::cout<<"brake_start_kmh="<<brakeStart*3.6f<<" brake_end_kmh="<<speed*3.6f<<"\n";assert(std::abs(speed)<.5f);
 // AWD split, lift catch grip, off-road grip and rear power-slide curve.
 d=Ring::Dynamics();d.FrontShare=.6f;
 for(int n=0;n<1000;n++){for(auto& w:d.Wheels){w.Grounded=true;w.Load=3500;w.ForwardSlip=0;}d.Step(.002f,1,0,.5f,10,10,false);}
 assert(std::abs(d.Wheels[0].Drive/d.Wheels[2].Drive-1.5f)<.001f&&d.Wheels[2].SlidingGrip<.6f);
 d.Step(.002f,0,0,0,10,10,false);assert(d.Wheels[0].Drive==0&&d.Wheels[0].Grip==2&&d.Wheels[2].SlidingGrip==.98f);
 d.Wheels[0].Asphalt=false;d.Step(.002f,0,0,0,10,10,false);assert(std::abs(d.Wheels[0].Grip-.96f)<.001f);
 d=Ring::Dynamics();for(int n=0;n<1000;n++)d.Step(.002f,-1,0,0,0,0,false);
 for(auto& w:d.Wheels)assert(w.Drive<0);
 Ring::Dynamics assisted;assisted.Motor.Omega=4500*(2*Ring::Pi/60);assisted.Throttle=1;
 for(auto& w:assisted.Wheels){w.Grounded=true;w.Load=3500;w.ForwardSlip=1;}
 auto unassisted=assisted;unassisted.TractionControl=false;
 assisted.Step(.002f,1,0,0,15,15,false);unassisted.Step(.002f,1,0,0,15,15,false);
 assert(std::abs(assisted.Wheels[0].Drive/unassisted.Wheels[0].Drive-.22f)<.001f);
 assisted.Step(.002f,0,1,0,15,15,false);assert(std::abs(assisted.Wheels[0].Brake-330)<.001f);
 Ring::Dynamics down;down.Gear=5;for(auto& w:down.Wheels)w.Omega=10/Ring::Radius;
 down.Step(.002f,1,0,0,10,10,false);assert(down.Gear<4&&down.ShiftDelay>0&&down.ClutchTorque==0);
 assert(std::abs(Ring::Friction(.6f,.2f,.6f,.5f)-.5f)<.0001f);
 assert(std::abs(Ring::Friction(.6f,.2f,.6f,.98f)-.98f)<.0001f);
 Ring::DrivingSettings settings;settings.Steering=48;settings.HighSpeedSteering=22;settings.SteeringRate=120;
 settings.FrontTorque=.6f;settings.Grip=1.2f;settings.TractionControl=false;
 Ring::Dynamics configured;settings.Apply(configured);
 configured.Step(.1f,0,0,1,0,0,false);assert(std::abs(configured.SteeringDegrees-12)<.001f);
 for(int n=0;n<10;n++)configured.Step(.1f,0,0,1,0,0,false);
 assert(std::abs(configured.SteeringDegrees-48)<.001f);
 for(int n=0;n<10;n++)configured.Step(.1f,0,0,1,65,65,false);
 assert(std::abs(configured.SteeringDegrees-22)<.001f);
 assert(std::abs(configured.Wheels[0].Grip-2.4f)<.001f);
 settings.Grip=9;settings.FrontTorque=-1;settings.Steering=100;settings.HighSpeedSteering=0;settings.SteeringRate=999;settings.ClampValues();
 settings.Apply(configured);configured.SteeringDegrees=0;configured.Step(.1f,0,0,1,0,0,false);
 assert(configured.GripSetting==1.3f&&configured.FrontShare==.1f&&configured.LowSpeedSteering==55&&configured.HighSpeedSteering==8&&configured.SteeringDegrees==15);
 std::cout<<"PASS driving setup: steering rate, low/high-speed limits, grip and saved-value bounds\n";
 std::cout<<"PASS drivetrain, tire budgets, reverse, brake cut, audio rates and source parity\n";
}
