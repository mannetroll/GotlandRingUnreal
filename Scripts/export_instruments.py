"""Export Unity's serialized four-vertex cluster and prefab transform, without opening Unity."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct


def export(source, output):
    prefab = source / 'Assets/Resources/SubaruImpreza.prefab'
    mesh = source / 'Assets/Resources/Visuals/PlesseClusterMesh.asset'
    face = source / 'Assets/Resources/Visuals/PlesseCluster.png'
    documents = {int(key): body for key, body in re.findall(r'--- !u!\d+ &(\d+)\n(.*?)(?=--- !u!|\Z)', prefab.read_text(), re.S)}
    game_object = next(key for key, body in documents.items() if 'm_Name: Plesse instrument cluster\n' in body)
    transform = next(key for key, body in documents.items() if body.startswith('Transform:') and f'm_GameObject: {{fileID: {game_object}}}' in body)
    def vector(body, key):
        return [float(v) for v in re.search(key + r': \{([^}]+)\}', body)[1].replace(' ', '').split(',') for v in [v.split(':')[1]]]
    def world(point):
        node = transform
        while node:
            body = documents[node]
            scale = vector(body, 'm_LocalScale')
            position = vector(body, 'm_LocalPosition')
            x, y, z, w = vector(body, 'm_LocalRotation')
            qnorm = math.sqrt(x*x+y*y+z*z+w*w)
            x,y,z,w = (v/qnorm for v in (x,y,z,w))
            a,b,c = (point[i]*scale[i] for i in range(3))
            point = [(1-2*(y*y+z*z))*a+2*(x*y-z*w)*b+2*(x*z+y*w)*c+position[0],
                     2*(x*y+z*w)*a+(1-2*(x*x+z*z))*b+2*(y*z-x*w)*c+position[1],
                     2*(x*z-y*w)*a+2*(y*z+x*w)*b+(1-2*(x*x+y*y))*c+position[2]]
            node = int(re.search(r'm_Father: \{fileID: (\d+)\}', body)[1])
        return point
    data = mesh.read_text()
    assert 'm_VertexCount: 4' in data and 'm_DataSize: 128' in data
    values = struct.unpack('<32f', bytes.fromhex(re.search(r'_typelessdata: (\w+)', data)[1]))
    points = [world(values[i:i+3]) for i in range(0,32,8)]
    uv = [values[i+6:i+8] for i in range(0,32,8)]
    width = math.dist(points[0],points[1]); height = math.dist(points[1],points[2])
    assert abs(width-.46)<.0001 and abs(height-.1533334)<.0001, (width,height)
    output.mkdir(parents=True,exist_ok=True)
    # Match the existing OBJ bridge: Unreal's OBJ reader reverses the OBJ Y axis.
    vertices = [(p[2]*100,-p[0]*100,p[1]*100) for p in points]
    lines = ['v '+' '.join(format(v,'.9g') for v in p) for p in vertices]
    lines += ['vt '+' '.join(format(v,'.9g') for v in p) for p in uv]
    indices = struct.unpack('<6H', bytes.fromhex(re.search(r'm_IndexBuffer: (\w+)',data)[1]))
    lines += ['f '+' '.join(f'{n+1}/{n+1}' for n in indices[i:i+3][::-1]) for i in (0,3)]
    (output/'InstrumentCluster.obj').write_text('\n'.join(lines)+'\n')
    shutil.copyfile(face, output/face.name)
    report = dict(width_m=width,height_m=height,unity_vertices_m=points,
                  unreal_vertices_cm=[[p[2]*100,p[0]*100,p[1]*100] for p in points],
                  source_sha256={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (prefab,mesh,face)})
    (output/'instruments.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__ == '__main__':
    root=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser()
    parser.add_argument('--unity-source',type=Path,default=root.parent/'GotlandRingUnity')
    parser.add_argument('--output',type=Path,default=root/'Export/Instruments')
    args=parser.parse_args()
    export(args.unity_source,args.output)
