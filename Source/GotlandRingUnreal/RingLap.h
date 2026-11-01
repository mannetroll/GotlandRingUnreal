#pragma once
namespace Ring {
// Ordered quarter-lap gates prevent reverse crossings and recovery jumps awarding laps.
struct LapClock {
 int Lap=1, Checkpoints=0, Previous=-1;
 float Seconds=0,Best=0;
 void Restart(){Checkpoints=0;Previous=-1;Seconds=0;}
 void Discontinuity(){Checkpoints=0;Previous=-1;}
 void Step(float dt,int nearest,int start,int count,bool onRoad){
  Seconds+=dt;
  if(!onRoad){Previous=-1;return;}
  if(Previous>=0){
   int before=(Previous-start+count)%count,after=(nearest-start+count)%count;
   int advance=(nearest-Previous+count)%count;
   if(advance>0&&advance<count/8){
    int gate=(Checkpoints+1)*(count/4);
    if(Checkpoints<3&&before<gate&&after>=gate)Checkpoints++;
    if(before>after&&Checkpoints==3){if(Best==0||Seconds<Best)Best=Seconds;Lap++;Seconds=0;Checkpoints=0;}
   }else if(advance>=count/8&&advance<count-count/8)Checkpoints=0;
  }
  Previous=nearest;
 }
};
}
