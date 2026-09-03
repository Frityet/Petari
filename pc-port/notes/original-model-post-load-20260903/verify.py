#!/usr/bin/env python3
"""Compile the original ModelUtil unit, verify retail relocations and exact imports."""
import ast, hashlib, importlib.util, json, re, shlex, struct, subprocess, types
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
BUILD=ROOT/'build/original-model-post-load-20260903';BUILD.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('original',HERE.parent/'original-only-camera-20260903/verify-object.py')
original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
read=lambda addr,size:original.dol_bytes(dol,addr,size)
assert read(0x80004224,16)==bytes.fromhex('3c40806b6042fc203da0806b61ad9620')
bases={2:0x806bfc20,13:0x806b9620}
source=(ROOT/'src/Game/Util/ModelUtil.cpp').read_text()
def body(text,name):
    match=re.search(r'^    (?:inline )?(?:bool|void) '+re.escape(name)+r'\(',text,re.M)
    assert match,name
    start=match.start();brace=text.index('{',start);depth=0
    for end in range(brace,len(text)):
        if text[end]=='{':depth+=1
        if text[end]=='}':
            depth-=1
            if not depth:return text[start:end+1]
    raise AssertionError(name)
imports={
 'pc-port/src/compat/ModelPostLoadCompat.cpp': ['setShapeVcdVatCmdSelf','initEnvelopeAndEnvMapOrProjMapModelData','downFracVtx','isUseFur'],
 'pc-port/src/compat/MaterialTextureModeCompat.cpp': ['isEnvelope','isUseTexMtx','isUseTexMtxEnvMap','isUseTexMtxProjMap']}
for path,names in imports.items():
    native=(ROOT/path).read_text()
    for name in names:assert body(source,name)==body(native,name),name
    print('PASS exact imported bodies:',path)
for node in ast.parse((ROOT/'configure.py').read_text()).body:
    if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cflags_game' for t in node.targets):
        flags=eval(compile(ast.Expression(node.value),'configure.py','eval'),{'config':types.SimpleNamespace(version='RMGK01'),'version_num':0});break
command=['build/tools/wibo','build/tools/sjiswrap.exe','build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags:command.extend(shlex.split(flag))
command+=['-c','src/Game/Util/ModelUtil.cpp','-o',str(BUILD/'ModelUtil.o')]
def run(cmd,log):
    with (HERE/log).open('w') as output:
        subprocess.run([str(x) for x in cmd],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True,timeout=60)
run(command,'original-build.log')
run(['build/tools/objdiff-cli','diff','-1','build/original-resource-holder-20260903/retail/obj/Game/Util/ModelUtil.o',
     '-2',BUILD/'ModelUtil.o','-o',BUILD/'objdiff.json','--format','json-pretty'],'objdiff.log')
objdiff=json.loads((BUILD/'objdiff.json').read_text())
symbols={}
for line in (ROOT/'config/RMGK01/symbols.txt').read_text().splitlines():
    match=re.match(r'(.+?) = \.\w+:0x([0-9A-Fa-f]+);',line)
    if match:symbols[match[1]]=int(match[2],16)
elf=original.Elf(BUILD/'ModelUtil.o');records=[]
methods=[name for group in imports.values() for name in group if name!='setShapeVcdVatCmdSelf']
for method in methods:
    symbol=next(s for s in elf.symbols if s[0].startswith(method+'__2MR'))
    name,start,size,index=symbol;address=symbols[name]
    data=bytearray(elf.section_data(index)[start:start+size]);relocations=[]
    for section in elf.sections:
        if section[1]!=4 or section[7]!=index:continue
        for offset in range(section[4],section[4]+section[5],section[9]):
            location,info,addend=struct.unpack_from('>IIi',elf.data,offset)
            relative=location-start
            if not 0<=relative<size:continue
            kind=info&255;target_symbol=elf.symbols[info>>8]
            target_name,target_value,target_size,target_index=target_symbol
            old=struct.unpack_from('>I',data,relative)[0]
            if kind==10:
                target=symbols[target_name]+addend
                displacement=target-(address+relative)
                assert -0x2000000<=displacement<0x2000000
                new=(old&0xfc000003)|(displacement&0x03fffffc)
            elif kind==109:
                # Derive the effective retail SDA address from the actual
                # argument instruction, then verify the compiled string there.
                retail=struct.unpack('>I',read(address+relative,4))[0]
                base=(retail>>16)&31;immediate=retail&65535
                if immediate&32768:immediate-=65536
                target=bases[base]+immediate
                assert method=='isUseFur' and base==13 and (retail>>21)&31==4
                compiled=elf.section_data(target_index)[target_value+addend:target_value+addend+4]
                assert compiled==read(target,4)==b'Fur\0'
                new=(old&0xffe00000)|(base<<16)|(immediate&65535)
            else:raise AssertionError((method,kind,target_name))
            struct.pack_into('>I',data,relative,new)
            relocations.append({'offset':hex(relative),'kind':kind,'symbol':target_name,'target':hex(target)})
    assert bytes(data)==read(address,size),method
    left=next(s for s in objdiff['left']['symbols'] if s['name']==name)
    records.append({'method':method,'address':hex(address),'size':size,'objdiff_match_percent':left['match_percent'],
                    'relocated_bytes_equal':True,'relocations':relocations})
    print('PASS retail bytes:',method,size)
probe=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-DAURORA','-Wno-inconsistent-missing-override',
       '-Wno-multichar','-include','pc-port/src/compat/MetrowerksStdCompat.hpp','-Ipc-port/src','-Ipc-port/aurora/include',
       '-fsyntax-only','pc-port/src/compat/ModelPostLoadCompat.cpp']
run(probe,'native-syntax.log')
result={'dol_sha1':hashlib.sha1(dol).hexdigest(),'original_command':command,'native_syntax_command':probe,'methods':records,
 'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ['src/Game/Util/ModelUtil.cpp',*imports]}}
(HERE/'source-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
