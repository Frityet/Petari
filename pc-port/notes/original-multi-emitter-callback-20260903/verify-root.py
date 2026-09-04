#!/usr/bin/env python3
from pathlib import Path
import importlib.util, json, subprocess, hashlib, re
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-multi-emitter-callback-20260903';B.mkdir(exist_ok=True)
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,R/path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
c=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
e=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
retail=R/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Effect/MultiEmitterCallBack.o'
commands=[]
for name,source in [('baseline',N/'baseline/MultiEmitterCallBack.cpp'),('current',R/'src/Game/Effect/MultiEmitterCallBack.cpp')]:
    cmd=c.compiler('cflags_game')+['-c',str(source),'-o',str(B/(name+'.o'))]
    result=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/(name+'-compile.log')).write_text(result.stdout+result.stderr);result.check_returncode();commands.append(cmd)
subprocess.run([R/'build/tools/objdiff-cli','diff','-1',retail,'-2',B/'current.o','-o',B/'diff.json','--format','json-pretty'],check=True,capture_output=True)
diff=json.loads((B/'diff.json').read_text());old=e.Elf(B/'baseline.o');new=e.Elf(B/'current.o');orig=e.Elf(retail)
def code(elf,name):
    _,start,size,index=next(x for x in elf.symbols if x[0]==name);return elf.section_data(index)[start:start+size]
def refs(elf,name):
    return [{k:v for k,v in x.items() if k!='symbol' or not x.get('value_hex')} for x in elf.references(name)]
selected=['isFollowSRT__20MultiEmitterCallBackCFPQ220MultiEmitterCallBack7FlagSRTb','setColor__20MultiEmitterCallBackFP14JPABaseEmitter','setSRTFromHostSRT__20MultiEmitterCallBackFP14JPABaseEmitterRCQ220MultiEmitterCallBack7FlagSRTb']
addresses={name:(int(address,16),int(size,16)) for name,address,size in re.findall(r'^([^\n]+?) = \.text:(0x[0-9A-Fa-f]+);.*?size:(0x[0-9A-Fa-f]+)',(R/'config/RMGK01/symbols.txt').read_text(),re.M)}
dol=c.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
name=selected[0];address,size=addresses[name];assert not new.references(name);assert code(new,name)==code(orig,name)==e.dol_bytes(dol,address,size)
# Existing implementation bytes and external/data references remain unchanged.
unchanged=[]
for name,start,size,index in old.symbols:
    if size and index and (old.sections[index][2] & 4) and name not in selected:
        assert code(old,name)==code(new,name),name
        assert refs(old,name)==refs(new,name),name
        unchanged.append(name)
rows=[]
for name in selected:
    info=next(x for x in diff['left']['symbols'] if x['name']==name)
    rows.append(dict(symbol=name,retail_address=hex(addresses[name][0]),retail_bytes=addresses[name][1],compiled_bytes=len(code(new,name)),match_percent=info['match_percent'],byte_exact=name==selected[0],retail_references=orig.references(name),compiled_references=new.references(name)))
for label,path in [('retail',retail),('compiled',B/'current.o')]:
    result=subprocess.run(['/opt/homebrew/opt/llvm/bin/llvm-objdump','-dr',path],capture_output=True,text=True);result.check_returncode();(N/(label+'.asm')).write_text(result.stdout)
(N/'root-evidence.json').write_text(json.dumps(dict(commands=commands,dol_sha1=hashlib.sha1(dol).hexdigest(),source_sha256=c.sha(R/'src/Game/Effect/MultiEmitterCallBack.cpp'),selected=rows,unchanged_methods=unchanged),indent=2)+'\n')
print('Recovered three complete methods; exact follow flags:',addresses[selected[0]][1],'bytes; other methods preserved:',len(unchanged));print([(x['symbol'],x['match_percent']) for x in rows])

# Missing original metadata query required by the callback virtual path.
name='isEffect2D__Q22MR6EffectFPC12MultiEmitter'
source=R/'src/Game/Effect/EffectSystemUtil.cpp';obj=B/'EffectSystemUtil.o'
cmd=c.compiler('cflags_game')+['-c',str(source),'-o',str(obj)]
result=subprocess.run(cmd,cwd=R,capture_output=True,text=True);result.check_returncode()
compiled=e.Elf(obj);address,size=addresses[name]
assert not compiled.references(name)
assert code(compiled,name)==e.dol_bytes(dol,address,size)
base=B/'EffectSystemUtil-baseline.o'
result=subprocess.run(c.compiler('cflags_game')+['-c',str(N/'baseline/EffectSystemUtil.cpp'),'-o',str(base)],cwd=R,capture_output=True,text=True);result.check_returncode()
prior=e.Elf(base);preserved=[]
for symbol,start,length,index in prior.symbols:
    if length and index and (prior.sections[index][2]&4):
        assert code(prior,symbol)==code(compiled,symbol),symbol
        assert refs(prior,symbol)==refs(compiled,symbol),symbol
        preserved.append(symbol)
(N/'metadata-query-evidence.json').write_text(json.dumps(dict(command=cmd,symbol=name,retail_address=hex(address),bytes=size,byte_exact=True,source_sha256=c.sha(source),unchanged_methods=preserved),indent=2)+'\n')
print('Original 2D metadata query:',size,'bytes exact; existing utility methods preserved:',len(preserved))
