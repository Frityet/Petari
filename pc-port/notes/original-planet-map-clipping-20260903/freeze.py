#!/usr/bin/env python3
from pathlib import Path
import difflib,json,hashlib,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-clipping-20260903';S=B/'staged'
root=['src/Game/Map/PlanetMap.cpp','include/Game/Map/PlanetMap.hpp'];patch=''
for name in root:patch+=''.join(difflib.unified_diff((B/'before'/name).read_text().splitlines(True),(R/name).read_text().splitlines(True),fromfile='a/'+name,tofile='b/'+name))
(N/'root.patch').write_text(patch)
patch='';records=[]
for source in sorted(S.rglob('*')):
 if not source.is_file():continue
 rel=source.relative_to(S);dest=R/'pc-port/src'/rel;old=dest.read_text() if dest.exists() else '';new=source.read_text();patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+str(dest.relative_to(R)) if dest.exists() else '/dev/null',tofile='b/'+str(dest.relative_to(R))))
 records.append(dict(staged=str(source.relative_to(R)),destination=str(dest.relative_to(R)),sha256=hashlib.sha256(source.read_bytes()).hexdigest(),production_before_sha256=hashlib.sha256(dest.read_bytes()).hexdigest() if dest.exists() else None))
(N/'native.patch').write_text(patch);(N/'native-manifest.json').write_text(json.dumps(records,indent=2)+'\n')
assert (R/root[0]).read_bytes()==(S/'Game/Map/PlanetMap.cpp').read_bytes();assert (R/root[1]).read_bytes()==(S/'Game/Map/PlanetMap.hpp').read_bytes()
(N/'source-evidence.json').write_text(json.dumps(dict(root_files=[dict(path=p,sha256=hashlib.sha256((R/p).read_bytes()).hexdigest()) for p in root],native_complete_source_and_header_literal=True,root_header_fallback=False),indent=2)+'\n')
subprocess.run(['git','apply','--check',str(N/'native.patch')],cwd=R,check=True);print('Frozen narrow root/native patches and literal source identity evidence.')
