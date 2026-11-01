"""Export Unity's current driver without modifying the source checkout, then compile replay data."""
from pathlib import Path
import os,shutil,subprocess
root=Path(__file__).resolve().parents[1];source=root.parent/'GotlandRingUnity';stage=root/'Export/UnitySceneryStage'
for name in ['Assets','Packages','ProjectSettings']:shutil.copytree(source/name,stage/name,dirs_exist_ok=True)
for name in ['ExportPilot.cs','ExportPilot.cs.meta']:(stage/'Assets/Editor'/name).unlink(missing_ok=True)
shutil.copyfile(root/'Scripts/Unity/ExportDriver.cs',stage/'Assets/Editor/ExportDriver.cs')
editor=source/'Tools/Unity/6000.3.25f1/Unity.app/Contents/MacOS/Unity'
subprocess.run([str(editor),'-batchmode','-nographics','-projectPath',str(stage),'-executeMethod','ExportDriver.Run','-quit','-logFile',str(root/'Export/unity_driver.log')],env=dict(os.environ,GOTLAND_DRIVER_OUTPUT=str(root/'Export/Vehicle/driver.json')),check=True)
subprocess.run(['python3',str(root/'Scripts/compile_reference.py')],check=True)
