from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json, subprocess
notes=Path('notes/original-camera-director-lifecycle-20260907')
base=json.loads(Path('notes/original-star-pointer-depth-20260907/original-depth-final-native-command.json').read_text())['command']
args=[];skip=False
for i,x in enumerate(base):
 if skip:skip=False;continue
 if x in ['-o','-MF']:skip=True;continue
 if x in ['-c','-MMD']:continue
 if x.endswith('.cpp'):continue
 if 'imgui/v1.91.9b-docking/' in x:
  x=x.replace('0ee3cc5024224d4da4704fa6c643e242','493627e30eb74b3095870a76bfff9885')
 args.append(x)
sources=['src/Game/Screen/ImageEffectLocalUtil.cpp','src/Game/Camera/CameraShaker.cpp','src/compat/OriginalImageEffectOwnerQuery.cpp','src/compat/SceneMovementCompat.cpp','src/camera/CameraDirectorRuntime.cpp','src/compat/SceneObjHolderCompat.cpp','src/compat/CameraLocalUtilRuntime.cpp','src/compat/CameraUtilCompat.cpp','src/compat/OriginalCameraOwnerUtil.cpp','src/runtime/RuntimeServices.cpp','src/runtime/SceneScheduler.cpp','src/scene/GatewayDemoScene.cpp','src/scene/StageHostScene.cpp','src/showcase/Showcase.cpp','src/Game/Camera/CameraTargetArg.cpp']
def run(source):
 cmd=args+['-Isrc/app','-fsyntax-only',source]
 r=subprocess.run(cmd,text=True,capture_output=True)
 log=notes/(Path(source).stem+'-syntax.log');log.write_text(r.stdout+r.stderr)
 print(source,r.returncode,flush=True)
 return {'source':source,'command':cmd,'exit_code':r.returncode,'log':str(log)}
with ThreadPoolExecutor(max_workers=3) as pool: results=list(pool.map(run,sources))
(notes/'native-syntax-results.json').write_text(json.dumps(results,indent=2)+'\n')
