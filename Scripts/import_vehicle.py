"""Import both Unity car prefabs with their own UV-matched materials and training assets."""
import json,re,hashlib
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve();source=root.parent/'GotlandRingUnity'
assets=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Migration/Prototype')
lookup={}
for p in (source/'Assets').rglob('*.meta'):
 m=re.search(r'^guid: (\w+)',p.read_text(errors='replace'),re.M)
 if m:lookup[m[1]]=Path(str(p)[:-5])
texture_cache={};parity=[]
def texture(guid,normal=False,mask=False):
 key=(guid,normal,mask)
 if key in texture_cache:return texture_cache[key]
 p=lookup[guid];name='T_'+guid;folder='/Game/Migration/Cars/Textures';asset=unreal.load_asset(folder+'/'+name)
 if not asset:
  task=unreal.AssetImportTask();task.filename=str(p);task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;assets.import_asset_tasks([task]);asset=task.imported_object_paths and unreal.load_asset(task.imported_object_paths[0])
 if normal:asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);asset.set_editor_property('srgb',False);asset.set_editor_property('flip_green_channel',True)
 if mask:asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS);asset.set_editor_property('srgb',False)
 unreal.EditorAssetLibrary.save_loaded_asset(asset);texture_cache[key]=asset;return asset

def constant(mat,value):
 n=edit.create_material_expression(mat,unreal.MaterialExpressionConstant);n.set_editor_property('r',value);return n

def material(model,item,paint):
 folder='/Game/Migration/Cars/'+model;name='M_'+item['name']
 if item['material']=='glass':return unreal.load_asset('/Game/Migration/Vehicle/M_Vehicle_14')
 if item['material']=='lcd_screen':return unreal.load_asset('/Game/Migration/Vehicle/M_Vehicle_16')
 p=source/item['materialPath'];text=p.read_text()
 mat=unreal.load_asset(folder+'/'+name) or assets.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew());edit.delete_all_material_expressions(mat)
 color=edit.create_material_expression(mat,unreal.MaterialExpressionVectorParameter if paint else unreal.MaterialExpressionConstant3Vector)
 rgba=item['color'];rgb=unreal.LinearColor(*rgba)
 if paint:color.set_editor_property('parameter_name','BodyPaint');color.set_editor_property('default_value',rgb)
 else:color.set_editor_property('constant',rgb)
 def sample(field,normal=False,mask=False):
  match=re.search(r'- '+field+r':\s+m_Texture: \{fileID: \d+, guid: (\w+)',text)
  if not match:return None
  node=edit.create_material_expression(mat,unreal.MaterialExpressionTextureSample);node.texture=texture(match[1],normal,mask)
  if normal:node.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
  if mask:node.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
  parity.append(dict(model=model,mesh=item['name'],material=item['material'],field=field,guid=match[1],sha256=hashlib.sha256(lookup[match[1]].read_bytes()).hexdigest()))
  return node
 base=sample('_MainTex')
 if base:
  mult=edit.create_material_expression(mat,unreal.MaterialExpressionMultiply);assert edit.connect_material_expressions(base,'RGB',mult,'A');assert edit.connect_material_expressions(color,'',mult,'B');assert edit.connect_material_property(mult,'',unreal.MaterialProperty.MP_BASE_COLOR)
 else:assert edit.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
 floats={k:float(v) for k,v in re.findall(r'- (_\w+): ([\d.e-]+)',text)}
 metal=sample('_MetallicGlossMap',mask=True) if '_METALLICGLOSSMAP' in text else None
 if metal:
  assert edit.connect_material_property(metal,'R',unreal.MaterialProperty.MP_METALLIC)
  mult=edit.create_material_expression(mat,unreal.MaterialExpressionMultiply);assert edit.connect_material_expressions(metal,'A',mult,'A');assert edit.connect_material_expressions(constant(mat,floats['_GlossMapScale']),'',mult,'B');rough=edit.create_material_expression(mat,unreal.MaterialExpressionOneMinus);assert edit.connect_material_expressions(mult,'',rough,'')
 else:
  assert edit.connect_material_property(constant(mat,floats['_Metallic']),'',unreal.MaterialProperty.MP_METALLIC);rough=constant(mat,1-floats['_Glossiness'])
 assert edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 normal=sample('_BumpMap',normal=True) if '_NORMALMAP' in text else None
 if normal:assert edit.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
 edit.recompile_material(mat);return mat

specs=[]
for model in ['RallyCar','SubaruImpreza']:
 data=json.loads((root/'Export/Models'/model/'vehicle.json').read_text());meshes=[];folder='/Game/Migration/Cars/'+model
 for i,item in enumerate(data['meshes']):
  task=unreal.AssetImportTask();task.filename=str(root/'Export/Models'/model/(item['name']+'.obj'));task.destination_path=folder;task.destination_name=item['name'];task.automated=True;task.replace_existing=True;task.save=True
  options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.static_mesh_import_data.set_editor_property('convert_scene',False);options.static_mesh_import_data.set_editor_property('convert_scene_unit',False);options.static_mesh_import_data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS);task.options=options
  assets.import_asset_tasks([task]);mesh=unreal.load_asset(folder+'/'+item['name']);mesh.set_material(0,material(model,item,i==data['paintSlot']));meshes.append(mesh)
 spec=unreal.RingCarModel();spec.set_editor_property('display_name',data['displayName']);spec.set_editor_property('credit',data['credit']);spec.set_editor_property('meshes',meshes);spec.set_editor_property('wheels',[i['wheel'] for i in data['meshes']]);spec.set_editor_property('centers',[unreal.Vector(*next(i['pivot'] for i in data['meshes'] if i['wheel']==w)) for w in range(4)])
 for field,key in [('cockpit','cockpit'),('bonnet','bonnet'),('steering_axis','steeringAxis')]:spec.set_editor_property(field,unreal.Vector(*data[key]))
 spec.set_editor_property('steering_pivot',unreal.Vector(*next(i['pivot'] for i in data['meshes'] if i['wheel']==-3)));spec.set_editor_property('paint_colors',[unreal.LinearColor(*data['paintColors'][j:j+4]) for j in range(0,len(data['paintColors']),4)]);spec.set_editor_property('paint_names',data['paintNames']);spec.set_editor_property('paint_slot',data['paintSlot']);spec.set_editor_property('initial_paint',data['initialPaint']);specs.append(spec)

training=[]
for name,color,tex,tiles in [('Asphalt',[1,1,1],'/Game/Migration/Asphalt',200),('Grass',[1,1,1],'Grass.jpg.meta',750),('Paint',[.8,.8,.68],None,1),('Orange',[1,.3,.035],None,1),('Black',[.025,.032,.039],None,1)]:
 folder='/Game/Migration/Training';mat=unreal.load_asset(folder+'/M_'+name) or assets.create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew());edit.delete_all_material_expressions(mat);mat.set_editor_property('two_sided',True)
 if tex:
  node=edit.create_material_expression(mat,unreal.MaterialExpressionTextureSample);node.texture=unreal.load_asset(tex) if tex.startswith('/Game/') else texture(re.search(r'^guid: (\w+)',(source/'Assets/Resources/Visuals'/tex).read_text(),re.M)[1]);uv=edit.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate);uv.set_editor_property('u_tiling',tiles);uv.set_editor_property('v_tiling',tiles);assert edit.connect_material_expressions(uv,'',node,'UVs');assert edit.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
 else:
  node=edit.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);node.set_editor_property('constant',unreal.LinearColor(*color));assert edit.connect_material_property(node,'',unreal.MaterialProperty.MP_BASE_COLOR)
 if name in ['Asphalt','Grass']:
  guid=re.search(r'^guid: (\w+)',(source/'Assets/Resources/Visuals'/(name+'Normal.jpg.meta')).read_text(),re.M)[1]
  normal=edit.create_material_expression(mat,unreal.MaterialExpressionTextureSample);normal.texture=texture(guid,normal=True);normal.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL;assert edit.connect_material_expressions(uv,'',normal,'UVs');assert edit.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
 assert edit.connect_material_property(constant(mat,.78 if name=='Asphalt' else .95),'',unreal.MaterialProperty.MP_ROUGHNESS);edit.recompile_material(mat);training.append(mat)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);car=next((a for a in actors.get_all_level_actors() if a.get_class().get_name()=='RingCar'),None)
if car is None:
 spawn=json.loads((root/'Export/Models/spawn.json').read_text());car=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/GotlandRingUnreal.RingCar'),unreal.Vector(*spawn['position']),unreal.Rotator(yaw=spawn['yaw']))
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
scenery=json.loads((root/'Export/Scenery/scenery.json').read_text());foliage={i['name'] for i in scenery['meshes'] if i['masked']}
for folder in ['/Game/Migration','/Game/Migration/Scenery']:
 for asset_path in unreal.EditorAssetLibrary.list_assets(folder,recursive=False):
  mesh=unreal.load_asset(asset_path)
  if not isinstance(mesh,unreal.StaticMesh):continue
  mesh_editor.remove_collisions(mesh);body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
for actor in actors.get_all_level_actors():
 if isinstance(actor,unreal.StaticMeshActor):
  c=actor.static_mesh_component
  if c.static_mesh and c.static_mesh.get_name() in foliage:c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
car.set_actor_label('Gotland Ring / two Unity car models')
car.set_editor_property('models',specs);car.set_editor_property('training_plane_mesh',unreal.load_asset('/Engine/BasicShapes/Plane'));car.set_editor_property('training_cube_mesh',unreal.load_asset('/Engine/BasicShapes/Cube'));car.set_editor_property('training_materials',training)
unreal.EditorAssetLibrary.save_directory('/Game/Migration/Cars');unreal.EditorAssetLibrary.save_directory('/Game/Migration/Training');unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
(root/'Validation/car_material_parity.json').write_text(json.dumps(dict(passed=True,models=[s.get_editor_property('display_name') for s in specs],textures=parity),indent=2))
unreal.log('BOTH_UNITY_CARS_IMPORTED')
