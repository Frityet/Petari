#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,os,hashlib
N=Path(__file__).resolve().parent;R=N.parents[2];B=R/'build/original-collision-query-runtime-20260903';S=R/'build/original-collision-area-query-20260903/staged';OWNER=R/'build/original-collision-scene-owner-20260903/staged'
spec=importlib.util.spec_from_file_location('heap',N.parent/'original-jkr-heap-20260903/verify-native.py');heap=importlib.util.module_from_spec(spec);spec.loader.exec_module(heap)
source=(R/'src/Game/Util/MathUtil.cpp').read_text()
start=source.index('    void createBoundingBox(');end=source.index('\n    }',start)+6
bounding=source[start:end]
assert bounding in (R/'pc-port/src/Game/Util/MathUtil.cpp').read_text()
(B/'include/resource').mkdir(parents=True,exist_ok=True)
(B/'include/resource/KCollisionResource.hpp').write_text((OWNER/'resource/KCollisionResource.hpp').read_text())
(B/'OriginalBoundingBox.cpp').write_text('#include "Game/Util/MathUtil.hpp"\nnamespace MR {\n'+bounding+'\n}\n')
(B/'KCollisionQueryTests.cpp').write_text((N/'KCollisionQueryTests.cpp').read_text());rows=[]
for label,extra in [('normal',[]),('asan',['-fsanitize=address,undefined'])]:
 msl=B/('msl-'+label+'.o');subprocess.run(['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-DAURORA','-g','-O1',*extra,'-Ipc-port/src','-Ipc-port/aurora/include','-c','pc-port/src/compat/MslPrintfCompat.cpp','-o',str(msl)],cwd=R,check=True)
 binary=B/('query-tests-'+label)
 cmd=heap.COMMON[:1]+['-I'+str(B/'include'),'-I'+str(S),'-Ipc-port/src','-I'+str(OWNER)]+heap.COMMON[1:]+['-Iinclude','-Ilibs/JSystem/include','-O1','-ffp-contract=off',*extra]+heap.SOURCES+['pc-port/src/compat/JkrHeapFinalizer.cpp',str(msl),str(OWNER/'resource/KCollisionResource.cpp'),'pc-port/src/resource/JMapResource.cpp','pc-port/src/resource/BcsvTable.cpp','pc-port/src/Game/Util/JMapInfo.cpp',str(B/'OriginalBoundingBox.cpp'),str(S/'Game/Map/KCollision.cpp'),str(S/'Game/Map/KCollisionPlus.cpp'),str(B/'KCollisionQueryTests.cpp'),'-Wl,-dead_strip','-pthread','-o',str(binary)]
 (B/(label+'-command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 with (N/(label+'-build.log')).open('w') as out:subprocess.run(cmd,cwd=R,stdout=out,stderr=subprocess.STDOUT,check=True,timeout=60)
 p=subprocess.run([str(binary)],cwd=R,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30,env={**os.environ,'ASAN_OPTIONS':'halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 (N/(label+'.log')).write_text(p.stdout);p.check_returncode();print(p.stdout,end='');rows.append({'mode':label,'returncode':p.returncode,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'command':cmd})
sources=[N/'KCollisionQueryTests.cpp',S/'Game/Map/KCollision.cpp',S/'Game/Map/KCollisionPlus.cpp',OWNER/'resource/KCollisionResource.cpp',OWNER/'resource/KCollisionResource.hpp',B/'OriginalBoundingBox.cpp']
(N/'runtime-evidence.json').write_text(json.dumps({'point_cases':14,'area_cases':10,'shared_server_query_cases':1,'baseline_resource_groups':10,'source_sha256':{str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},'runs':rows},indent=2)+'\n')
