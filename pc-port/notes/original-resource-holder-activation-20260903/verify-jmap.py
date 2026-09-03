#!/usr/bin/env python3
"""Build the real shared JMap/JKR lifetime fixture, with no shared xmake build."""
import importlib.util, json, os, subprocess
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-resource-holder-activation-20260903'
spec = importlib.util.spec_from_file_location('heap', HERE.parent / 'original-jkr-heap-20260903/verify-native.py')
heap = importlib.util.module_from_spec(spec); spec.loader.exec_module(heap)
BUILD.mkdir(parents=True, exist_ok=True)
for label, flags in [('normal', ['-O2']), ('asan', ['-O1','-fsanitize=address,undefined']), ('tsan', ['-O1','-fsanitize=thread'])]:
    msl = BUILD / ('jmap-msl-'+label+'.o')
    commands = [
        ['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-DAURORA','-g',*flags,
         '-Ipc-port/src','-Ipc-port/aurora/include','-c','pc-port/src/compat/MslPrintfCompat.cpp','-o',str(msl)],
        heap.COMMON + flags + heap.SOURCES + [str(msl), 'pc-port/src/resource/JMapResource.cpp',
         'pc-port/src/Game/Util/JMapInfo.cpp','pc-port/src/resource/BcsvTable.cpp',
         'pc-port/tests/OriginalJMapResourceTests.cpp','-Wl,-dead_strip','-pthread','-o',str(BUILD/('jmap-'+label))]
    ]
    (BUILD/('jmap-'+label+'-commands.json')).write_text(json.dumps(commands,indent=2)+'\n')
    with (HERE/('jmap-'+label+'-build.log')).open('w') as log:
        for command in commands:
            subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
    with (HERE/('jmap-'+label+'.log')).open('w') as log:
        subprocess.run([str(BUILD/('jmap-'+label))],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=30,
                       env={**os.environ,'ASAN_OPTIONS':'halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
    print((HERE/('jmap-'+label+'.log')).read_text(),end='')
