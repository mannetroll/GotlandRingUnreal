"""Export existing road and sampled aprons; no synthetic surrounding terrain."""
import csv
import hashlib
import json
import math
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UNITY = ROOT.parent / 'GotlandRingUnity'
SOURCE = UNITY / 'Assets/Resources/Track/Surface.csv'

def build():
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    rows = list(csv.DictReader(SOURCE.open(encoding='utf-8-sig')))
    rows = [{k: float(v) for k, v in r.items()} for r in rows]
    out = ROOT / 'Export'
    out.mkdir(exist_ok=True)
    textures = out / 'Textures'
    textures.mkdir(exist_ok=True)
    for name in ('Asphalt', 'Gravel'):
        shutil.copyfile(UNITY / f'Assets/Resources/Visuals/{name}.jpg', textures / f'{name}.jpg')
    meshes = []
    max_edge_error = 0.0
    for name, columns in [('Road', range(17)), ('LeftApron', range(2)), ('RightApron', range(2))]:
        vertices, uv, faces = [], [], []
        for r in rows:
            fx, fz = r['forward_x'], r['forward_z']
            norm = math.hypot(fx, fz)
            rx, rz = fz / norm, -fx / norm
            for col in columns:
                if name == 'Road':
                    d = -r['width_left_m'] * (1-col/8) if col <= 8 else r['width_right_m']*(col-8)/8
                    extra = 0
                else:
                    side = -1 if name == 'LeftApron' else 1
                    width = r['width_left_m' if side < 0 else 'width_right_m']
                    d = side*(width+3*col)
                    extra = col*r['ground_apron_left_dy_m' if side < 0 else 'ground_apron_right_dy_m']
                road_d = d if name == 'Road' else side*width
                y = r['y_m'] + r['cross_slope_center_percent']/100*road_d + r['cross_curve_per_m']*road_d**2 + extra
                x, z = r['x_m']+rx*d, r['z_m']+rz*d
                # Unreal X=north, Y=east, Z=up, centimetres. Cyclic permutation preserves handedness.
                vertices.append([100*z,100*x,100*y])
                uv.append([d/4,r['distance_m']/4])
                if name == 'Road' and col in (0,16):
                    edge = 'left' if col == 0 else 'right'
                    max_edge_error = max(max_edge_error, math.dist((x,y,z), [r[f'{edge}_edge_{axis}_m'] for axis in 'xyz']))
        width = len(columns)
        for i in range(len(rows)-1):
            for j in range(width-1):
                a=i*width+j; b=a+width
                triangles = [(a,b,a+1),(a+1,b,b+1)]
                if name == 'LeftApron': triangles = [tuple(reversed(t)) for t in triangles]
                for tri in triangles:
                    p,q,s = [vertices[t] for t in tri]
                    nz=(q[0]-p[0])*(s[1]-p[1])-(q[1]-p[1])*(s[0]-p[0])
                    if nz <= 0: raise ValueError(f'Inverted or degenerate triangle: {name}')
                    faces.append(tri)
        path=out/f'{name}.obj'
        with path.open('w') as f:
            f.write('# OBJ right-handed: X=north, Y=west, Z=up, centimetres; UE importer reflects Y\n')
            for v in vertices: f.write('v '+' '.join(f'{x:.9f}' for x in (v[0],-v[1],v[2]))+'\n')
            for v in uv: f.write('vt '+' '.join(f'{x:.9f}' for x in v)+'\n')
            for tri in faces: f.write('f '+' '.join(f'{i+1}/{i+1}' for i in reversed(tri))+'\n')
        meshes.append(dict(name=name,vertices=vertices,uv=uv,triangles=faces))
    if max_edge_error > .002: raise ValueError('CSV edge mismatch exceeds 2 mm')
    for side, roadcol in [('LeftApron',0),('RightApron',16)]:
        apron=next(m for m in meshes if m['name']==side)
        assert all(apron['vertices'][i*2]==meshes[0]['vertices'][i*17+roadcol] for i in range(len(rows)))
    closure_gap = max(math.dist(m['vertices'][j], m['vertices'][-width+j])
        for m, width in zip(meshes, (17,2,2)) for j in range(width))
    if closure_gap > 1e-6: raise ValueError(f'Circuit closure gap: {closure_gap} cm')
    report=dict(source_sha256=digest,rows=len(rows),chainage_m=[rows[0]['distance_m'],rows[-1]['distance_m']],
        edge_reconstruction_max_m=max_edge_error,road_apron_seam_gap_m=0,circuit_closure_gap_cm=closure_gap,
        triangles=sum(len(m['triangles']) for m in meshes),
        bounds_cm=[[min(v[a] for m in meshes for v in m['vertices']),max(v[a] for m in meshes for v in m['vertices'])] for a in range(3)],
        origin=dict(latitude=57.835756925442,longitude=18.830856092125,height_RH2000_m=38.507350922,sweref99_e=727401.318349,sweref99_n=6416863.884251),
        transform='UE(X,Y,Z)=100*Unity(z,x,y); local AEQD, not SWEREF offsets',
        limitations='Terrain-derived road and 3 m aprons only. No DTM raster present. Export checks only; see Validation/import_validation.json for engine checks.')
    (out/'scene.json').write_text(json.dumps(dict(metadata=report,meshes=meshes),indent=2)+'\n')
    (out/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    assert digest == hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    print(json.dumps(report,indent=2))

if __name__ == '__main__': build()
