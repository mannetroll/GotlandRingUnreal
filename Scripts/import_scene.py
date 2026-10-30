"""Run in Unreal Editor Python. Requires export_scene.py first."""
import json
import math
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
metadata = json.loads((root/'Export/validation.json').read_text())
source_meshes = json.loads((root/'Export/scene.json').read_text())['meshes']
import_checks = []
assets = unreal.AssetToolsHelpers.get_asset_tools()
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_path = '/Game/Migration/Prototype'
if unreal.EditorAssetLibrary.does_asset_exist(level_path):
    if not levels.load_level(level_path):
        raise RuntimeError('Could not load generated level')
    actors.destroy_actors(actors.get_all_level_actors())
elif not levels.new_level(level_path):
    raise RuntimeError('Could not create generated level')
for name, texture_name in [('Road','Asphalt'),('LeftApron','Gravel'),('RightApron','Gravel')]:
    task = unreal.AssetImportTask()
    task.filename = str(root/'Export'/f'{name}.obj')
    task.destination_path = '/Game/Migration'
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_materials = False
    options.import_textures = False
    options.static_mesh_import_data.set_editor_property('convert_scene', False)
    options.static_mesh_import_data.set_editor_property('convert_scene_unit', False)
    options.static_mesh_import_data.set_editor_property('import_uniform_scale', 1.0)
    task.options = options
    assets.import_asset_tasks([task])
    mesh = unreal.load_asset('/Game/Migration/'+name)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError('Static mesh import failed: '+name)
    texture_task = unreal.AssetImportTask()
    texture_task.filename = str(root/'Export/Textures'/f'{texture_name}.jpg')
    texture_task.destination_path = '/Game/Migration'
    texture_task.automated = True
    texture_task.replace_existing = True
    texture_task.save = True
    assets.import_asset_tasks([texture_task])
    texture = unreal.load_asset('/Game/Migration/'+texture_name)
    if not texture: raise RuntimeError('Texture import failed')
    mat_path = '/Game/Migration/M_'+name
    material = unreal.load_asset(mat_path)
    if not material:
        material = assets.create_asset('M_'+name, '/Game/Migration', unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample)
    sample.texture = texture
    unreal.MaterialEditingLibrary.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,0))
    actor.set_actor_label(name)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0,material)
    center, extent = actor.get_actor_bounds(False)
    actual = [[c-e,c+e] for c,e in zip((center.x,center.y,center.z),(extent.x,extent.y,extent.z))]
    points = next(m['vertices'] for m in source_meshes if m['name']==name)
    expected = [[min(v[a] for v in points),max(v[a] for v in points)] for a in range(3)]
    error = max(abs(actual[a][b]-expected[a][b]) for a in range(3) for b in range(2))
    if error > 0.1:
        raise RuntimeError(f'{name}: imported bounds differ by {error} cm: {actual} versus {expected}')
    import_checks.append(dict(mesh=name,bounds_cm=actual,max_bounds_error_cm=error))
exec(compile((root/'Scripts/import_scenery.py').read_text(), str(root/'Scripts/import_scenery.py'), 'exec'))
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,10000), unreal.Rotator(pitch=-45,yaw=30,roll=0))
sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_component.set_editor_property('atmosphere_sun_light', True)
sun_component.set_editor_property('intensity', 10.0)
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,10000))
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_editor_property('real_time_capture', True)
sky_component.set_editor_property('intensity', 0.5)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0,0,0))
bounds = metadata['bounds_cm']
center = [(lo+hi)/2 for lo,hi in bounds]
height = max((bounds[0][1]-bounds[0][0])*16/9, bounds[1][1]-bounds[1][0])*0.8
camera_location = unreal.Vector(center[0]-height/math.tan(math.radians(75)), center[1], center[2]+height)
camera_rotation = unreal.Rotator(pitch=-75,yaw=0,roll=0)
camera = actors.spawn_actor_from_class(unreal.CameraActor, camera_location, camera_rotation)
settings = unreal.PostProcessSettings()
settings.set_editor_property('override_auto_exposure_method', True)
settings.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
settings.set_editor_property('override_auto_exposure_bias', True)
settings.set_editor_property('auto_exposure_bias', 0.0)
settings.set_editor_property('override_auto_exposure_apply_physical_camera_exposure', True)
settings.set_editor_property('auto_exposure_apply_physical_camera_exposure', False)
camera.get_component_by_class(unreal.CameraComponent).set_editor_property('post_process_settings', settings)
world.set_level_viewport_camera_info(camera_location, camera_rotation)
if not levels.save_current_level():
    raise RuntimeError('Could not save generated level')
unreal.EditorAssetLibrary.save_directory('/Game/Migration')
(root/'Saved').mkdir(exist_ok=True)
(root/'Saved/import_validation.json').write_text(json.dumps(dict(engine=unreal.SystemLibrary.get_engine_version(),chainage_m=metadata['chainage_m'],cross_sections=metadata['rows'],triangles=metadata['triangles'],circuit_closure_gap_cm=metadata['circuit_closure_gap_cm'],meshes=import_checks),indent=2))
unreal.log('Import finished. Verify scale, winding, UV orientation and lighting in the viewport.')

exec(compile((root/'Scripts/import_vehicle.py').read_text(), str(root/'Scripts/import_vehicle.py'), 'exec'))
