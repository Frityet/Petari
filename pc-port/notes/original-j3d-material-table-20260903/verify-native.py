#!/usr/bin/env python3
"""Use the configured real game flags and frozen native archives for this fixture."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-j3d-material-table-20260903'
TARGET = 'smg-pc-original-j3d-material-resource-tests'
CACHE = PC / 'build/.deps'
SOURCES = [
    'src/resource/J3dMaterialTableData.cpp',
    'src/resource/J3dAllocationIdentity.cpp',
    'tests/OriginalJ3DMaterialTableTests.cpp',
]
parser=argparse.ArgumentParser()
parser.add_argument('--sanitize',action='store_true')
options=parser.parse_args()
SUFFIX='-asan' if options.sanitize else ''
FLAGS=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if options.sanitize else []
if options.sanitize:
    SOURCES += [
        'src/resource/J3dMaterialBlockData.cpp',
        'src/resource/J3dNameData.cpp',
        'src/compat/J3DMaterialLoaderFactoryCompat.cpp',
        'src/compat/J3DMaterialVariantsCompat.cpp',
    ]
    SOURCES += ['src/compat/' + name + '.cpp' for name in (
        'JKRExpHeapCompat','MemoryHeapScopeCompat','JkrAllocationDomain','JkrAllocationProvenance',
        'MetrowerksAlignedNew','JKRHeapCompat','JKRSolidHeapCompat','JKRDisposerCompat','JSUListCompat','JkrDiagnostics')]
    SOURCES += ['aurora/lib/dolphin/os/' + name + '.cpp' for name in ('OSExecution','OSMutex','OSReport')]

def strings(text):
    return [json.loads(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', text)]

def arguments(path):
    return strings(re.search(r'values = \{(.*?)\n    \}', path.read_text(), re.S).group(1))

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

BUILD.mkdir(parents=True, exist_ok=True)
commands, objects = [], []
for source in SOURCES:
    template = ('smg-pc-game/macosx/arm64/debug/src/compat/J3DJointCompat.cpp.o.d'
                if not source.startswith('tests/') else TARGET + '/macosx/arm64/debug/tests/OriginalJ3DMaterialResourceTests.cpp.o.d')
    output = BUILD / (Path(source).stem + '.native'+SUFFIX+'.o')
    command = arguments(CACHE / template) + FLAGS + ['-Iaurora/lib/dolphin','-c', source, '-o', str(output)]
    commands.append(command)
    with (BUILD / (Path(source).stem + '.compile.log')).open('w') as log:
        result = subprocess.run(command, cwd=PC, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((BUILD / (Path(source).stem + '.compile.log')).read_text())
        result.check_returncode()
    objects.append(str(output))
cache = CACHE / TARGET / 'macosx/arm64/debug' / (TARGET + '.d')
args = arguments(cache)
archive_paths = [PC / value for value in strings(cache.read_text().split('files = {', 1)[1]) if value.endswith('.a')]
archives = {str(p.relative_to(PC)): sha(p) for p in archive_paths}
binary = BUILD / ('material-table-tests'+SUFFIX)
compat = PC / 'build/.objs' / TARGET / 'macosx/arm64/debug/aurora/lib/compat.cpp.o'
command = [args[0], *FLAGS, *objects, str(compat), *args[1:], '-o', str(binary)]
commands.append(command)
(BUILD / ('native'+SUFFIX+'.commands.json')).write_text(json.dumps(commands, indent=2) + '\n')
with (BUILD / ('native'+SUFFIX+'.link.log')).open('w') as log:
    result = subprocess.run(command, cwd=PC, stdout=log, stderr=subprocess.STDOUT)
if result.returncode:
    print((BUILD / ('native'+SUFFIX+'.link.log')).read_text())
    result.check_returncode()
assert archives == {str(p.relative_to(PC)): sha(p) for p in archive_paths}
env = dict(os.environ,ASAN_OPTIONS='detect_leaks=1',UBSAN_OPTIONS='halt_on_error=1')
run = subprocess.run([str(binary)], cwd=PC, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
(HERE / ('native'+SUFFIX+'.log')).write_text(run.stdout)
print(run.stdout, end='')
run.check_returncode()
assert '[pass] 6 original J3D material-table groups' in run.stdout
report = {
    'scope': 'New material-table component and identity helper linked against unchanged frozen native archives; no shared build or GPU work.',
    'group_count': 6,
    'actual_mario_resource': False,
    'instrumentation': 'Selected component, decoders, factories, heap/lifecycle providers and tests use ASan/UBSan; other SDK/Aurora archives are uninstrumented.' if options.sanitize else 'Configured native debug build flags.',
    'return_code': run.returncode,
    'binary_sha256': sha(binary),
    'source_sha256': {source: sha(PC / source) for source in SOURCES},
    'archive_sha256': archives,
}
(HERE / ('native'+SUFFIX+'-evidence.json')).write_text(json.dumps(report, indent=2) + '\n')
