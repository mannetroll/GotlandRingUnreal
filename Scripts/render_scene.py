"""Import the full circuit, capture it after rendering settles, and leave the Editor open."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
exec(compile((root/'Scripts/import_scene.py').read_text(), str(root/'Scripts/import_scene.py'), 'exec'))
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.AutomationLibrary.finish_loading_before_screenshot()
camera = next(a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.CameraActor))
capture = unreal.AutomationLibrary.take_high_res_screenshot(2560,1440,str(root/'Saved/GotlandRing.png'),camera=camera,delay=10.0,force_game_view=True)
unreal.log('GotlandRing import validated; screenshot requested at Saved/GotlandRing.png')
