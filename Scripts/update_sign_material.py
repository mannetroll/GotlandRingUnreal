"""Preserve the navy/blue sign palette and white text under scene exposure."""
import unreal
edit=unreal.MaterialEditingLibrary
# Linear display colours calibrated for this project's filmic tone mapper.
# Scenery_16 contains the 42 sign number panels; road kerbs use separate assets.
for path,rgb in [('M_Scenery_15',(.008,.02,.03)),('M_Scenery_16',(.04,.15,.3))]:
 material=unreal.load_asset('/Game/Migration/Scenery/'+path)
 assert isinstance(material,unreal.Material)
 edit.delete_all_material_expressions(material)
 material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
 color=edit.create_material_expression(material,unreal.MaterialExpressionConstant3Vector)
 color.set_editor_property('constant',unreal.LinearColor(*rgb))
 exposure=edit.create_material_expression(material,unreal.MaterialExpressionEyeAdaptationInverse)
 assert edit.connect_material_expressions(color,'',exposure,'')
 edit.connect_material_property(exposure,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 edit.recompile_material(material)
 unreal.EditorAssetLibrary.save_loaded_asset(material)
text_material=unreal.load_asset('/Game/Migration/Scenery/M_SignText')
if not text_material:
 text_material=unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque','/Game/Migration/Scenery/M_SignText')
text_material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
node=edit.get_material_property_input_node(text_material,unreal.MaterialProperty.MP_BASE_COLOR)
output=edit.get_material_property_input_node_output_name(text_material,unreal.MaterialProperty.MP_BASE_COLOR)
exposure=edit.get_material_property_input_node(text_material,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
if not isinstance(exposure,unreal.MaterialExpressionEyeAdaptationInverse):
 exposure=edit.create_material_expression(text_material,unreal.MaterialExpressionEyeAdaptationInverse)
assert edit.connect_material_expressions(node,output,exposure,'')
edit.connect_material_property(exposure,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(text_material)
unreal.EditorAssetLibrary.save_loaded_asset(text_material)
unreal.log('Updated all navy faces, blue number panels and exposure-compensated white lettering')
