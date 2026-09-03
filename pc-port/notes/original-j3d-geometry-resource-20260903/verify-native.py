#!/usr/bin/env python3
"""Validate typed geometry and original factory/finalizers against frozen native archives."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-j3d-geometry-resource-20260903'
SOURCES = {
    'src/resource/J3dGeometryData.cpp': 'smg-pc-game',
    'src/resource/J3dNameData.cpp': 'smg-pc-game',
    'src/compat/J3DShapeFactoryCompat.cpp': 'smg-pc-game',
    'src/compat/J3DShapeTableCompat.cpp': 'smg-pc-game',
    'tests/OriginalJ3DGeometryResourceTests.cpp': 'smg-pc-original-j3d-joint-resource-tests',
}
CACHE = PC / 'build/.deps'
TARGET = 'smg-pc-original-j3d-joint-resource-tests'

def strings(text):
    return [json.loads(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', text)]

def arguments(path):
    source = path.read_text()
    return strings(source.split('values = {', 1)[1].split('files = {', 1)[0])

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

objects = []
commands = []
for source, target in SOURCES.items():
    cache_source = 'tests/OriginalJ3DJointResourceTests.cpp' if source.startswith('tests/') else 'src/resource/J3dJointData.cpp'
    args = arguments(CACHE / target / 'macosx/arm64/debug' / (cache_source + '.o.d'))
    output = BUILD / (Path(source).stem + '-final.o')
    command = args + ['-c', source, '-o', str(output)]
    commands.append(command)
    with (BUILD / (Path(source).stem + '-final.compile.log')).open('w') as log:
        subprocess.run(command, cwd=PC, stdout=log, stderr=subprocess.STDOUT, check=True)
    objects.append(str(output))
cache = CACHE / TARGET / 'macosx/arm64/debug' / (TARGET + '.d')
link_args = arguments(cache)
archive_paths = [PC / value for value in strings(cache.read_text().split('files = {', 1)[1]) if value.endswith('.a')]
archives = {str(p.relative_to(PC)): sha(p) for p in archive_paths}
binary = BUILD / 'geometry-resource-tests-final'
compat = PC / 'build/.objs' / TARGET / 'macosx/arm64/debug/aurora/lib/compat.cpp.o'
command = [link_args[0], *objects, str(compat), *link_args[1:], '-o', str(binary)]
commands.append(command)
(BUILD / 'geometry-native.commands.json').write_text(json.dumps(commands, indent=2) + '\n')
with (BUILD / 'geometry-native.link.log').open('w') as log:
    subprocess.run(command, cwd=PC, stdout=log, stderr=subprocess.STDOUT, check=True)
assert archives == {str(p.relative_to(PC)): sha(p) for p in archive_paths}, 'shared archive changed during isolated validation'
env = dict(os.environ, SMGPC_REAL_DISC=str(ROOT / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'))
run = subprocess.run([str(binary)], cwd=PC, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
(HERE / 'geometry-native.log').write_text(run.stdout)
run.check_returncode()
assert '[pass] 5 original J3D geometry-resource groups' in run.stdout
assert '[resource] mario.bdl:' in run.stdout
report = {
    'scope': 'Native geometry owner and actual original factory/finalizers linked against prior frozen archives; no shared build or GPU work.',
    'group_count': 5,
    'actual_mario_resource': True,
    'return_code': run.returncode,
    'binary_sha256': sha(binary),
    'source_sha256': {source: sha(PC / source) for source in SOURCES},
    'archive_sha256': archives,
}
(HERE / 'geometry-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
print(run.stdout, end='')
