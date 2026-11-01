"""Export Unity's banking map and existing sign anchors without changing Unity."""
import argparse, csv, hashlib, json, math, re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--unity',type=Path,default=root.parent/'GotlandRingUnity')
p.add_argument('--scenery',type=Path,required=True)
a=p.parse_args()
surface=a.unity/'Assets/Resources/Track/Surface.csv'
landmarks=a.unity/'Assets/Scripts/TrackLandmarks.cs'
rows=list(csv.DictReader(surface.open(encoding='utf-8-sig')))
start=int(re.search(r'StartFinishPoint=(\d+)',landmarks.read_text()).group(1))
names=[s.replace('\\n','\n') for _,s in re.findall(r'\((\d+), "([^"]+)"\)',landmarks.read_text().split('All =')[1])]
labels=json.loads(a.scenery.read_text())['labels']
labels=[s for s in labels if s['text'] in names or re.fullmatch(r'\d{2}',s['text'])]
assert len(labels)==2*len(names)==84
banks=[math.degrees(math.atan(float(r['cross_slope_center_percent'])/100+float(r['cross_curve_per_m'])*(float(r['width_right_m'])-float(r['width_left_m'])))) for r in rows[:-1]]
def lit(v):return f'{float(v):.9f}f'
text='#pragma once\nnamespace Ring {\n'
text+=f'constexpr int CircuitStart={start};\nconstexpr float CircuitLength={lit(rows[-1]["distance_m"])};\n'
text+='constexpr float CircuitBanking[]={\n'+',\n'.join(','.join(map(lit,banks[i:i+12])) for i in range(0,len(banks),12))+'\n};\n'
text+='struct SignLayout {const TCHAR* Text; float X,Y,Z,Width,Height;};\nconstexpr SignLayout SignLayouts[]={\n'
for s in labels:
    number=s['text'].isdigit()
    text+='{TEXT('+json.dumps(s['text'],ensure_ascii=False)+'),'+','.join(map(lit,[*s['position'],118 if number else 630,120 if number else 170 if '\n' in s['text'] else 85]))+'},\n'
text+='};\n}\n'
(root/'Source/GotlandRingUnreal/RingPresentationData.h').write_text(text)
print(json.dumps(dict(sections=len(banks),sign_labels=len(labels),start=start,bank_limit=max(map(abs,banks)),length=float(rows[-1]['distance_m']),surface_sha256=hashlib.sha256(surface.read_bytes()).hexdigest()),indent=2))
