// Analogue cluster port of Unity Visuals/CockpitInstruments.shader.
struct Display {
 float box(float2 p,float2 centre,float2 halfSize,float aa){
  float2 d=abs(p-centre)-halfSize;
  return 1-smoothstep(-aa,aa,max(d.x,d.y));
 }
 float digit(float2 p,int number,float aa){
  uint mask=0;
  switch(number){
   case 0:mask=63;break;case 1:mask=6;break;case 2:mask=91;break;case 3:mask=79;break;
   case 4:mask=102;break;case 5:mask=109;break;case 6:mask=125;break;case 7:mask=7;break;
   case 8:mask=127;break;case 9:mask=111;break;
  }
  float lit=0;
  lit+=((mask>>0)&1)*box(p,float2(.5,.93),float2(.31,.055),aa);
  lit+=((mask>>1)&1)*box(p,float2(.88,.72),float2(.065,.155),aa);
  lit+=((mask>>2)&1)*box(p,float2(.88,.28),float2(.065,.155),aa);
  lit+=((mask>>3)&1)*box(p,float2(.5,.07),float2(.31,.055),aa);
  lit+=((mask>>4)&1)*box(p,float2(.12,.28),float2(.065,.155),aa);
  lit+=((mask>>5)&1)*box(p,float2(.12,.72),float2(.065,.155),aa);
  lit+=((mask>>6)&1)*box(p,float2(.5,.5),float2(.31,.055),aa);
  return saturate(lit);
 }
 float number(float2 p,int value,int places,float aa){
  int column=(int)floor(p.x);
  if(column<0||column>=places)return 0;
  int divisor=1;
  for(int position=column+1;position<places;position++)divisor*=10;
  return digit(float2(frac(p.x)/.83,p.y),(value/divisor)%10,aa);
 }
 float glyph(float2 p,uint mask){
  if(any(p<0)||any(p>=1))return 0;
  int2 cell=(int2)floor(p*float2(3,5));
  return (mask>>(cell.x+(4-cell.y)*3))&1;
 }
 float needle(float2 p,float2 centre,float length,float angle,float aa){
  float2 direction=float2(cos(angle),sin(angle)),q=p-centre;
  float along=dot(q,direction),across=abs(dot(q,float2(-direction.y,direction.x)));
  float width=lerp(3.2,1,saturate(along/length));
  return (1-smoothstep(width-aa,width+aa,across))*smoothstep(-12-aa,-12+aa,along)*(1-smoothstep(length-aa,length+aa,along));
 }
 float disc(float2 p,float2 c,float radius,float aa){return 1-smoothstep(radius-aa,radius+aa,length(p-c));}
 float3 gauge(float3 color,float2 p,float2 c,float radius,float angle,float aa){
  float shadow=needle(p-float2(2,-3),c,radius,angle,aa);
  color=lerp(color,float3(.28,.29,.30),shadow*.35);
  color=lerp(color,float3(.93,.14,.08),needle(p,c,radius,angle,aa));
  color=lerp(color,float3(.10,.12,.14),disc(p,c,radius>70?19:14,aa));
  color=lerp(color,float3(.27,.29,.31),disc(p,c+float2(-1,1),radius>70?15:11,aa));
  return color;
 }
 float4 Draw(float2 UV,float4 Face,float _Rpm,float _Speed,float _Gear,float _Fuel,float _Coolant,float _TripKm){
  float2 p=float2(UV.x,1-UV.y)*float2(1200,400);float aa=max(fwidth(p.y),.6);
  float4 face=Face;float3 color=face.rgb;
  color=gauge(color,p,float2(898,169),112,radians(225-270*saturate(_Rpm/9000)),aa);
  color=gauge(color,p,float2(600,183),125,radians(225-270*saturate(_Speed/180)),aa);
  color=gauge(color,p,float2(230,181),43,radians(200-120*saturate(_Fuel)),aa);
  color=gauge(color,p,float2(350,181),43,radians(200-120*saturate(_Coolant)),aa);
  float odometer=number((p-float2(557,92))/float2(14,20),(int)floor(_TripKm*10),6,aa/20);
  color=lerp(color,float3(.16,.21,.14),max(odometer,disc(p,float2(625,94),1,aa)));
  // The warning strip carries a discreet low-fuel lamp and the selected gear.
  float gear=_Gear<0?glyph((p-float2(368,61))/float2(15,23),23275):digit((p-float2(366,61))/float2(17,24),(int)_Gear,aa/24);
  color=lerp(color,float3(.72,.76,.66),gear);
  if(_Fuel<.13334)color=lerp(color,float3(1,.49,.08),disc(p,float2(201,79),5,aa));
  return float4(color,face.a);
 }
};
Display d;return d.Draw(UV,Face,Rpm,Speed,Gear,Fuel,Coolant,TripKm);
