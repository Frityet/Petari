#!/usr/bin/env python3
"""Use the configured real game flags and frozen native archives for this fixture."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-j3d-material-resource-20260903'
TARGET = 'smg-pc-original-j3d-joint-resource-tests'
CACHE = PC / 'build/.deps'
SOURCES = [
    'src/compat/J3DMaterialLoaderFactoryCompat.cpp',
    'src/compat/J3DMaterialVariantsCompat.cpp',
    'src/resource/J3dMaterialBlockData.cpp',
    'src/resource/J3dNameData.cpp',
    'tests/OriginalJ3DMaterialResourceTests.cpp',
]

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
                if source.startswith('src/') else TARGET + '/macosx/arm64/debug/tests/OriginalJ3DJointResourceTests.cpp.o.d')
    output = BUILD / (Path(source).stem + '.native.o')
    command = arguments(CACHE / template) + ['-c', source, '-o', str(output)]
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
binary = BUILD / 'material-resource-tests'
compat = PC / 'build/.objs' / TARGET / 'macosx/arm64/debug/aurora/lib/compat.cpp.o'
command = [args[0], *objects, str(compat), *args[1:], '-o', str(binary)]
commands.append(command)
(BUILD / 'native.commands.json').write_text(json.dumps(commands, indent=2) + '\n')
with (BUILD / 'native.link.log').open('w') as log:
    result = subprocess.run(command, cwd=PC, stdout=log, stderr=subprocess.STDOUT)
if result.returncode:
    print((BUILD / 'native.link.log').read_text())
    result.check_returncode()
assert archives == {str(p.relative_to(PC)): sha(p) for p in archive_paths}
env = dict(os.environ, SMGPC_REAL_DISC=str(ROOT / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'))
run = subprocess.run([str(binary)], cwd=PC, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
(HERE / 'native.log').write_text(run.stdout)
print(run.stdout, end='')
run.check_returncode()
assert '[pass] 4 original J3D material-resource groups' in run.stdout
assert '[resource] mario.bdl:' in run.stdout
report = {
    'scope': 'New material factory/variant/decoder fixture linked against unchanged frozen native archives; no shared build or GPU work.',
    'group_count': 4,
    'actual_mario_resource': True,
    'return_code': run.returncode,
    'binary_sha256': sha(binary),
    'source_sha256': {source: sha(PC / source) for source in SOURCES},
    'archive_sha256': archives,
}
(HERE / 'native-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
