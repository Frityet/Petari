#!/usr/bin/env python3
from pathlib import Path
import re,json,hashlib,difflib
ROOT=Path(__file__).resolve().parents[3];NOTE=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-direct-draw-20260903';stage=BUILD/'staged/compat/OriginalDirectDraw.cpp'
source=(ROOT/'src/Game/Util/DirectDraw.cpp').read_text()
wanted={'setViewMtx','loadViewMtx','setModelMtx','resetViewMtx','close','setup','sendPoint','drawFillCircle','drawTexture3D','drawFillBox3D','cameraInit3D','cameraInit2D','fix2Dpos'}
functions=[]
for match in re.finditer(r'^    void (\w+)\([^;]*?\) \{',source,re.M):
 if match[1]not in wanted:continue
 begin=match.start();at=source.index('{',begin);depth=1;end=at+1
 while depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 functions.append((match[1],source[begin:end]))
assert len(functions)==15,len(functions)
headers=source[:source.index('namespace {')]
text=headers+'namespace {\n    static Mtx mViewMtx;\n}\n\nnamespace TDDraw {\n'+'\n\n'.join(s for _,s in functions)+'\n}\n'
stage.parent.mkdir(parents=True,exist_ok=True);stage.write_text(text)
records=[dict(name=name,body_sha256=hashlib.sha256(body.encode()).hexdigest())for name,body in functions]
(NOTE/'source-correspondence.json').write_text(json.dumps(dict(source='src/Game/Util/DirectDraw.cpp',native='pc-port/src/compat/OriginalDirectDraw.cpp',unchanged_bodies=records),indent=2)+'\n')
dest=ROOT/'pc-port/src/compat/OriginalDirectDraw.cpp';old=dest.read_text()if dest.exists()else''
patch=''.join(difflib.unified_diff(old.splitlines(True),text.splitlines(True),fromfile='a/src/compat/OriginalDirectDraw.cpp'if dest.exists()else'/dev/null',tofile='b/src/compat/OriginalDirectDraw.cpp'))
(BUILD/'native.patch').write_text(patch);(NOTE/'native.patch').write_text(patch)
print('Staged 15 root-exact full bodies')
