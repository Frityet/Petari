#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,subprocess,difflib,re
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-collision-point-query-20260903';STAGE=BUILD/'staged';PREVIOUS=ROOT/'build/original-collision-scene-owner-20260903/staged'
files=['Game/Map/'+u+'.cpp' for u in ('CollisionCategorizedKeeper','CollisionParts','KCollision','KCollisionPlus')]
rows=[];patches={'native.patch':[],'native-incremental.patch':[]}
base=json.loads((ROOT/'pc-port/notes/original-map-query-access-20260903/native-evidence.json').read_text())[0]['command'];base=base[:base.index('-c')]
base=[f for f in base if 'original-map-query-access-20260903/staged' not in f and 'original-collision-scene-owner-20260903/staged' not in f]
base[1:1]=['-I'+str(STAGE),'-I'+str(ROOT/'pc-port/src'),'-I'+str(PREVIOUS)]
for file in files:
 text=(ROOT/'src'/file).read_text()
 if file.endswith('/KCollision.cpp'):
  text='#include "resource/KCollisionResource.hpp"\n'+text
  text=re.sub(r'void KCollisionServer::setData\(void\* pData\) \{.*?\n\}', 'void KCollisionServer::setData(void* pData) {\n    mFile = smgpc::resource::require_native_kcollision_file(pData);\n}',text,count=1,flags=re.S)
  text=text.replace('    return reinterpret_cast< const s32* >(pData)[0] < 0;', '    return smgpc::resource::is_native_kcollision_file(pData);')
 path=STAGE/file;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text);out=BUILD/(path.stem+'.o');cmd=base+['-c',str(path),'-o',str(out)]
 with (NOTES/(path.stem+'-compile.log')).open('w') as log:r=subprocess.run(cmd,cwd=ROOT/'pc-port',stdout=log,stderr=subprocess.STDOUT)
 rows.append({'file':file,'returncode':r.returncode,'command':cmd,'source_sha256':hashlib.sha256(text.encode()).hexdigest()});print(file,r.returncode)
 for name,old in [('native.patch',ROOT/'pc-port/src'/file),('native-incremental.patch',PREVIOUS/file)]:
  patches[name].append(''.join(difflib.unified_diff(old.read_text().splitlines(True) if old.exists() else [],text.splitlines(True),fromfile='a/pc-port/src/'+file if old.exists() else '/dev/null',tofile='b/pc-port/src/'+file)))
(NOTES/'native-evidence.json').write_text(json.dumps(rows,indent=2)+'\n')
for name,parts in patches.items():(NOTES/name).write_text(''.join(parts))
(NOTES/'root.patch').write_text(subprocess.check_output(['git','diff','--',*[str(ROOT/'src'/f) for f in files]],text=True))
assert all(r['returncode']==0 for r in rows)
