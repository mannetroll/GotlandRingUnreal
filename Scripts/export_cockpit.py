"""Export source camera anchors, racing line and controller reference cases from the Unity staging copy."""
import os
from pathlib import Path
import shutil
import subprocess
root=Path(__file__).resolve().parents[1]
stage=root/'Export/UnitySceneryStage'
output=root/'Export/Pilot'
output.mkdir(parents=True,exist_ok=True)
shutil.copyfile(root/'Scripts/Unity/ExportPilot.cs',stage/'Assets/Editor/ExportPilot.cs')
editor=root.parent/'GotlandRingUnity/Tools/Unity/6000.3.25f1/Unity.app/Contents/MacOS/Unity'
subprocess.run([str(editor),'-batchmode','-nographics','-projectPath',str(stage),'-executeMethod','ExportPilot.Run','-quit','-logFile',str(root/'Export/unity_pilot.log')],env=dict(os.environ,GOTLAND_PILOT_OUTPUT=str(output)),check=True)
subprocess.run(['python3',str(root/'Scripts/compile_pilot.py')],check=True)
