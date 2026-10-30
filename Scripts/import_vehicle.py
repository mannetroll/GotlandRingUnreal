"""Import the Subaru prefab and configure drivable collision in the migration level."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
manifest = json.loads((root/'Export/Vehicle/vehicle.json').read_text())
assets = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
checks = []
textures = {}
for item in manifest['meshes']:
    name = item['name']
    task = unreal.AssetImportTask()
    task.filename = str(root/'Export/Vehicle'/f'{name}.obj')
    task.destination_path = '/Game/Migration/Vehicle'
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
    mesh = unreal.load_asset('/Game/Migration/Vehicle/'+name)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError('Vehicle import failed: '+name)
    material = unreal.load_asset('/Game/Migration/Vehicle/M_'+name)
    if not material:
        material = assets.create_asset('M_'+name, '/Game/Migration/Vehicle', unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    edit = unreal.MaterialEditingLibrary
    color = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(*item['color']))
    if item.get('texture'):
        texture_name = Path(item['texture']).stem
        if texture_name not in textures:
            texture_task = unreal.AssetImportTask()
            texture_task.filename = str(root/'Export/Vehicle/Textures'/item['texture'])
            texture_task.destination_path = '/Game/Migration/Vehicle'
            texture_task.automated = True
            texture_task.replace_existing = True
            texture_task.save = True
            assets.import_asset_tasks([texture_task])
            textures[texture_name] = unreal.load_asset('/Game/Migration/Vehicle/'+texture_name)
        sample = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample)
        sample.texture = textures[texture_name]
        multiply = edit.create_material_expression(material, unreal.MaterialExpressionMultiply)
        edit.connect_material_expressions(sample, 'RGB', multiply, 'A')
        edit.connect_material_expressions(color, '', multiply, 'B')
        edit.connect_material_property(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        edit.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    for key, prop in [('roughness', unreal.MaterialProperty.MP_ROUGHNESS), ('metallic', unreal.MaterialProperty.MP_METALLIC)]:
        value = edit.create_material_expression(material, unreal.MaterialExpressionConstant)
        value.set_editor_property('r', item[key])
        edit.connect_material_property(value, '', prop)
    edit.recompile_material(material)
    mesh.set_material(0, material)
    if item['material']=='glass':
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        alpha=edit.create_material_expression(material,unreal.MaterialExpressionConstant)
        alpha.set_editor_property('r',.18)
        edit.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
        edit.recompile_material(material)
    if item['material']=='lcd_screen':
        edit.delete_all_material_expressions(material)
        material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        custom=edit.create_material_expression(material,unreal.MaterialExpressionCustom)
        custom.set_editor_property('code',(root/'Scripts/CockpitDisplay.hlsl').read_text())
        custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs=[]
        for input_name in ['UV','Rpm','Speed','Gear']:
            item_input=unreal.CustomInput()
            item_input.set_editor_property('input_name',input_name)
            inputs.append(item_input)
        custom.set_editor_property('inputs',inputs)
        uv=edit.create_material_expression(material,unreal.MaterialExpressionTextureCoordinate)
        edit.connect_material_expressions(uv,'',custom,'UV')
        for key,value in [('Rpm',900),('Speed',0),('Gear',1)]:
            param=edit.create_material_expression(material,unreal.MaterialExpressionScalarParameter)
            param.set_editor_property('parameter_name',key)
            param.set_editor_property('default_value',value)
            edit.connect_material_expressions(param,'',custom,key)
        edit.connect_material_property(custom,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        edit.recompile_material(material)
    checks.append(dict(name=name,triangles=item['triangles']))
car_class=unreal.load_class(None,'/Script/GotlandRingUnreal.RingCar')
for actor in actors.get_all_level_actors():
    if actor.get_class()==car_class: actors.destroy_actor(actor)
spawn=json.loads((root/'Export/Vehicle/spawn.json').read_text())
car=actors.spawn_actor_from_class(car_class,unreal.Vector(*spawn['position']),unreal.Rotator(pitch=0,yaw=spawn['yaw'],roll=0))
car.set_actor_label('Subaru Impreza — drive with Play')
car.set_editor_property('visual_meshes',[unreal.load_asset('/Game/Migration/Vehicle/'+i['name']) for i in manifest['meshes']])
car.set_editor_property('visual_wheels',[i['wheel'] for i in manifest['meshes']])
pivots=[next(i['pivot'] for i in manifest['meshes'] if i['wheel']==w) for w in range(4)]
car.set_editor_property('wheel_centers',[unreal.Vector(*p) for p in pivots])
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
collision=[]
scenery=json.loads((root/'Export/Scenery/scenery.json').read_text())
foliage={i['name'] for i in scenery['meshes'] if i['masked']}
for folder in ['/Game/Migration','/Game/Migration/Scenery']:
    for path in unreal.EditorAssetLibrary.list_assets(folder,recursive=False):
        mesh=unreal.load_asset(path)
        if not isinstance(mesh,unreal.StaticMesh):continue
        mesh_editor.remove_collisions(mesh)
        body=mesh.get_editor_property('body_setup')
        body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        body.set_editor_property('double_sided_geometry',True)
        collision.append(mesh.get_name())
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.StaticMeshActor):
        component=actor.static_mesh_component
        mesh=component.static_mesh
        if mesh:
            material=mesh.get_material(0)
            masked=mesh.get_name() in foliage
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION if masked else unreal.CollisionEnabled.QUERY_AND_PHYSICS)
            component.set_collision_profile_name('NoCollision' if masked else 'BlockAll')
# Explicit world override also covers this level when launched with -game.
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('force_no_precomputed_lighting',True)
for actor in actors.get_all_level_actors():
    if isinstance(actor,(unreal.DirectionalLight,unreal.SkyLight)):
        actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/GotlandRingUnreal.RingGameMode'))
unreal.EditorAssetLibrary.save_directory('/Game/Migration')
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
(root/'Saved/vehicle_import_validation.json').write_text(json.dumps(dict(meshes=checks,collision_meshes=collision,wheel_centers_cm=pivots,wheel_radius_cm=manifest['wheelRadius']*100),indent=2))
unreal.log('VEHICLE_IMPORT_COMPLETE')
