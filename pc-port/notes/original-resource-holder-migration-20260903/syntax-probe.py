#!/usr/bin/env python3
import concurrent.futures, json, subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
PC=ROOT/'pc-port'
DB=json.loads((PC/'compile_commands.json').read_text())
base=next(x for x in DB if x['file'].endswith('ResourceHolderCompat.cpp'))
files=['src/compat/ResourceHolderCompat.cpp','src/resource/GameResourceRuntime.cpp','src/Game/System/ResourceHolder.cpp','src/runtime/RuntimeServices.cpp','src/runtime/RuntimeContext.cpp','src/app/Application.cpp','src/compat/PlanetMapRuntimeCompat.cpp','src/compat/CollisionPartsCompat.cpp']
files += [str(x.relative_to(PC)) for d in ('tests','src/showcase','src/debug') for x in (PC/d).glob('*.cpp') if 'GameResourceRuntime' in x.read_text()]
def probe(file):
 entry=next((x for x in DB if x['file']==file),base)
 args=[];it=iter(entry['arguments'])
 for arg in it:
  if arg=='-o': next(it);continue
  if arg=='-c' or arg==entry['file']:continue
  args.append(arg)
 args += ['-fsyntax-only',file]
 run=subprocess.run(args,cwd=PC,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
 return {'file':file,'status':run.returncode,'command':args,'output':run.stdout}
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
 results=list(pool.map(probe,files))
(HERE/'syntax-results.json').write_text(json.dumps(results,indent=2)+'\n')
for x in results:
 print(x['file'],x['status'])
 if x['status']:print(x['output'])
raise SystemExit(any(x['status'] for x in results))
