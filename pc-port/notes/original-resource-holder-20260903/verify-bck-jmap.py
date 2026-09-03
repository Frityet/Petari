#!/usr/bin/env python3
"""Original BckCtrl comparison and shared JMap borrowed-string lifetime checks."""
import ast
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-resource-holder-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('proof', HERE.parent / 'original-binder-reaction-20260903/verify-runtime.py')
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)
reader = proof.reaction.reader

def run(cmd, log):
    result = subprocess.run([str(x) for x in cmd], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            env=dict(os.environ, ASAN_OPTIONS='halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1'))
    (HERE / log).write_text(result.stdout)
    result.check_returncode()
    return result.stdout

raw = DOL.read_bytes()
assert hashlib.sha1(raw).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
for node in ast.parse((ROOT / 'configure.py').read_text()).body:
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_game' for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                     {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
        break
base = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags: base.extend(shlex.split(flag))
command = base + ['-c', 'src/Game/Animation/BckCtrl.cpp', '-o', BUILD / 'BckCtrl.o']
run(command, 'bck-compile.log')
(BUILD / 'bck.command.json').write_text(json.dumps([str(x) for x in command], indent=2) + '\n')
target_path = BUILD / 'retail/obj/Game/Animation/BckCtrl.o'
if not target_path.exists():
    raise RuntimeError('Run verify-archive.py first to prepare the original dtk objects')
run(['build/tools/objdiff-cli', 'diff', '-1', target_path, '-2', BUILD / 'BckCtrl.o', '-o', BUILD / 'bck.diff.json',
     '--format', 'json-pretty'], 'bck-objdiff.log')
diff = json.loads((BUILD / 'bck.diff.json').read_text())
records = [{'name': s['name'], 'retail_size': s['size'], 'objdiff_match_percent': s['match_percent']}
           for s in diff['left']['symbols'] if 'BckCtrl' in s['name'] and s.get('match_percent') is not None and int(s['size']) > 4]
for record in records:
    assert record['objdiff_match_percent'] >= 92
    print(record['name'], record['objdiff_match_percent'])
target, compiled = reader.Elf(target_path), reader.Elf(BUILD / 'BckCtrl.o')
name = '__as__11BckCtrlDataFRC11BckCtrlData'
code, refs = proof.relocated(compiled, target, name, 0x80017c70, 0x64, raw)
assert code == reader.dol_bytes(raw, 0x80017c70, 0x64)
for a, b in [('src/Game/Animation/BckCtrl.cpp', 'pc-port/src/Game/Animation/BckCtrl.cpp'),
             ('include/Game/Animation/BckCtrl.hpp', 'pc-port/src/Game/Animation/BckCtrl.hpp')]:
    assert (ROOT / a).read_bytes() == (ROOT / b).read_bytes(), (a, b)
source_files = ['pc-port/tests/OriginalJMapResourceTests.cpp', 'pc-port/src/resource/JMapResource.cpp',
                'pc-port/src/resource/BcsvTable.cpp', 'pc-port/src/Game/Util/JMapInfo.cpp']
tests = {}
for name, flags in [('normal', []), ('address-undefined', ['-fsanitize=address,undefined']), ('thread', ['-fsanitize=thread'])]:
    binary = BUILD / ('jmap-tests-' + name)
    run(['clang++', '-std=c++20', '-O1', '-g', '-DTARGET_PC', '-Ipc-port/src', '-Ipc-port/aurora/include', *flags,
         *source_files, '-o', binary], 'jmap-' + name + '-compile.log')
    log = run([binary], 'jmap-' + name + '.log')
    cases = [line.removeprefix('PASS ') for line in log.splitlines() if line.startswith('PASS ')]
    assert len(cases) == 4 and 'runtime error:' not in log and 'WARNING: ThreadSanitizer' not in log
    tests[name] = {'cases': cases, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()}
    print(name, '4/4 passed')
source_files += ['pc-port/src/Game/Util/JMapInfo.hpp', 'include/Game/Animation/BckCtrl.hpp',
                 'src/Game/Animation/BckCtrl.cpp', 'pc-port/src/Game/Animation/BckCtrl.hpp',
                 'pc-port/src/Game/Animation/BckCtrl.cpp', 'pc-port/tests/OriginalBckCtrlTests.cpp']
(HERE / 'bck-jmap-evidence.json').write_text(json.dumps({'bck_comparison': records,
    'assignment_retail_bytes_equal_after_relocation': True,
    'manual_semantic_review': 'See bck-review.md for instruction-level residuals; nonmatching routines are functional recoveries, not byte matches.',
    'native_jmap_tests': tests, 'native_bck_runtime_tests': 'Awaiting coordinated parent build; syntax-only probe passed.',
    'source_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in source_files},
    'scope': 'Original BckCtrl prerequisite and shared borrowed-string ownership; ResourceHolder migration not activated.'}, indent=2) + '\n')
