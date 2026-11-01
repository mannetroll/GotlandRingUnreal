"""Run rendered Unity parity checks against a self-contained macOS game."""
from pathlib import Path
import json,shutil,subprocess,sys,zlib
root=Path(__file__).resolve().parents[1]
app=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root.parent/'Releases/complete-macOS/Mac/GotlandRingUnreal.app'
exe=app/'Contents/MacOS/GotlandRingUnreal-Mac-Shipping'
saved=Path.home()/'Library/Containers/com.YourCompany.GotlandRingUnreal/Data/Library/Application Support/Epic/GotlandRingUnreal/Saved'
for mode in ['Complete','Migration','Instruments','Settings','Driving','Wind','FrameRate']:
 stem='frame_rate' if mode=='FrameRate' else mode.lower();report=saved/(stem+'_validation.json');report.unlink(missing_ok=True)
 log=root/'Saved'/('complete_'+stem+'_mac.log');log.parent.mkdir(exist_ok=True)
 with log.open('w') as output:
  result=subprocess.run([str(exe),'-Ring'+mode+'Test','-windowed','-ResX=1600','-ResY=900'],stdout=output,stderr=output,timeout=150)
 data=json.loads(report.read_text());shutil.copyfile(report,root/'Validation'/(stem+'_complete_macos.json'));assert data['passed'],data
 print(mode,'PASS',flush=True)
 if mode=='Complete':
  desktop=Path(data['desktop_png']);png=desktop.read_bytes();assert png[:8]==b'\x89PNG\r\n\x1a\n';offset=8;metadata={}
  while offset<len(png):
   size=int.from_bytes(png[offset:offset+4],'big');tag=png[offset+4:offset+8];body=png[offset+8:offset+8+size];crc=int.from_bytes(png[offset+8+size:offset+12+size],'big');assert zlib.crc32(tag+body)==crc
   if tag==b'iTXt':key,text=body.split(b'\0',1);assert text[:4]==b'\0'*4;metadata[key.decode()]=text[4:].decode('utf-8')
   offset+=size+12
  assert {'Software','Creation Time','Description','HUD','Vehicle','Camera'}<=metadata.keys()
  (root/'Validation/desktop_png_macos.json').write_text(json.dumps(dict(passed=True,crc_verified=True,metadata=metadata),indent=2)+'\n')
  shutil.copyfile(desktop,root/'Renders/DesktopCaptureMac.png');desktop.unlink()
  for name in ['RallyPaintRed','RallyPaintBlue','RallyPaintWhite','SubaruBlueGold','TrainingArea']:
   shutil.copyfile(saved/(name+'.png'),root/'Renders'/(name+'Mac.png'))
print('ALL COMPLETE MACOS CHECKS PASSED',flush=True)
