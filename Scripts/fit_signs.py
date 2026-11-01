"""Fit every Unity sign label at its centred source anchor in the current level."""
import json,re
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
labels=json.loads((root/'Export/Scenery/scenery.json').read_text())['labels']
names={json.loads(m) for m in re.findall(r'TEXT\(("(?:[^"\\]|\\.)*")\)',(root/'Source/GotlandRingUnreal/RingPresentationData.h').read_text())}
labels={s['text']:s for s in labels if s['text'] in names}
material=unreal.load_asset('/Game/Migration/Scenery/M_SignText')
count=0
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if not isinstance(actor,unreal.TextRenderActor):continue
 text=actor.get_component_by_class(unreal.TextRenderComponent)
 value=str(text.get_editor_property('text')).replace('<br>','\n')
 if value not in labels:continue
 label=labels[value];number=value.isdigit()
 text.set_text(value.replace('\n','<br>'));text.set_world_size(100)
 text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
 text.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
 size=text.get_text_local_size();width=118 if number else 630;height=120 if number else 170 if '\n' in value else 85
 text.set_world_size(100*min(width/size.y,height/size.z))
 text.set_text_render_color(unreal.Color(255,255,255,255));text.set_text_material(material)
 actor.set_actor_location(unreal.Vector(*label['position']),False,False)
 count+=1
assert count==84,count
unreal.log('Fitted all 84 sign labels at Unity anchors with unlit white lettering')
