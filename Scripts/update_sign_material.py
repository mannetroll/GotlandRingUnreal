"""Use the Unity sign's sRGB navy colour in Unreal's linear material graph."""
import unreal
material=unreal.load_asset('/Game/Migration/Scenery/M_Scenery_15')
assert isinstance(material,unreal.Material)
edit=unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
def linear(v):return v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4
color=edit.create_material_expression(material,unreal.MaterialExpressionConstant3Vector)
color.set_editor_property('constant',unreal.LinearColor(*[linear(v) for v in (.025,.075,.11)]))
edit.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
rough=edit.create_material_expression(material,unreal.MaterialExpressionConstant)
rough.set_editor_property('r',.88)
edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
edit.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('Updated Unity track sign navy material')
