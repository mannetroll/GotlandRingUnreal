"""Compile the Unity-exported racing line and preserve controller reference fixtures."""
import csv
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
rows=list(csv.reader((root/'Export/Pilot/route.csv').open()))[1:]
def literal(s):
    return (s if '.' in s or 'e' in s.lower() else s+'.0')+'f'
(root/'Source/GotlandRingUnreal/RingRouteData.h').write_text('#pragma once\n#include "RingPilot.h"\nnamespace Ring {\nconstexpr RouteRow RouteData[]={\n'+''.join('{'+','.join(map(literal,r))+'},\n' for r in rows)+'};\n}\n')
shutil.copyfile(root/'Export/Pilot/pilot_reference.csv',root/'Validation/pilot_reference.csv')
