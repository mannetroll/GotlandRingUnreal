from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
for name in ['import_sky.py','import_vehicle.py']:
 exec(compile((root/'Scripts'/name).read_text(),str(root/'Scripts'/name),'exec'))
