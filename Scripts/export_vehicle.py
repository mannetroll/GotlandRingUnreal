"""Export the existing licensed Subaru prefab from the isolated Unity staging project.
Run export_scenery.py first to prepare that project; the source Unity project is unchanged.
"""
import csv
import json
import math
import os
from pathlib import Path
import shutil
import subprocess

root=Path(__file__).resolve().parents[1]
source=root.parent/'GotlandRingUnity'
stage=root/'Export/UnitySceneryStage'
output=root/'Export/Vehicle'
output.mkdir(parents=True,exist_ok=True)
shutil.copyfile(root/'Scripts/Unity/ExportVehicle.cs',stage/'Assets/Editor/ExportVehicle.cs')
editor=source/'Tools/Unity/6000.3.25f1/Unity.app/Contents/MacOS/Unity'
subprocess.run([str(editor),'-batchmode','-nographics','-projectPath',str(stage),'-executeMethod','ExportVehicle.Run','-quit','-logFile',str(root/'Export/unity_vehicle.log')],env=dict(os.environ,GOTLAND_VEHICLE_OUTPUT=str(output)),check=True)
rows=list(csv.DictReader((source/'Assets/Resources/Track/Surface.csv').open()))
r=rows[284] # TrackLandmarks.StartFinishPoint
position=[float(r[a])*100 for a in ['z_m','x_m','y_m']]
position[2]+=50
(output/'spawn.json').write_text(json.dumps(dict(position=position,yaw=math.degrees(math.atan2(float(r['forward_x']),float(r['forward_z']))))))
probes=[]
for r in rows:
    for axes in [('z_m','x_m','y_m'),('left_edge_z_m','left_edge_x_m','left_edge_y_m'),('right_edge_z_m','right_edge_x_m','right_edge_y_m')]:
        probes.append([float(r[a])*100 for a in axes])
(root/'Validation/collision_probes.json').write_text(json.dumps(probes,separators=(',',':')))
