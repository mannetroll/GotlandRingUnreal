"""Reuse Unity's photographic HDR sky with its 145-degree rotation and 0.8 exposure."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);level.load_level('/Game/Migration/Prototype')
assets=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
folder='/Game/Migration/Sky'
task=unreal.AssetImportTask();task.filename=str(root.parent/'GotlandRingUnity/Assets/Resources/Visuals/Sky.hdr');task.destination_path=folder;task.destination_name='UnitySky';task.automated=True;task.replace_existing=True;task.save=True
assets.import_asset_tasks([task]);texture=unreal.load_asset(folder+'/UnitySky')
assert isinstance(texture,unreal.TextureCube),str(type(texture))
material=unreal.load_asset(folder+'/M_UnitySky') or assets.create_asset('M_UnitySky',folder,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(material);material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT);material.set_editor_property('two_sided',True);material.set_editor_property('is_sky',True)
position=edit.create_material_expression(material,unreal.MaterialExpressionWorldPosition)
camera=edit.create_material_expression(material,unreal.MaterialExpressionCameraPositionWS)
sub=edit.create_material_expression(material,unreal.MaterialExpressionSubtract);assert edit.connect_material_expressions(position,'',sub,'A');assert edit.connect_material_expressions(camera,'',sub,'B')
rotation=edit.create_material_expression(material,unreal.MaterialExpressionCustom);rotation.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
input=unreal.CustomInput();input.set_editor_property('input_name','Direction');rotation.set_editor_property('inputs',[input]);rotation.set_editor_property('code','float3 d=normalize(Direction); float a=radians(145.0);return float3(cos(a)*d.x-sin(a)*d.y,sin(a)*d.x+cos(a)*d.y,d.z);')
assert edit.connect_material_expressions(sub,'',rotation,'Direction')
sample=edit.create_material_expression(material,unreal.MaterialExpressionTextureSampleParameterCube);sample.texture=texture;sample.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR;assert edit.connect_material_expressions(rotation,'',sample,'UVs')
scale=edit.create_material_expression(material,unreal.MaterialExpressionMultiply);scale.set_editor_property('const_b',.8);assert edit.connect_material_expressions(sample,'RGB',scale,'A')
exposure=edit.create_material_expression(material,unreal.MaterialExpressionEyeAdaptationInverse);assert edit.connect_material_expressions(scale,'',exposure,'');assert edit.connect_material_property(exposure,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);edit.recompile_material(material)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
 if actor.get_class().get_name() in ['SkyAtmosphere','VolumetricCloud'] or actor.actor_has_tag('RingPhotographicSky'):actors.destroy_actor(actor)
actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());actor.set_actor_label('Unity photographic sky / white clouds');actor.set_editor_property('tags',['RingPhotographicSky'])
c=actor.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Sphere'));c.set_material(0,material);c.set_collision_profile_name('NoCollision');c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION);actor.set_actor_enable_collision(False);c.set_cast_shadow(False);actor.set_actor_scale3d(unreal.Vector(100000,100000,100000))
unreal.EditorAssetLibrary.save_directory(folder);level.save_current_level();unreal.log('UNITY_SKY_IMPORTED')
