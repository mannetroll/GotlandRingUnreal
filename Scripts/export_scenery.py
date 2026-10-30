"""Run existing Unity scenery construction in an isolated copy; never edit the source."""
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
source = root.parent/'GotlandRingUnity'
stage = root/'Export/UnitySceneryStage'
stage.mkdir(parents=True, exist_ok=True)
for name in ('Assets', 'Packages', 'ProjectSettings'):
    shutil.copytree(source/name, stage/name, dirs_exist_ok=True)
# Invoke only scenery construction, before batching, cars, physics, or audio.
p = stage/'Assets/Scripts/RingDrive.cs'
s = p.read_text()
start = s.index(' void Awake(){')
end = s.index(' void MakeTrackSigns(){', start)
body = s[start:s.index('MakeLandscape();', start)+len('MakeLandscape();')]
p.write_text(s[:start]+body+'\n }\n'+s[end:])
p = stage/'Assets/Scripts/TrackForest.cs'
p.write_text(p.read_text().replace('mesh.UploadMeshData(true)', 'mesh.UploadMeshData(false)'))
shutil.copyfile(root/'Scripts/Unity/ExportScenery.cs', stage/'Assets/Editor/ExportScenery.cs')
editor = source/'Tools/Unity/6000.3.25f1/Unity.app/Contents/MacOS/Unity'
output = root/'Export/Scenery'
output.mkdir(exist_ok=True)
env = dict(os.environ, GOTLAND_SCENERY_OUTPUT=str(output))
subprocess.run([str(editor), '-batchmode', '-nographics', '-projectPath', str(stage), '-executeMethod', 'ExportScenery.Run', '-quit', '-logFile', str(root/'Export/unity_scenery.log')], env=env, check=True)
