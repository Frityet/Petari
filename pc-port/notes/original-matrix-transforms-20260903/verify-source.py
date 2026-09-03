#!/usr/bin/env python3
from pathlib import Path
import importlib.util,subprocess,json,hashlib
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-matrix-transforms-20260903'
spec=importlib.util.spec_from_file_location('compiler',ROOT/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');compiler=importlib.util.module_from_spec(spec);spec.loader.exec_module(compiler)
BUILD.mkdir(parents=True,exist_ok=True)
source=(ROOT/'src/Game/Util/MtxUtil.cpp').read_text();native=(ROOT/'pc-port/src/compat/OriginalMatrixTransforms.cpp').read_text()
names=['blendMtxRotate','blendMtxRotateSlerp','blendMtx','makeMtxWithoutScale','addTransMtx','addTransMtxLocal','addTransMtxLocalX','addTransMtxLocalY','addTransMtxLocalZ','tmpMtxTrans','tmpMtxScale','tmpMtxRotXRad','tmpMtxRotYRad','tmpMtxRotZRad','tmpMtxRotXDeg','tmpMtxRotYDeg','tmpMtxRotZDeg','orderRotateMtx']
def method(text,name):
    a=text.index(('    MtxPtr ' if name.startswith('tmp') else '    void ')+name+'(');b=text.index('{',a)+1;depth=1
    while depth:
        depth+=(text[b]=='{')-(text[b]=='}');b+=1
    return text[a:b]
for name in names:assert method(source,name)==method(native,name),name
assert source[:source.index('namespace MR {')]==native[:native.index('namespace MR {')]
command=compiler.compiler('cflags_game')+['-c',str(ROOT/'src/Game/Util/MtxUtil.cpp'),'-o',str(BUILD/'MtxUtil.o')]
with (BUILD/'compile.log').open('w') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Util/MtxUtil.o'),'-2',str(BUILD/'MtxUtil.o'),'-o',str(BUILD/'diff.json'),'--format','json-pretty'],cwd=ROOT,check=True)
d=json.loads((BUILD/'diff.json').read_text());rows=[{k:s[k] for k in ['name','size','match_percent']} for s in d['left']['symbols'] if s['name'].split('__')[0] in names]
assert len(rows)==18
assert all(r['match_percent']>=99 for r in rows if not r['name'].startswith('blendMtx__'))
(NOTES/'source-evidence.json').write_text(json.dumps(dict(root_sha256=hashlib.sha256(source.encode()).hexdigest(),native_sha256=hashlib.sha256(native.encode()).hexdigest(),compiler_command=command,methods=rows),indent=2)+'\n')
print('PASS: 18 literal original matrix methods and shared temporary identities; original compiler evidence recorded')
