#!/usr/bin/env python3
"""Native heap/list checks using real providers and frozen shared archives."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-jkr-base-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
TARGET = 'smg-pc-original-j3d-joint-resource-tests'
CACHE = PC / 'build/.deps'
SOURCES = ['src/compat/JKRHeapCompat.cpp', 'src/compat/JKRSolidHeapCompat.cpp',
           'src/compat/JKRDisposerCompat.cpp', 'src/compat/JSUListCompat.cpp',
           'src/compat/JkrDiagnostics.cpp', 'src/compat/JkrAllocationProvenance.cpp',
           'aurora/lib/dolphin/os/OSExecution.cpp', 'aurora/lib/dolphin/os/OSMutex.cpp',
           'aurora/lib/dolphin/os/OSReport.cpp', 'tests/OriginalJkrHeapTests.cpp']

def strings(text):
    return [json.loads(x) for x in re.findall(r'"(?:\\.|[^"\\])*"', text)]

def args(path):
    return strings(re.search(r'values = \{(.*?)\n    \}', path.read_text(), re.S)[1])

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def run(command, log):
    result = subprocess.run(command, cwd=PC, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            env=dict(os.environ, ASAN_OPTIONS='halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1'))
    (BUILD / log).write_text(result.stdout)
    if result.returncode: print(result.stdout[-4000:])
    result.check_returncode()
    return result.stdout

base = args(CACHE / 'smg-pc-game/macosx/arm64/debug/src/resource/J3dJointData.cpp.o.d')
link_cache = CACHE / TARGET / 'macosx/arm64/debug' / (TARGET + '.d')
link = args(link_cache)
archives = [PC / x for x in strings(re.search(r'files = \{(.*?)\n    \}', link_cache.read_text(), re.S)[1]) if x.endswith('.a')]
archive_hashes = {str(p.relative_to(PC)): sha(p) for p in archives}
source_hashes = {p: sha(PC / p) for p in SOURCES}
compat = PC / 'build/.objs' / TARGET / 'macosx/arm64/debug/aurora/lib/compat.cpp.o'
records = {}
for label, flags in [('normal', []), ('address-undefined', ['-fsanitize=address,undefined', '-fno-sanitize-recover=all'])]:
    objects, commands = [], []
    for source in SOURCES:
        out = BUILD / (Path(source).stem + '-' + label + '.o')
        command = base + flags + ['-Wno-register', '-fno-color-diagnostics', '-c', source, '-o', str(out)]
        commands.append(command)
        run(command, Path(source).stem + '-' + label + '.compile.log')
        objects.append(str(out))
    binary = BUILD / ('heap-tests-' + label)
    command = [link[0], *objects, str(compat), *link[1:], *flags, '-o', str(binary)]
    commands.append(command)
    run(command, label + '.link.log')
    output = run([str(binary)], label + '.log')
    assert '[pass] 5 original JKR heap/list groups' in output
    assert 'runtime error:' not in output and 'ERROR: AddressSanitizer' not in output
    records[label] = {'groups': 5, 'binary_sha256': sha(binary), 'commands': commands}
    print(label, '5/5 passed')
assert archive_hashes == {str(p.relative_to(PC)): sha(p) for p in archives}
assert source_hashes == {p: sha(PC / p) for p in SOURCES}
(HERE / 'native-evidence.json').write_text(json.dumps({
    'scope': 'Actual native Base/Solid/Disposer/JSU list, OS execution/mutex/report and allocation provenance; complete runtime/global-new routing is validated separately.',
    'native_tests': records, 'source_sha256': source_hashes, 'shared_archive_sha256': archive_hashes}, indent=2) + '\n')
