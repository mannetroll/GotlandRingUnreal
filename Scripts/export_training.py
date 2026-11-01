"""Export the source Unity training route and both handling-mode pilot speed plans."""
from pathlib import Path
import csv,os,shutil,subprocess
root=Path(__file__).resolve().parents[1];source=root.parent/'GotlandRingUnity';stage=root/'Export/UnitySceneryStage';output=root/'Export/Training'
for name in ['DrivingSurface','AutopilotController','TrackData','DrivingSettings']:shutil.copyfile(source/'Assets/Scripts'/(name+'.cs'),stage/'Assets/Scripts'/(name+'.cs'))
shutil.copyfile(root/'Scripts/Unity/ExportTraining.cs',stage/'Assets/Editor/ExportTraining.cs')
editor=source/'Tools/Unity/6000.3.25f1/Unity.app/Contents/MacOS/Unity'
subprocess.run([str(editor),'-batchmode','-nographics','-projectPath',str(stage),'-executeMethod','ExportTraining.Run','-quit','-logFile',str(root/'Export/training_export.log')],env=dict(os.environ,GOTLAND_TRAINING_OUTPUT=str(output)),check=True)
def literal(s):return (s if '.' in s or 'e' in s.lower() else s+'.0')+'f'
text='#pragma once\n#include "RingPilot.h"\nnamespace Ring {\n'
for name in ['awd','arcade']:
 rows=list(csv.reader((output/(name+'.csv')).open()))[1:];text+='constexpr RouteRow Training'+name.title()+'[]={\n'+''.join('{'+','.join(map(literal,r))+'},\n' for r in rows)+'};\n'
(root/'Source/GotlandRingUnreal/RingTrainingData.h').write_text(text+'}\n')
