#!/usr/bin/env python3
"""Isolated native heap/scope tests; no shared xmake build or GPU runtime."""
import argparse,json,subprocess,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
NOTES=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-jkr-heap-20260903'
SOURCES=[f'pc-port/src/compat/{name}.cpp' for name in (
 'JKRExpHeapCompat','MemoryHeapScopeCompat','JkrAllocationDomain','JkrAllocationProvenance',
 'MetrowerksAlignedNew','JKRHeapCompat','JKRSolidHeapCompat','JKRDisposerCompat','JSUListCompat','JkrDiagnostics')]
SOURCES += [f'pc-port/aurora/lib/dolphin/os/{name}.cpp' for name in ('OSExecution','OSMutex','OSReport')]
COMMON=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-DAURORA','-g',
 '-Wno-multichar','-Wno-inconsistent-missing-override','-Wno-macro-redefined',
 '-fno-builtin-sprintf','-fno-builtin-snprintf','-fno-builtin-vsprintf','-fno-builtin-vsnprintf',
 '-include','pc-port/src/compat/MetrowerksStdCompat.hpp','-Ipc-port/src','-Ipc-port/aurora/include',
 '-Ipc-port/aurora/lib/dolphin']
def main():
 p=argparse.ArgumentParser();p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
 variants=[('',[])]+([('-asan',['-fsanitize=address,undefined']),('-tsan',['-fsanitize=thread'])] if args.sanitizers else [])
 for suffix,flags in variants:
  binary=BUILD/('domain-tests'+suffix)
  msl=BUILD/('MslPrintfCompat'+suffix+'.o')
  msl_command=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-DAURORA','-g',*flags,
   '-Ipc-port/src','-Ipc-port/aurora/include','-c','pc-port/src/compat/MslPrintfCompat.cpp','-o',str(msl)]
  subprocess.run(msl_command,cwd=ROOT,check=True,timeout=60)
  cmd=COMMON+flags+SOURCES+[str(msl)]+['pc-port/tests/JkrAllocationDomainTests.cpp','-pthread','-o',str(binary)]
  (BUILD/('native-command'+suffix+'.json')).write_text(json.dumps(cmd,indent=2)+'\n')
  with (NOTES/('native-build'+suffix+'.log')).open('w') as log:
   subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
  with (NOTES/('native-tests'+suffix+'.log')).open('w') as log:
   subprocess.run([str(binary)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45,
    env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'detect_leaks=1'})
  print((NOTES/('native-tests'+suffix+'.log')).read_text(),end='')
if __name__=='__main__': main()
