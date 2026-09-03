#!/usr/bin/env python3
"""Verify unchanged original table/holder bodies and actual static archive count."""
import ast, importlib.util, json, os, shlex, subprocess, types
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
BUILD=ROOT/'build/original-resource-holder-activation-20260903';BUILD.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('heap',HERE.parent/'original-jkr-heap-20260903/verify-native.py')
heap=importlib.util.module_from_spec(spec);spec.loader.exec_module(heap)
def run(command,log):
    with (HERE/log).open('w') as output:
        subprocess.run([str(x) for x in command],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True,timeout=60,
                       env={**os.environ,'ASAN_OPTIONS':'halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
for node in ast.parse((ROOT/'configure.py').read_text()).body:
    if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cflags_game' for t in node.targets):
        flags=eval(compile(ast.Expression(node.value),'configure.py','eval'),{'config':types.SimpleNamespace(version='RMGK01'),'version_num':0});break
base=['build/tools/wibo','build/tools/sjiswrap.exe','build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags:base.extend(shlex.split(flag))
records={}
for name in ('ResourceInfo','ResourceHolder'):
    target=ROOT/f'build/original-resource-holder-20260903/retail/obj/Game/System/{name}.o'
    if not target.exists():raise RuntimeError('Run prior verify-archive.py to prepare original dtk objects')
    command=base+['-c',f'src/Game/System/{name}.cpp','-o',str(BUILD/(name+'-ppc.o'))]
    run(command,name+'-original-build.log')
    run(['build/tools/objdiff-cli','diff','-1',target,'-2',BUILD/(name+'-ppc.o'),'-o',BUILD/(name+'-diff.json'),'--format','json-pretty'],name+'-objdiff.log')
    diff=json.loads((BUILD/(name+'-diff.json')).read_text())
    matches=[{'name':s['name'],'match_percent':s['match_percent']} for s in diff['left']['symbols'] if s.get('match_percent') is not None and any(tag in s['name'] for tag in (('__8ResTable','__11ResFileInfo') if name=='ResourceInfo' else ('__14ResourceHolder',)))]
    assert len(matches)==(15 if name=='ResourceInfo' else 13)
    assert all(s['match_percent']==100 for s in matches)
    records[name]={'command':command,'methods':matches}
(HERE/'resource-object-evidence.json').write_text(json.dumps(records,indent=2)+'\n')
# Link only the exact static methods under test. No ResourceHolder is constructed
# and no fake model/resource-holder provider supplies unrelated constructors.
source=(ROOT/'src/Game/System/ResourceHolder.cpp').read_text()
def function(marker):
    start=source.index(marker);brace=source.index('{',start);depth=0
    for end in range(brace,len(source)):
        if source[end]=='{':depth+=1
        if source[end]=='}':
            depth-=1
            if depth==0:return source[start:end+1]
    raise AssertionError(marker)
(BUILD/'ResourceHolderCount.cpp').write_text('#include "Game/System/ResourceHolder.hpp"\n#include <cstdio>\n#include <cstring>\n'+
    function('JKRFileFinder* ResourceHolder::getFileFinder(')+'\n'+function('s32 ResourceHolder::count(')+'\n')
(BUILD/'ResourceHolderCountTests.cpp').write_text((HERE/'ResourceHolderCountTests.cpp.in').read_text().replace('@ARCHIVE_TEST_SOURCE@',str(ROOT/'pc-port/tests/OriginalJkrArchiveTests.cpp')))
flags=['-O1','-fsanitize=address,undefined'];msl=BUILD/'count-msl.o'
run(['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-DAURORA',*flags,'-Ipc-port/src','-Ipc-port/aurora/include','-c','pc-port/src/compat/MslPrintfCompat.cpp','-o',msl],'count-msl-build.log')
command=heap.COMMON+flags+heap.SOURCES+[str(BUILD/'ResourceHolderCount.cpp'),str(BUILD/'ResourceHolderCountTests.cpp'),
    'pc-port/src/resource/RarcArchive.cpp','pc-port/src/resource/Yaz0.cpp','pc-port/src/compat/JKRArchiveCompat.cpp',
    'pc-port/src/compat/JKRFileFinderCompat.cpp',str(msl),'-Wl,-dead_strip','-pthread','-o',str(BUILD/'count-tests')]
run(command,'count-build.log')
arc=ROOT/'build/original-resource-holder-20260903/MarioAnime.arc'
args=[BUILD/'count-tests']+([arc] if arc.exists() else [])
run(args,'count-tests.log')
print((HERE/'count-tests.log').read_text(),end='')
print('PASS 15 ResourceInfo and 13 ResourceHolder original compiler comparisons')
