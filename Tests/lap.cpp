#include "RingLap.h"
#include <cassert>
int main(){
 Ring::LapClock clock;int n=2405,start=284;
 for(int i=0;i<=n;i++)clock.Step(.05f,(start+i)%n,start,n,true);
 assert(clock.Lap==2&&clock.Best>120&&clock.Best<121&&clock.Checkpoints==0);
 clock.Restart();for(int i=0;i<n;i++)clock.Step(.05f,(start-i+n)%n,start,n,true);assert(clock.Lap==2);
 clock.Restart();clock.Step(1,start,start,n,true);clock.Step(1,(start+n-1)%n,start,n,true);clock.Step(1,start,start,n,true);assert(clock.Lap==2);
 clock.Restart();for(int i=0;i<n*3/4+1;i++)clock.Step(.05f,(start+i)%n,start,n,true);clock.Discontinuity();clock.Step(1,(start+n-1)%n,start,n,true);clock.Step(1,start,start,n,true);assert(clock.Lap==2);
 clock.Restart();assert(clock.Seconds==0&&clock.Best>0);
}
