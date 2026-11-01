"""Set readable Baltic daylight and dry road surfaces for every camera."""
import unreal
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Migration/Prototype')
for actor in actors.get_all_level_actors():
 if isinstance(actor,unreal.DirectionalLight):
  light=actor.get_component_by_class(unreal.DirectionalLightComponent)
  light.set_mobility(unreal.ComponentMobility.MOVABLE);light.set_intensity(6.0)
  light.set_light_color(unreal.LinearColor(1,.95,.86))
  light.set_editor_property('light_source_angle',1.5)
  actor.set_actor_rotation(unreal.Rotator(pitch=-32,yaw=-48,roll=0),False)
 if isinstance(actor,unreal.SkyLight):
  light=actor.get_component_by_class(unreal.SkyLightComponent)
  light.set_mobility(unreal.ComponentMobility.MOVABLE);light.set_intensity(.8)
  light.set_editor_property('real_time_capture',True)
settings=unreal.PostProcessSettings()
for name,value in [('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL),('auto_exposure_apply_physical_camera_exposure',False),('auto_exposure_bias',0.0),('bloom_intensity',0),('lens_flare_intensity',0),('motion_blur_amount',0),('scene_fringe_intensity',0),('color_contrast',unreal.Vector4(1.02,1.02,1.02,1))]:
 settings.set_editor_property('override_'+name,True);settings.set_editor_property(name,value)
volumes=[a for a in actors.get_all_level_actors() if a.actor_has_tag('RingDaylight')]
volume=volumes[0] if volumes else actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector(0,0,0))
volume.tags=['RingDaylight'];volume.set_actor_label('Readable Baltic daylight')
volume.set_editor_property('unbound',True);volume.set_editor_property('priority',10);volume.set_editor_property('settings',settings)
for actor in actors.get_all_level_actors():
 if isinstance(actor,unreal.CameraActor):actor.get_component_by_class(unreal.CameraComponent).set_editor_property('post_process_settings',settings)
edit=unreal.MaterialEditingLibrary
for name,roughness in [('Road',.78),('LeftApron',.95),('RightApron',.95)]:
 material=unreal.load_asset('/Game/Migration/M_'+name)
 for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,roughness),(unreal.MaterialProperty.MP_SPECULAR,.25),(unreal.MaterialProperty.MP_METALLIC,0)]:
  node=edit.get_material_property_input_node(material,prop)
  if not isinstance(node,unreal.MaterialExpressionConstant):node=edit.create_material_expression(material,unreal.MaterialExpressionConstant)
  node.set_editor_property('r',value);edit.connect_material_property(node,'',prop)
 edit.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material)
levels.save_current_level()
unreal.log('Daylight: sun 6 lux, soft shadows, fixed exposure, no bloom/flare/motion blur, dry asphalt roughness 0.78')
