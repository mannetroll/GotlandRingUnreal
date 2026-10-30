#include "../Source/GotlandRingUnreal/RingRouteData.h"
#include <fstream>
#include <sstream>
#include <iostream>
int main(int argc,char**argv){Ring::Pilot pilot(Ring::RouteData,sizeof(Ring::RouteData)/sizeof(Ring::RouteRow));std::ifstream file(argv[1]);std::string line;std::getline(file,line);float worst[6]={};int count=0;
while(std::getline(file,line)){std::replace(line.begin(),line.end(),',',' ');std::istringstream in(line);float x,y,yaw,vx,vy,speed;int segment,road;float expected[6];in>>x>>y>>yaw>>vx>>vy>>speed>>segment>>road;for(float&v:expected)in>>v;pilot.Reset();auto c=pilot.Drive({x,y},yaw,{vx,vy},segment,road,.016666667f,speed);float actual[]={c.Steering,c.Throttle,c.Brake,c.Target,c.Error,c.Curvature};for(int i=0;i<6;i++)worst[i]=std::max(worst[i],std::abs(actual[i]-expected[i]));count++;}
bool pass=count==500&&worst[0]<.001&&worst[1]<.001&&worst[2]<.001&&worst[3]<.01&&worst[4]<.001&&worst[5]<.001;std::cout<<"cases="<<count<<" max_errors steering throttle brake target error curvature:";for(float v:worst)std::cout<<" "<<v;std::cout<<" passed="<<pass<<"\n";return pass?0:1;}
