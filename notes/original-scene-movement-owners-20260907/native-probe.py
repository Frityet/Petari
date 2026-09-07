from pathlib import Path
import hashlib,json,subprocess
root=Path(__file__).resolve().parents[2];note=Path(__file__).resolve().parent
base=json.loads((root/'notes/original-mario-owner-activation-20260907/Mario-compile.json').read_text())['command'];base=base[:base.index('-o')]
sources=['src/Game/NameObj/NameObjHolder.cpp','src/Game/Scene/StopSceneController.cpp','src/Game/Scene/SceneNameObjMovementController.cpp','src/scene/SceneNameObjRegistry.cpp','src/compat/SceneMovementOwnerLifetime.cpp','src/compat/SceneNameObjUtilCompat.cpp','src/compat/NameObjLifetimeCompat.cpp','src/Game/LiveActor/LiveActor.cpp','src/scene/StageInitializationService.cpp']
results=[]
for source in sources:
 p=root/source;cmd=base+['-o',str(note/(p.stem+'.o')),str(p)];log=note/(p.stem+'-native.log')
 with log.open('w') as f:r=subprocess.run(cmd,cwd=root,stdout=f,stderr=subprocess.STDOUT)
 results.append({'source':source,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'exit':r.returncode,'command':cmd});print(source,r.returncode,flush=True)
 if r.returncode:print('\n'.join(x for x in log.read_text().splitlines() if 'error:' in x),flush=True)
(note/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
