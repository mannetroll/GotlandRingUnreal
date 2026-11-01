"""Import Unity-generated scenery into the current migration level."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
manifest = json.loads((root/'Export/Scenery/scenery.json').read_text())
assets = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
checks = []
textures = {}
for item in manifest['meshes']:
    name = item['name']
    task = unreal.AssetImportTask()
    task.filename = str(root/'Export/Scenery'/f'{name}.obj')
    task.destination_path = '/Game/Migration/Scenery'
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
    options.static_mesh_import_data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.options = options
    assets.import_asset_tasks([task])
    mesh = unreal.load_asset('/Game/Migration/Scenery/'+name)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError('Scenery import failed: '+name)
    material = unreal.load_asset('/Game/Migration/Scenery/M_'+name)
    if not material:
        material = assets.create_asset('M_'+name, '/Game/Migration/Scenery', unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    edit = unreal.MaterialEditingLibrary
    color = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(*item['color']))
    if item.get('texture'):
        texture_name = Path(item['texture']).stem
        if texture_name not in textures:
            texture_task = unreal.AssetImportTask()
            texture_task.filename = str(root/'Export/Scenery/Textures'/item['texture'])
            texture_task.destination_path = '/Game/Migration/Scenery'
            texture_task.automated = True
            texture_task.replace_existing = True
            texture_task.save = True
            assets.import_asset_tasks([texture_task])
            textures[texture_name] = unreal.load_asset('/Game/Migration/Scenery/'+texture_name)
        sample = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample)
        sample.texture = textures[texture_name]
        multiply = edit.create_material_expression(material, unreal.MaterialExpressionMultiply)
        edit.connect_material_expressions(sample, 'RGB', multiply, 'A')
        edit.connect_material_expressions(color, '', multiply, 'B')
        edit.connect_material_property(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR)
        if item['masked']:
            material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
            material.set_editor_property('two_sided', True)
            material.set_editor_property('opacity_mask_clip_value', 0.45)
            edit.connect_material_property(sample, 'A', unreal.MaterialProperty.MP_OPACITY_MASK)
    else:
        edit.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    for key, prop in [('roughness', unreal.MaterialProperty.MP_ROUGHNESS), ('metallic', unreal.MaterialProperty.MP_METALLIC)]:
        value = edit.create_material_expression(material, unreal.MaterialExpressionConstant)
        value.set_editor_property('r', item[key])
        edit.connect_material_property(value, '', prop)
    edit.recompile_material(material)
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0,0,0))
    actor.set_actor_label(item['material'])
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, material)
    center, extent = actor.get_actor_bounds(False)
    actual = [x for c,e in zip((center.x,center.y,center.z),(extent.x,extent.y,extent.z)) for x in (c-e,c+e)]
    error = max(abs(a-b) for a,b in zip(actual,item['bounds']))
    if error > 0.1:
        raise RuntimeError(f'Scenery bounds mismatch: {name}, {error} cm')
    checks.append(dict(name=name, material=item['material'], objects=item['objects'], triangles=item['triangles'], max_bounds_error_cm=error))
for label in manifest['labels']:
    position = unreal.Vector(*label['position'])
    direction = unreal.Vector(*label['forward'])
    rotation = unreal.MathLibrary.find_look_at_rotation(position, position+direction)
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, position, rotation)
    actor.set_actor_label('Unity sign: '+label['text'].replace('\n', ' / '))
    text = actor.get_component_by_class(unreal.TextRenderComponent)
    text.set_text(label['text'])
    text.set_world_size(label['height'])
    center, extent = actor.get_actor_bounds(False)
    actor.set_actor_location(position+(position-center), False, False)
(root/'Saved/scenery_validation.json').write_text(json.dumps(dict(trees=manifest['trees'],labels=len(manifest['labels']),meshes=checks),indent=2))
unreal.log(f'Imported {len(checks)} scenery groups, {manifest["trees"]} mapped trees')

# Apply the source sign colour after importing the scenery material groups.
import runpy
runpy.run_path(str(root/"Scripts/update_sign_material.py"))

runpy.run_path(str(root/"Scripts/fit_signs.py"))
