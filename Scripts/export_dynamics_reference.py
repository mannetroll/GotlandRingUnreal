"""Execute original Unity engine/audio classes in the isolated staging project."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess
root=Path(__file__).resolve().parents[1]
source=root.parent/'GotlandRingUnity'
stage=root/'Export/UnitySceneryStage'
output=root/'Validation/UnityReference'
output.mkdir(parents=True,exist_ok=True)
files=['Assets/Scripts/SubaruEngine.cs','Assets/Scripts/SubaruVehicleSetup.cs','Assets/Scripts/SubaruAwdController.cs','Assets/Scripts/ImprezaEngineMixer.cs','Assets/Scripts/BoxerAudio.cs']
provenance=[]
for name in files:
    p=source/name
    assert p.read_bytes()==(stage/name).read_bytes(),f'Staging source differs: {name}'
    provenance.append(dict(path=name,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
shutil.copyfile(root/'Scripts/Unity/ExportDynamics.cs',stage/'Assets/Editor/ExportDynamics.cs')
editor=source/'Tools/Unity/6000.3.25f1/Unity.app/Contents/MacOS/Unity'
subprocess.run([str(editor),'-batchmode','-nographics','-projectPath',str(stage),'-executeMethod','ExportDynamics.Run','-quit','-logFile',str(root/'Export/unity_dynamics.log')],env=dict(os.environ,GOTLAND_DYNAMICS_OUTPUT=str(output)),check=True)
(root/'Validation/dynamics_sources.json').write_text(json.dumps(provenance,indent=2)+'\n')
