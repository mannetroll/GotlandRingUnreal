"""Separate the existing Unity rotor geometry from its combined paint mesh."""
import csv,json,math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root/'Export/Scenery'
manifest=json.loads((source/'scenery.json').read_text())
item=next(x for x in manifest['meshes'] if x['material']=='Paint')
lines=(source/(item['name']+'.obj')).read_text().splitlines()
v=[list(map(float,x.split()[1:])) for x in lines if x.startswith('v ')]
uv=[x for x in lines if x.startswith('vt ')]
n=[x for x in lines if x.startswith('vn ')]
faces=[[int(t.split('/')[0])-1 for t in x.split()[1:]] for x in lines if x.startswith('f ')]
sites=list(csv.DictReader((root.parent/'GotlandRingUnity/Assets/Resources/Track/WindTurbines.csv').open()))
parent=list(range(len(v)))
def find(j):
 while parent[j]!=j:parent[j]=parent[parent[j]];j=parent[j]
 return j
for f in faces:
 for j in f[1:]:parent[find(j)]=find(f[0])
owner=[-1]*len(v); rotors=[]
for i,s in enumerate(sites):
 size=float(s['rotor_diameter_m'])/66
 hub=[float(s['z_m'])*100,-(float(s['x_m'])-4*size)*100,float(s['hub_y_m'])*100]
 radius=float(s['rotor_diameter_m'])*50
 for j,p in enumerate(v):
  radial=math.hypot(p[0]-hub[0],p[2]-hub[2])
  if math.dist(p,hub)<151*size or (300*size<radial<=radius+.1 and abs(p[1]-hub[1])<radius*.006+.1):
   assert owner[j]==-1;owner[j]=i
 rotors.append(dict(id=s['turbine_id'],hub=[hub[0],-hub[1],hub[2]],obj_hub=hub))
components={find(j):o for j,o in enumerate(owner) if o>=0}
owner=[components.get(find(j),-1) for j in range(len(v))]
groups=[[] for _ in range(13)]
for f in faces:
 ids={owner[j] for j in f}
 assert len(ids)==1,('Rotor boundary crosses a triangle',ids,[v[j] for j in f])
 groups[next(iter(ids))+1].append(f)
output=root/'Export/WindRotors';output.mkdir(parents=True,exist_ok=True)
def write(name,fs,hub):
 ids=sorted({j for f in fs for j in f});remap={j:k+1 for k,j in enumerate(ids)}
 with (output/(name+'.obj')).open('w') as out:
  for j in ids:out.write('v '+' '.join(str(v[j][k]-hub[k]) for k in range(3))+'\n')
  for j in ids:out.write(uv[j]+'\n')
  for j in ids:out.write(n[j]+'\n')
  for f in fs:out.write('f '+' '.join(f'{remap[j]}/{remap[j]}/{remap[j]}' for j in f)+'\n')
 return len(fs)
write(item['name'],groups[0],[0,0,0])
for i,r in enumerate(rotors):
 r['mesh']=f'Rotor_{i:02}';r['triangles']=write(r['mesh'],groups[i+1],r.pop('obj_hub'))
 assert r['triangles']==852, r  # Three 28-triangle blades and the 768-triangle Unity hub.
(output/'rotors.json').write_text(json.dumps(dict(paint_mesh=item['name'],rpm=10,rotors=rotors),indent=2))
print([(r['id'],r['triangles']) for r in rotors])
