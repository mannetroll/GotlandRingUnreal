"""Import the Unity analogue cluster into the existing vehicle's instrument slot."""
from pathlib import Path
import json
import unreal


def import_cluster(mesh_name='Vehicle_16'):
    root=Path(unreal.Paths.project_dir()).resolve()
    assets=unreal.AssetToolsHelpers.get_asset_tools()
    edit=unreal.MaterialEditingLibrary
    folder='/Game/Migration/Vehicle'
    def task(filename,name):
        t=unreal.AssetImportTask()
        t.filename=str(filename);t.destination_path=folder;t.destination_name=name
        t.automated=True;t.replace_existing=True;t.save=True
        return t
    texture_task=task(root/'Export/Instruments/PlesseCluster.png','PlesseCluster')
    assets.import_asset_tasks([texture_task])
    texture=unreal.load_asset(folder+'/PlesseCluster')
    mesh_task=task(root/'Export/Instruments/InstrumentCluster.obj',mesh_name)
    options=unreal.FbxImportUI()
    options.import_mesh=True;options.import_materials=False;options.import_textures=False
    options.static_mesh_import_data.set_editor_property('convert_scene',False)
    options.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
    options.static_mesh_import_data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)
    mesh_task.options=options
    assets.import_asset_tasks([mesh_task])
    mesh=unreal.load_asset(folder+'/'+mesh_name)
    material=unreal.load_asset(folder+'/M_'+mesh_name)
    if not material:
        material=assets.create_asset('M_'+mesh_name,folder,unreal.Material,unreal.MaterialFactoryNew())
    edit.delete_all_material_expressions(material)
    material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property('two_sided',True)
    custom=edit.create_material_expression(material,unreal.MaterialExpressionCustom)
    custom.set_editor_property('code',(root/'Scripts/CockpitDisplay.hlsl').read_text())
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs=[]
    for name in ['UV','Face','Rpm','Speed','Gear','Fuel','Coolant','TripKm']:
        i=unreal.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
    custom.set_editor_property('inputs',inputs)
    uv=edit.create_material_expression(material,unreal.MaterialExpressionTextureCoordinate)
    edit.connect_material_expressions(uv,'',custom,'UV')
    sample=edit.create_material_expression(material,unreal.MaterialExpressionTextureSample)
    sample.texture=texture
    edit.connect_material_expressions(sample,'RGBA',custom,'Face')
    for key,value in [('Rpm',900),('Speed',0),('Gear',1),('Fuel',1),('Coolant',.5),('TripKm',0)]:
        param=edit.create_material_expression(material,unreal.MaterialExpressionScalarParameter)
        param.set_editor_property('parameter_name',key);param.set_editor_property('default_value',value)
        edit.connect_material_expressions(param,'',custom,key)
    rgb=edit.create_material_expression(material,unreal.MaterialExpressionComponentMask)
    rgb.set_editor_property('r',True);rgb.set_editor_property('g',True);rgb.set_editor_property('b',True)
    alpha=edit.create_material_expression(material,unreal.MaterialExpressionComponentMask)
    alpha.set_editor_property('r',False);alpha.set_editor_property('g',False);alpha.set_editor_property('b',False);alpha.set_editor_property('a',True)
    assert edit.connect_material_expressions(custom,'',rgb,'')
    assert edit.connect_material_expressions(custom,'',alpha,'')
    edit.connect_material_property(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    edit.recompile_material(material)
    mesh.set_material(0,material)
    expected=json.loads((root/'Export/Instruments/instruments.json').read_text())['unreal_vertices_cm']
    bounds=mesh.get_bounding_box()
    for axis in ('x','y','z'):
        i=('x','y','z').index(axis)
        assert abs(getattr(bounds.min,axis)-min(p[i] for p in expected))<.01
        assert abs(getattr(bounds.max,axis)-max(p[i] for p in expected))<.01
    for asset in (texture,material,mesh):unreal.EditorAssetLibrary.save_loaded_asset(asset)
    (root/'Saved/instruments_import_validation.json').write_text(json.dumps(dict(mesh=mesh_name,bounds_match=True,passed=True),indent=2))
    unreal.log('INSTRUMENTS_IMPORT_COMPLETE')

if __name__=='__main__':
    import_cluster()
