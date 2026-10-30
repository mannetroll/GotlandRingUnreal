"""Capture the saved main-building, pit-straight, and turbine scenery."""
from pathlib import Path
import unreal
root = Path(unreal.Paths.project_dir()).resolve()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/Migration/Prototype'):
    raise RuntimeError('Saved migration level is missing')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
camera = next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.CameraActor))
position = unreal.Vector(-14000,-18000,12000)
target = unreal.Vector(13000,-1000,1000)
camera.set_actor_location(position, False, False)
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(position,target), False)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.AutomationLibrary.finish_loading_before_screenshot()
capture = unreal.AutomationLibrary.take_high_res_screenshot(2560,1440,str(root/'Saved/GotlandRingDetail.png'),camera=camera,delay=10.0,force_game_view=True)
