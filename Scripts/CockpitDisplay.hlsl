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
  int divisor=places-column==4?1000:places-column==3?100:places-column==2?10:1;
  return digit(float2(frac(p.x)/.83,p.y),(value/divisor)%10,aa);
 }
 float glyph(float2 p,uint mask){
  if(any(p<0)||any(p>=1))return 0;
  int2 cell=(int2)floor(p*float2(3,5));
  return (mask>>(cell.x+(4-cell.y)*3))&1;
 }
 // Compact 3 x 5 letters keep the small physical LCD readable without a font atlas.
 float labelRPM(float2 p){return glyph(p,23275)+glyph(p-float2(1.3,0),4843)+glyph(p-float2(2.6,0),23549);}
 float labelKmh(float2 p){return glyph(p,23277)+glyph(p-float2(1.3,0),23549)+glyph(p-float2(2.6,0),4772)+glyph(p-float2(3.9,0),23533);}
float3 Draw(float2 UV,float _Rpm,float _Speed,float _Gear){
  float2 p=(float2(UV.x,1-UV.y)-float2(.03,.16))/float2(.96,.63);float aa=fwidth(p.y);
  float3 color=float3(.006,.012,.014);
  float3 white=float3(.78,.91,.88),orange=float3(1,.46,.06);
  float bar=box(p,float2(.5,.9),float2(.44,.06),aa);
  float cell=floor((p.x-.06)/.88*24);
  float gap=step(.11,frac((p.x-.06)/.88*24));
  float lit=step((cell+.5)/24,saturate(_Rpm/7000));
  float3 barColor=cell>=22?float3(1,.04,.015):cell>=20?orange:float3(.09,.8,.26);
  color+=bar*gap*lerp(float3(.022,.04,.033),barColor,lit);
  color+=white*labelRPM((p-float2(.07,.68))/float2(.033,.08));
  color+=white*number((p-float2(.48,.655))/float2(.115,.145),(int)floor(_Rpm+.5),4,aa/.145);
  color+=white*number((p-float2(.065,.215))/float2(.18,.32),(int)floor(_Speed+.5),3,aa/.32);
  color+=white*.8*labelKmh((p-float2(.18,.08))/float2(.032,.08));
  color+=float3(.04,.095,.10)*box(p,float2(.69,.35),float2(.003,.23),aa);
  float gear=_Gear<0?glyph((p-float2(.77,.225))/float2(.16,.30),23275):digit((p-float2(.755,.215))/float2(.19,.32),(int)_Gear,aa/.32);
  color+=orange*gear;
  return color;
 }
};
Display d;return d.Draw(UV,Rpm,Speed,Gear);
