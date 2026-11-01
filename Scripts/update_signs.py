"""Update the complete saved circuit sign set."""
from pathlib import Path
import runpy,unreal
root=Path(unreal.Paths.project_dir()).resolve()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Migration/Prototype')
for name in ['update_sign_material.py','fit_signs.py']:runpy.run_path(str(root/'Scripts'/name))
levels.save_current_level()
