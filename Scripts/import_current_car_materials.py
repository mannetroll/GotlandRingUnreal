"""Migrate the current Unity rally car surfaces, retaining the working cluster and glass."""
from pathlib import Path
import re,json
import unreal
root=Path(unreal.Paths.project_dir()).resolve();source=root.parent/'GotlandRingUnity';assets=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
lookup={}
for path in (source/'Assets').rglob('*.meta'):
 match=re.search(r'^guid: (\w+)',path.read_text(errors='replace'),re.M)
 if match:lookup[match[1]]=Path(str(path)[:-5])
materials={};folder='/Game/Migration/Vehicle'
def texture(guid):
 path=lookup[guid];name='Unity_'+path.stem.replace(' ','_')+'_'+guid[:8]
 asset=unreal.load_asset(folder+'/'+name)
 if not asset:
  t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=folder;t.destination_name=name;t.automated=True;t.save=True;assets.import_asset_tasks([t]);asset=unreal.load_asset(folder+'/'+name)
 return asset
for item in json.loads((root/'Export/Vehicle/vehicle.json').read_text())['meshes']:
 if item['material'] in ['glass','lcd_screen']:continue
 path=source/'Assets/Models/RallyCar/Materials'/(item['material'].replace(' ','_')+'.mat')
 if not path.exists():continue
 text=path.read_text();m=unreal.load_asset(folder+'/M_'+item['name']);assert isinstance(m,unreal.Material)
 edit.delete_all_material_expressions(m);m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT);m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
 match=re.search(r'_Color: \{r: ([\d.e-]+), g: ([\d.e-]+), b: ([\d.e-]+)',text);rgb=list(map(float,match.groups()))
 if item['material']=='livery':rgb=[.95,.035,.055]
 color=edit.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);color.set_editor_property('constant',unreal.LinearColor(*rgb))
 match=re.search(r'- _MainTex:\s+m_Texture: \{fileID: \d+, guid: (\w+)',text)
 if match:
  sample=edit.create_material_expression(m,unreal.MaterialExpressionTextureSample);sample.texture=texture(match[1]);mult=edit.create_material_expression(m,unreal.MaterialExpressionMultiply);assert edit.connect_material_expressions(sample,'RGB',mult,'A');assert edit.connect_material_expressions(color,'',mult,'B');edit.connect_material_property(mult,'',unreal.MaterialProperty.MP_BASE_COLOR)
 else:edit.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for field,prop,value in [('_Glossiness',unreal.MaterialProperty.MP_ROUGHNESS,None),('_Metallic',unreal.MaterialProperty.MP_METALLIC,None)]:
  match=re.search(r'- '+field+r': ([\d.e-]+)',text);value=float(match[1]);value=1-value if field=='_Glossiness' else value
  node=edit.create_material_expression(m,unreal.MaterialExpressionConstant);node.set_editor_property('r',value);edit.connect_material_property(node,'',prop)
 match=re.search(r'- _BumpMap:\s+m_Texture: \{fileID: \d+, guid: (\w+)',text)
 if match and '_NORMALMAP' in text:
  tex=texture(match[1]);tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('srgb',False);tex.set_editor_property('flip_green_channel',True)
  node=edit.create_material_expression(m,unreal.MaterialExpressionTextureSample);node.texture=tex;node.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL;edit.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_NORMAL)
 edit.recompile_material(m);materials[item['name']]=str(path.relative_to(source))
unreal.EditorAssetLibrary.save_directory(folder)
(root/'Saved/current_car_materials.json').write_text(json.dumps(materials,indent=2));unreal.log('CURRENT_CAR_MATERIALS_IMPORTED')
