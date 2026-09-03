#!/usr/bin/env python3
from pathlib import Path
import difflib,hashlib,json,shutil
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-resource-loader-20260903';S=B/'staged'
shutil.copytree(S,N/'native',dirs_exist_ok=True)
for name in ['probe.cpp','block-probe.cpp','full-probe.cpp','extract_file.c','JPAResource-before.cpp']:
 shutil.copy2(B/name,N/name)
shutil.copy2(B/'full-probe-link.log',N/'full-probe-link.log')
shutil.copy2(B/'makeColorTable.asm',N/'makeColorTable.asm')
manifest=[];patches=[]
for src in sorted(S.rglob('*')):
 if not src.is_file():continue
 rel=src.relative_to(S);dest=R/'pc-port/src'/rel
 before=dest.read_text() if dest.exists() else ''
 after=src.read_text()
 patches.append(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),'a/'+str(dest.relative_to(R)) if dest.exists() else '/dev/null','b/'+str(dest.relative_to(R)))))
 manifest.append({'source':'native/'+str(rel),'destination':str(dest.relative_to(R)),'sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'current_destination_sha256':hashlib.sha256(dest.read_bytes()).hexdigest() if dest.exists() else None})
(N/'native.patch').write_text(''.join(patches));(N/'native-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
p=S/'JSystem/JGeometry/TVec.hpp';old=(R/'pc-port/src/JSystem/JGeometry/TVec.hpp').read_text();new=p.read_text();(N/'general-vector-header.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/pc-port/src/JSystem/JGeometry/TVec.hpp','b/pc-port/src/JSystem/JGeometry/TVec.hpp')))
old=(R/'src/JSystem/JParticle/JPABaseShape.cpp').read_text();new=(S/'JSystem/JParticle/JPABaseShape.cpp').read_text();(N/'native-color-guard.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/src/JSystem/JParticle/JPABaseShape.cpp','b/pc-port/src/JSystem/JParticle/JPABaseShape.cpp')))
print('Frozen',len(manifest),'native files; production activation intentionally gated')
