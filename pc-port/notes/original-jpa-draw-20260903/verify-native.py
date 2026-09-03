#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,shutil,hashlib,sys
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-draw-20260903';S=B/'staged';P=R/'build/original-jpa-resource-loader-20260903'
S.mkdir(exist_ok=True)
for src in (P/'staged').rglob('*'):
 if src.is_file() and 'JGeometry' not in src.parts:
  dst=S/src.relative_to(P/'staged');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
src=R/'src/JSystem/JParticle/JPABaseShape.cpp'
(S/'JSystem/JParticle/JPABaseShape.cpp').write_text(src.read_text().replace('if (i == i_data[j].index)','if (j < param_2 && i == i_data[j].index)'))
s=(R/'src/Game/System/Overwrite.cpp').read_text()
headers='''#include "Game/Util/MathUtil.hpp"
#include "JSystem/JParticle/JPABaseShape.hpp"
#include "JSystem/JParticle/JPAEmitter.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include "JSystem/JParticle/JPAParticle.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"
#include <revolution/gx/GXVert.h>

'''
extract=headers+s[s.index('namespace {'):s.index('void JPABaseEmitter::init')]+s[s.index('void JPADrawDirection('):]
(S/'OriginalJPADraw.cpp').write_text(extract);(N/'OriginalJPADraw.cpp').write_text(extract)
shutil.copyfile(R/'pc-port/notes/original-jpa-overwrite-20260903/OriginalJPAFields.cpp',S/'OriginalJPAFields.cpp')
shutil.copyfile(R/'pc-port/notes/original-effect-construction-20260903/native/OriginalJPAEmitterInit.cpp',S/'OriginalJPAEmitterInit.cpp')
shutil.copyfile(N/'full-probe.cpp',S/'full-probe.cpp')
cmd=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
cmd=[a.replace(str(P/'staged'),str(S)) for a in cmd]
sanitize='--sanitize' in sys.argv;ownership='--ownership-only' in sys.argv;suffix=('-asan' if sanitize else '')+('-ownership' if ownership else '');flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else []
results=[];objects=[]
for src in sorted(S.rglob('*.cpp')):
 obj=B/(src.stem+suffix+'.o');c=cmd+flags+['-c',str(src),'-o',str(obj)]
 p=subprocess.run(c,cwd=R/'pc-port',capture_output=True,text=True);(B/(src.stem+suffix+'-compile.log')).write_text(p.stdout+p.stderr)
 results.append({'source':str(src.relative_to(S)),'sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'command':c,'returncode':p.returncode});print(src.stem,p.returncode,flush=True)
 if p.returncode: print(p.stdout,p.stderr);p.check_returncode()
 objects.append(str(obj))
old=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/full-probe-link.json').read_text())['command']
c=[old[0]]+flags+objects+old[old.index('-Wl,-dead_strip'):];c[c.index('-o')+1]=str(B/('full-probe'+suffix))
p=subprocess.run(c,cwd=R/'pc-port',capture_output=True,text=True);(B/('full-probe'+suffix+'-link.log')).write_text(p.stdout+p.stderr)
results.append({'command':c,'returncode':p.returncode});(N/('native-verification'+suffix+'.json')).write_text(json.dumps(results,indent=2)+'\n')
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
c=[str(B/('full-probe'+suffix)),str(P/'Effect.arc')]+(['--ownership-only'] if ownership else [])
p=subprocess.run(c,cwd=R,capture_output=True,text=True);(N/('full-probe'+suffix+'-runtime.log')).write_text(p.stdout+p.stderr)
results.append({'command':c,'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr});(N/('native-verification'+suffix+'.json')).write_text(json.dumps(results,indent=2)+'\n')
print(p.stdout,p.stderr);p.check_returncode()
