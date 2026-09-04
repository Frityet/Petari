#!/usr/bin/env python3
from pathlib import Path
import difflib,json,hashlib,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-creator-20260903';S=B/'staged'
rootfiles=['src/Game/Map/PlanetMapCreator.cpp','include/Game/Map/PlanetMap.hpp'];patch=''
for name in rootfiles:
 old=(B/'before'/name).read_text();new=(R/name).read_text();patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
(N/'root.patch').write_text(patch)
records=[];patch=''
for source in sorted(S.rglob('*')):
 if not source.is_file():continue
 rel=source.relative_to(S);dest=R/'pc-port/src'/rel;old=dest.read_text() if dest.exists() else '';new=source.read_text();patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+str(dest.relative_to(R)) if dest.exists() else '/dev/null',tofile='b/'+str(dest.relative_to(R))))
 records.append(dict(staged=str(source.relative_to(R)),destination=str(dest.relative_to(R)),sha256=hashlib.sha256(source.read_bytes()).hexdigest(),production_before_sha256=hashlib.sha256(dest.read_bytes()).hexdigest() if dest.exists() else None))
(N/'native-proposal.patch').write_text(patch);(N/'native-manifest.json').write_text(json.dumps(records,indent=2)+'\n')
source=(R/rootfiles[0]).read_text();staged=(S/'Game/Map/PlanetMapCreator.cpp').read_text();marker='class PlanetMapFarClippable';assert source[source.index(marker):]==staged[staged.index(marker):]
for name in ['PlanetMap.hpp','PlanetMapCreator.hpp']:assert (R/'include/Game/Map'/name).read_bytes()==(S/'Game/Map'/name).read_bytes()
(N/'source-evidence.json').write_text(json.dumps(dict(root_sources=[dict(path=p,sha256=hashlib.sha256((R/p).read_bytes()).hexdigest()) for p in rootfiles],native_all_method_class_and_table_bodies_literal=True,native_headers_root_identical=True,patch_scope='Compile proposal only; no provider activation'),indent=2)+'\n')
subprocess.run(['git','apply','--check',str(N/'native-proposal.patch')],cwd=R,check=True)
print('Frozen root patch, native proposal, manifest and source identity evidence.')
