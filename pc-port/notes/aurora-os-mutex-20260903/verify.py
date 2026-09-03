#!/usr/bin/env python3
"""Compile original/native mutex providers and verify retail/source correspondence."""
import argparse, ast, hashlib, json, re, shlex, struct, subprocess, types
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT/'build/aurora-os-mutex-20260903'
GTEST = ROOT/'build/aurora-upstream-merge-tests'
TARGET = ROOT/'build/xanime-core-matrix-calculation-20260903/entrypoints/retail/obj/RVL_SDK/os/OSMutex.o'
FUNCTIONS = {'OSInitMutex':(0x804AAAC4,0x38), 'OSLockMutex':(0x804AAAFC,0xDC),
             'OSUnlockMutex':(0x804AABD8,0xC8), '__OSUnlockAllMutex':(0x804AACA0,0x6C),
             'OSTryLockMutex':(0x804AAD0C,0xBC)}
args=argparse.ArgumentParser();args.add_argument('--run',action='store_true');args=args.parse_args()
BUILD.mkdir(parents=True,exist_ok=True)
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def run(cmd, log):
    with (NOTES/log).open('w') as out:
        subprocess.run([str(x) for x in cmd],cwd=ROOT,stdout=out,stderr=subprocess.STDOUT,check=True,timeout=60)
if args.run:
    for node in ast.parse((ROOT/'configure.py').read_text()).body:
        if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cflags_sdk' for t in node.targets):
            flags=eval(compile(ast.Expression(node.value),'configure.py','eval'),{'config':types.SimpleNamespace(version='RMGK01'),'version_num':0});break
    cmd=['build/tools/wibo','build/tools/sjiswrap.exe','build/compilers/GC/3.0a5.2/mwcceppc.exe']
    for f in flags: cmd.extend(shlex.split(f))
    cmd.extend(['-c','src/RVL_SDK/os/OSMutex.c','-o',str(BUILD/'OSMutex-original.o')])
    (BUILD/'original-command.json').write_text(json.dumps(cmd,indent=2)+'\n')
    run(cmd,'original-compile.log')
    run(['build/tools/objdiff-cli','diff','-1',TARGET,'-2',BUILD/'OSMutex-original.o','-o',BUILD/'objdiff.json','--format','json-pretty'],'objdiff.log')
    for suffix, extra in [('',[]),('_tsan',['-fsanitize=thread'])]:
        executable=BUILD/('os_mutex_tests'+suffix)
        cmd=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++20','-g',*extra,'-DAURORA','-DTARGET_PC',
             '-Ipc-port/aurora/include',f'-I{GTEST}/_deps/googletest-src/googletest/include',
             'pc-port/aurora/lib/dolphin/os/OSExecution.cpp','pc-port/aurora/lib/dolphin/os/OSMutex.cpp',
             'pc-port/aurora/tests/os_mutex_test.cpp','pc-port/aurora/tests/os_execution_test.cpp',
             GTEST/'lib/libgtest_main.a',GTEST/'lib/libgtest.a','-pthread','-o',executable]
        run(cmd,'build'+suffix+'.log')
        run([executable,f'--gtest_output=xml:{NOTES}/tests{suffix}.xml'],'tests'+suffix+'.log')

# The root file is not reconstructed anew. Check the only native source edits.
expected=(ROOT/'src/RVL_SDK/os/OSMutex.c').read_text().replace('#include <revolution/os.h>',
    '#include <dolphin/os.h>\n#include "thread.hpp"\n\n#include <bit>\n#include <cassert>\n#define ASSERT(condition) assert(condition)')
expected=expected.replace('mutex->count++;','mutex->count = std::bit_cast<s32>(std::bit_cast<u32>(mutex->count) + 1U);')
expected=expected.replace('if (mutex->thread == currentThread && --mutex->count == 0) {',
    'if (mutex->thread == currentThread &&\n        (mutex->count = std::bit_cast<s32>(std::bit_cast<u32>(mutex->count) - 1U)) == 0) {')
assert expected.strip() == (ROOT/'pc-port/aurora/lib/dolphin/os/OSMutex.cpp').read_text().strip()

# Verify the previously split target against the current DOL, including relocated call targets.
dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
def at(addr,size):
    for i in range(18):
        offset,start,count=(struct.unpack_from('>I',dol,base+i*4)[0] for base in [0,0x48,0x90])
        if start<=addr and addr+size<=start+count:return dol[offset+addr-start:offset+addr-start+size]
    raise AssertionError(hex(addr))
e=TARGET.read_bytes(); shoff=struct.unpack_from('>I',e,0x20)[0]
shsize,shnum,shnames=struct.unpack_from('>HHH',e,0x2e)
sections=[struct.unpack_from('>10I',e,shoff+i*shsize) for i in range(shnum)]
def contents(s):return e[s[4]:s[4]+s[5]]
def string(table,index):return table[index:table.index(0,index)].decode()
names=contents(sections[shnames]); byname={string(names,s[0]):s for s in sections}
symsection=byname['.symtab']; strings=contents(sections[symsection[6]])
symbols=[]
for offset in range(symsection[4],symsection[4]+symsection[5],16):
    name,value,size,info,other,section=struct.unpack_from('>IIIBBH',e,offset)
    symbols.append((string(strings,name),value,size,section))
relocs={}
for offset in range(byname['.rela.text'][4],byname['.rela.text'][4]+byname['.rela.text'][5],12):
    position,info,addend=struct.unpack_from('>IIi',e,offset);relocs[position]=(info&255, symbols[info>>8][0],addend)
addresses={m.group(1):int(m.group(2),16) for m in re.finditer(r'^([^ =]+) = \.\w+:0x([0-9a-fA-F]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
text=contents(byname['.text']); verified_words=0
for name,address_size in FUNCTIONS.items():
    address,size=address_size
    _,offset,actual_size,_=next(s for s in symbols if s[0]==name)
    assert actual_size==size
    for i in range(0,size,4):
        targetword=struct.unpack_from('>I',text,offset+i)[0];liveword=struct.unpack('>I',at(address+i,4))[0]
        if offset+i in relocs:
            kind,symbol,addend=relocs[offset+i];assert kind==10
            assert liveword&0xFC000003==targetword&0xFC000003
            displacement=liveword&0x03FFFFFC
            if displacement&0x02000000:displacement-=0x04000000
            assert (address+i+displacement)&0xFFFFFFFF == (addresses[symbol]+addend)&0xFFFFFFFF
        else: assert liveword==targetword,(name,i)
        verified_words+=1
matches={s['name']:s['match_percent'] for s in json.loads((BUILD/'objdiff.json').read_text())['left']['symbols'] if s['name'] in FUNCTIONS}
assert set(matches)==set(FUNCTIONS) and all(x==100.0 for x in matches.values())
sources=['src/RVL_SDK/os/OSMutex.c','src/RVL_SDK/os/OSThread.c','pc-port/aurora/lib/dolphin/os/OSExecution.cpp',
         'pc-port/aurora/lib/dolphin/os/OSMutex.cpp','pc-port/aurora/lib/dolphin/os/thread.hpp','pc-port/aurora/tests/os_mutex_test.cpp']
results={}
for suffix in ['', '_tsan']:
    xml=ET.parse(NOTES/('tests'+suffix+'.xml')).getroot()
    results[suffix or 'normal']={key:int(xml.attrib[key]) for key in ['tests','failures','disabled','errors']}
    assert results[suffix or 'normal']['failures']==results[suffix or 'normal']['errors']==0
report={'original_compiler':'GC/3.0a5.2 with configured cflags_sdk, VERSION=0',
        'verified_dol_sha1':hashlib.sha1(dol).hexdigest(),'target_checked_instruction_words':verified_words,
        'objdiff_match_percent':matches,'target_object_sha256':sha(TARGET),'compiled_object_sha256':sha(BUILD/'OSMutex-original.o'),
        'original_command':json.loads((BUILD/'original-command.json').read_text()),
        'source_sha256':{p:sha(ROOT/p) for p in sources},'native_tests':results,
        'limits':'Native cooperative wait/identity mapping is not a PowerPC binary match or preemptive SDK thread implementation. Google Test libraries are not TSan-instrumented.'}
(NOTES/'evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'root_match':matches,'native_tests':results,'retail_words':verified_words},indent=2))
