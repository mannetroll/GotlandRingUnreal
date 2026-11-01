"""Compile Unity reference timing, source track viewpoints and driver skin data."""
from pathlib import Path
import json,csv,math
root=Path(__file__).resolve().parents[1];source=root.parent/'GotlandRingUnity';out=root/'Source/GotlandRingUnreal'
f=lambda v:format(float(v),'.9g')+('f' if '.' in format(float(v),'.9g') or 'e' in format(float(v),'.9g') else '.0f')
v=lambda p:'{'+','.join(f(x) for x in (p['z']*100,p['x']*100,p['y']*100))+'}'
data=json.loads((source/'Assets/Resources/Replay/ReferenceLap.json').read_text())
s='#pragma once\nnamespace Ring {\nstruct ReferenceFrame {float Distance,Speed,Rpm,Load;int Gear;};\n'
s+=f'constexpr float ReferenceFps={f(data["fps"])},ReferenceLapStart={f(data["lapStart"])},ReferenceLapDuration={f(data["lapDuration"])};\nconstexpr ReferenceFrame ReferenceFrames[]={{\n'
s+=''.join('{'+','.join([f(a[k]) for k in ['distance','speed','rpm','load']]+[str(a['gear'])])+'},\n' for a in data['frames'])+'};\n}\n'
(out/'RingReferenceData.h').write_text(s)
rig=json.loads((root/'Export/Vehicle/driver.json').read_text())
s='#pragma once\nnamespace Ring {\nstruct DriverVertex {FVector Position,Normal;FVector2D UV;int Bones[4];float Weights[4];};\n'
s+='const FVector DriverCentre='+v(rig['centre'])+',DriverAxis='+v({k:x/100 for k,x in rig['axis'].items()})+';\n'
s+='const FVector DriverJoints[]={'+','.join(v(p) for p in rig['joints'])+'};\n'
s+='const FVector CockpitAnchor='+v(rig['cockpit'])+',BonnetAnchor='+v(rig['bonnet'])+';\n'
s+='const DriverVertex DriverVertices[]={\n'
for a in rig['vertices']:
 s+='{'+v(a['p'])+','+v({k:x/100 for k,x in a['n'].items()})+',{'+f(a['uv']['x'])+','+f(1-a['uv']['y'])+'},{'+','.join(map(str,a['bones']))+'},{'+','.join(f(x) for x in a['weights'])+'}},\n'
s+='};\nconst int DriverTriangles[]={'+','.join(map(str,rig['triangles']))+'};\n}\n'
(out/'RingDriverData.h').write_text(s)
# UE (z,x,y) and Unity have opposite handedness, so preserve source triangle order.
print(len(data['frames']),len(rig['vertices']),len(rig['triangles'])//3,rig['cockpit'],rig['bonnet'])
rows=list(csv.DictReader((source/'Assets/Resources/Track/Surface.csv').open()));sites=list(csv.DictReader((source/'Assets/Resources/Track/WindTurbines.csv').open()))
pos=lambda x,y,z:'FVector('+','.join(f(a*100) for a in (z,x,y))+')'
views=[]
b=(-12.1,float(sites[2]['y_m'])+.1,125.7)
views.append(('Main building',tuple(b[i]+(53,31,-75)[i] for i in range(3)),(b[0],b[1]+3,b[2]),0))
views.append(('Main building / turbine 2',tuple(b[i]+(-20,240,-35)[i] for i in range(3)),(0,0,0),140))
for i,name in [(235,'Pit lane / catch fencing'),(444,'Nankang Hairpin / Armco'),(1835,'The Klaus Bowl / concrete barriers'),(278,'Gutemålrakan / start and finish'),(1240,'Havsörnen approach'),(1280,'Havsörnen / limestone quarry'),(2264,'Arho / blue and white kerbs'),(932,'Altarkarusellen'),(1883,'F.S. Krämertsskog')]:
 a=rows[i];n=rows[(i+8)%len(rows)];target=[float(a[k]) for k in ['x_m','y_m','z_m']];target[1]+=3
 forward=[float(n[k])-float(a[k]) for k in ['x_m','y_m','z_m']];length=math.sqrt(sum(x*x for x in forward));forward=[x/length for x in forward];length=math.hypot(forward[0],forward[2]);right=[forward[2]/length,0,-forward[0]/length]
 p=[target[k]-forward[k]*100+right[k]*70+(75 if k==1 else 0) for k in range(3)];views.append((name,p,target,0))
for i in [0,6,9]:
 a=sites[i];height=float(a['hub_height_m']);scale=float(a['rotor_diameter_m'])/66
 hub=[float(a['x_m'])-4*scale,float(a['hub_y_m']),float(a['z_m'])];d=[-170,-25,-45];p=[hub[k]+d[k]*height/78 for k in range(3)];target=[hub[0],hub[1]-height*.2,hub[2]]
 views.append(('Wind turbines / '+a['model'],p,target,0))
views.append(('Gotland Ring / all 12 wind turbines',(50,1500,-125),(0,0,0),980))
s='// Positions and framing from Unity RingDrive.SceneryViews.\n'
for name,p,t,size in views:s+='Add(TEXT('+json.dumps(name,ensure_ascii=False)+'),'+pos(*p)+','+pos(*t)+','+f(size*200)+');\n'
(out/'RingSceneryViews.inl').write_text(s)
import re
text=(source/'Assets/Scripts/TrackLandmarks.cs').read_text().split('KerbRuns={')[1].split('};')[0]
runs=re.findall(r'\("[^"]+",(\d+),(\d+),(-?\d+),',text)
(out/'RingKerbs.inl').write_text(''.join('{'+','.join(a)+'},\n' for a in runs))
