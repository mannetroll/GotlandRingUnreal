"""Import pivot-centred Unity rotors and replace the combined paint geometry."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
data=json.loads((root/'Export/WindRotors/rotors.json').read_text())
assets=unreal.AssetToolsHelpers.get_asset_tools()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Migration/Prototype')
for actor in actors.get_all_level_actors():
 if actor.actor_has_tag('RingRotor'):actors.destroy_actor(actor)
for name in [data['paint_mesh']]+[r['mesh'] for r in data['rotors']]:
 task=unreal.AssetImportTask()
 task.filename=str(root/'Export/WindRotors'/f'{name}.obj');task.destination_path='/Game/Migration/Scenery';task.destination_name=name
 task.automated=True;task.replace_existing=True;task.save=True
 options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False
 options.static_mesh_import_data.set_editor_property('convert_scene',False)
 options.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
 options.static_mesh_import_data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
 task.options=options;assets.import_asset_tasks([task])
 mesh=unreal.load_asset('/Game/Migration/Scenery/'+name)
 assert isinstance(mesh,unreal.StaticMesh), name
 if name==data['paint_mesh']:continue
 r=next(r for r in data['rotors'] if r['mesh']==name)
 actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*r['hub']))
 actor.set_actor_label('Wind rotor / '+r['id']);actor.tags=['RingRotor']
 component=actor.static_mesh_component;component.set_mobility(unreal.ComponentMobility.MOVABLE)
 component.set_static_mesh(mesh);component.set_material(0,unreal.load_asset('/Game/Migration/Scenery/M_'+data['paint_mesh']))
 component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
levels.save_current_level()
unreal.EditorAssetLibrary.save_directory('/Game/Migration/Scenery')
unreal.log('Imported 12 independently movable Unity wind rotors')
