#include "RingSurfaceMixer.h"
#include <fstream>
#include <iostream>
#include <algorithm>
int main(int argc,char**argv){
 std::ifstream input(argv[1],std::ios::binary);float worst=0,peak=0;
 for(int rate:{44100,48000,96000}){
  Ring::SurfaceMixer mixer;
  for(int n=0;n<400;n++){
   float data[960]={},reference[960];input.read(reinterpret_cast<char*>(reference),sizeof(reference));if(!input)return 2;
   mixer.Render(data,960,2,rate,n<25?0:30,n<100?1:0,n>=100&&n<200?1:0,n>=200?1:0,n>=350);
   for(int i=0;i<960;i++){worst=std::max(worst,std::abs(data[i]-reference[i]));peak=std::max(peak,std::abs(data[i]));}
  }
 }
 std::cout<<"surface audio source comparison: max error="<<worst<<" peak="<<peak<<"\n";
 return worst<.000001f&&peak>.02f?0:1;
}
