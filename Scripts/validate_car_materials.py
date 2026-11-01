"""Check the imported car meshes against each prefab's actual Unity material references."""
from pathlib import Path
import json,re,hashlib
import unreal
root=Path(unreal.Paths.project_dir()).resolve();source=root.parent/'GotlandRingUnity';edit=unreal.MaterialEditingLibrary
lookup={}
for p in (source/'Assets').rglob('*.meta'):
 m=re.search(r'^guid: (\w+)',p.read_text(errors='replace'),re.M)
 if m:lookup[m[1]]=Path(str(p)[:-5])
checks=[]
for model in ['RallyCar','SubaruImpreza']:
 data=json.loads((root/'Export/Models'/model/'vehicle.json').read_text())
 for item in data['meshes']:
  mesh=unreal.load_asset('/Game/Migration/Cars/'+model+'/'+item['name']);mat=mesh.get_material(0)
  if item['material'] in ['glass','lcd_screen']:
   expected='/Game/Migration/Vehicle/M_Vehicle_'+('14' if item['material']=='glass' else '16');assert mat.get_path_name().split('.')[0]==expected
   continue
  text=(source/item['materialPath']).read_text();used=set();pending=[edit.get_material_property_input_node(mat,p) for p in [unreal.MaterialProperty.MP_BASE_COLOR,unreal.MaterialProperty.MP_NORMAL,unreal.MaterialProperty.MP_METALLIC,unreal.MaterialProperty.MP_ROUGHNESS]]
  while pending:
   node=pending.pop()
   if node is None:continue
   if isinstance(node,unreal.MaterialExpressionTextureSample):used.add(node.texture.get_name())
   if isinstance(node,unreal.MaterialExpressionOneMinus):assert all(edit.get_inputs_for_material_expression(mat,node)),(model,item['name'],'disconnected roughness')
   pending.extend(edit.get_inputs_for_material_expression(mat,node))
  fields=['_MainTex']+(['_BumpMap'] if '_NORMALMAP' in text else [])+(['_MetallicGlossMap'] if '_METALLICGLOSSMAP' in text else [])
  for field in fields:
   m=re.search(r'- '+field+r':\s+m_Texture: \{fileID: \d+, guid: (\w+)',text)
   if not m:continue
   guid=m[1];assert 'T_'+guid in used,(model,item['name'],field,used)
   checks.append(dict(model=model,mesh=item['name'],source_material=item['materialPath'],field=field,guid=guid,sha256=hashlib.sha256(lookup[guid].read_bytes()).hexdigest()))
(root/'Validation/car_material_parity.json').write_text(json.dumps(dict(passed=True,models=['RallyCar','SubaruImpreza'],texture_checks=len(checks),textures=checks),indent=2))
unreal.log('CAR_MATERIAL_PARITY_PASSED')
