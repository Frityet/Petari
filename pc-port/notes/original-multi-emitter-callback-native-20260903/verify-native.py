#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, shutil, os, hashlib
R=Path(__file__).resolve().parents[3]; N=Path(__file__).resolve().parent
B=R/'build/original-multi-emitter-callback-native-20260903'; S=B/'staged'
S.mkdir(parents=True,exist_ok=True)
manifest=[]
def copy(src,dest):
 dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dest)
 manifest.append({'source':str(src.relative_to(R)),'staged':str(dest.relative_to(R)),'sha256':hashlib.sha256(src.read_bytes()).hexdigest()})
for name in ['MultiEmitter','MultiEmitterCallBack','SingleEmitter','MultiEmitterParticleCallBack','AutoEffectInfo']:
 for suffix,root in [('cpp','src'),('hpp','include')]:
  copy(R/root/f'Game/Effect/{name}.{suffix}',S/f'Game/Effect/{name}.{suffix}')
P=R/'pc-port/notes/original-particle-resource-owner-20260903/native'
for src in P.rglob('*'):
 if src.is_file():copy(src,S/src.relative_to(P))
helper=S/'compat/OriginalEffect2D.cpp'; helper.parent.mkdir(parents=True,exist_ok=True)
root=(R/'src/Game/Effect/EffectSystemUtil.cpp').read_text()
start=root.index('bool isEffect2D('); brace=root.index('{',start); depth=1; end=brace+1
while depth:
 depth += (root[end]=='{')-(root[end]=='}');end+=1
helper.write_text('#include "Game/Effect/EffectSystemUtil.hpp"\n#include "Game/Effect/MultiEmitter.hpp"\n#include "Game/Effect/AutoEffectInfo.hpp"\nnamespace MR { namespace Effect {\n'+root[start:end]+'\n} }\n')
manifest.append({'source':'src/Game/Effect/EffectSystemUtil.cpp::MR::Effect::isEffect2D','staged':str(helper.relative_to(R)),'sha256':hashlib.sha256(helper.read_bytes()).hexdigest()})
string_source=(R/'src/Game/Util/StringUtil.cpp').read_text()
string_bodies=[]
for signature in ['bool hasStringSpace(', 'bool isDigitStringTail(']:
 start=string_source.index(signature);brace=string_source.index('{',start);end=brace+1;depth=1
 while depth:
  depth += (string_source[end]=='{')-(string_source[end]=='}');end+=1
 string_bodies.append(string_source[start:end])
string_helper=S/'compat/OriginalEmitterStringQueries.cpp'
string_helper.write_text('#include "Game/Util/StringUtil.hpp"\n#include <cctype>\n#include <cstring>\nnamespace MR {\n'+'\n\n'.join(string_bodies)+'\n}\n')
manifest.append({'source':'src/Game/Util/StringUtil.cpp::hasStringSpace,isDigitStringTail','staged':str(string_helper.relative_to(R)),'sha256':hashlib.sha256(string_helper.read_bytes()).hexdigest()})
base=json.loads((R/'pc-port/notes/original-effect-sync-checker-20260903/native-compiles.json').read_text())[0]['command'][:-4]
base=[x.replace(str(R/'build/original-effect-sync-checker-20260903/staged'),str(S)) for x in base]
sources=list(S.rglob('*.cpp'))+[R/'pc-port/src/Game/Effect/ParticleResourceHolder.cpp',N/'probe.cpp']
# Instrument the actual SDK construction/transform code as well as all Game
# callback code. Other already-built libraries remain ordinary debug objects.
sources += [R/'pc-port/src/JSystem/JParticle'/f'{name}.cpp' for name in ['JPAEmitterManager','JPAEmitter','JPAMath']]
rows=[];objects=[]
def run(cmd,label):
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True)
 (B/(label+'.log')).write_text(p.stdout+p.stderr)
 rows.append({'command':cmd,'returncode':p.returncode})
 (N/'native-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 return p
for src in sources:
 out=B/(src.stem+'.o');objects.append(str(out))
 run(base+['-fsanitize=address,undefined','-fno-omit-frame-pointer','-c',str(src),'-o',str(out)],src.stem+'-compile')
old=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/probe-link.json').read_text())['command']
cmd=[old[0],'-fsanitize=address,undefined','-fno-omit-frame-pointer']+objects+old[old.index('-Wl,-dead_strip'):]
cmd[cmd.index('-o')+1]=str(B/'probe');run(cmd,'probe-link')
(N/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
env=os.environ.copy();env['SMGPC_REAL_DISC']=str(next(R.glob('*.rvz')))
env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1';env['ASAN_OPTIONS']='halt_on_error=1'
p=subprocess.run([str(B/'probe')],cwd=R,env=env,capture_output=True,text=True,timeout=60)
(N/'probe-asan.log').write_text(p.stdout+p.stderr); print(p.stdout,p.stderr)
(N/'runtime-proof.json').write_text(json.dumps({'command':[str(B/'probe')],'disc':Path(env['SMGPC_REAL_DISC']).name,'ASAN_OPTIONS':env['ASAN_OPTIONS'],'UBSAN_OPTIONS':env['UBSAN_OPTIONS'],'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr},indent=2)+'\n')
p.check_returncode()
