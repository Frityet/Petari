#!/usr/bin/env python3
"""Original finder proof and isolated native typed archive lifecycle checks."""
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
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                     {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
        break
base = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags: base.extend(shlex.split(flag))
command = base + ['-c', 'src/JSystem/JKernel/JKRFileFinder.cpp', '-o', BUILD / 'JKRFileFinder.o']
run(command, 'finder-compile.log')
config = (ROOT / 'config/RMGK01/config.yml').read_text().replace('object_base: orig/RMGK01', 'object_base: ' + str(DOL.parent))
config = config.replace('object: sys/main.dol', 'object: ' + DOL.name).replace('symbols: config/', 'symbols: ' + str(ROOT / 'config') + '/')
config = config.replace('splits: config/', 'splits: ' + str(ROOT / 'config') + '/')
(BUILD / 'config.yml').write_text(config)
run(['build/tools/dtk', 'dol', 'split', '--no-update', '-j', '2', BUILD / 'config.yml', BUILD / 'retail'], 'dtk.log')
target_path = BUILD / 'retail/obj/JSystem/JKernel/JKRFileFinder.o'
run(['build/tools/objdiff-cli', 'diff', '-1', target_path, '-2', BUILD / 'JKRFileFinder.o', '-o', BUILD / 'finder.diff.json',
     '--format', 'json-pretty'], 'objdiff.log')
diff = json.loads((BUILD / 'finder.diff.json').read_text())
target, compiled = reader.Elf(target_path), reader.Elf(BUILD / 'JKRFileFinder.o')
methods = [
    ('__ct__12JKRArcFinderFP10JKRArchivell', 0x80410c44, 0x8c),
    ('findNextFile__12JKRArcFinderFv', 0x80410cd0, 0x9c),
    ('__ct__13JKRFileFinderFv', 0x80410d6c, 0x1c),
    ('__dt__12JKRArcFinderFv', 0x80410d88, 0x40),
    ('__dt__13JKRFileFinderFv', 0x80277a00, 0x40),
]
records = []
for name, addr, size in methods:
    comparison_target = compiled if name == '__dt__13JKRFileFinderFv' else target
    code, refs = proof.relocated(compiled, comparison_target, name, addr, size, raw)
    assert code == reader.dol_bytes(raw, addr, size), name
    percent = None if comparison_target is compiled else next(s for s in diff['left']['symbols'] if s['name'] == name)['match_percent']
    assert percent is None or percent == 100
    records.append({'name': name, 'address': hex(addr), 'size': size, 'retail_bytes_equal_after_relocation': True,
                    'objdiff_match_percent': percent, 'references': refs})
assert (ROOT / 'src/JSystem/JKernel/JKRFileFinder.cpp').read_bytes() == (ROOT / 'pc-port/src/compat/JKRFileFinderCompat.cpp').read_bytes()
source_files = ['pc-port/tests/OriginalJkrArchiveTests.cpp', 'pc-port/src/resource/RarcArchive.cpp',
                'pc-port/src/resource/Yaz0.cpp', 'pc-port/src/compat/JKRArchiveCompat.cpp', 'pc-port/src/compat/JKRFileFinderCompat.cpp']
tests = {}
for name, flags in [('normal', []), ('address-undefined', ['-fsanitize=address,undefined'])]:
    binary = BUILD / ('archive-tests-' + name)
    run(['clang++', '-std=c++20', '-O1', '-g', '-DTARGET_PC', '-Ipc-port/src', '-Ipc-port/aurora/include', *flags,
         *source_files, '-o', binary], name + '-compile.log')
    log = run([binary], name + '.log')
    cases = [line.removeprefix('PASS ') for line in log.splitlines() if line.startswith('PASS ')]
    assert len(cases) == 6 and 'runtime error:' not in log and 'ERROR: AddressSanitizer' not in log
    tests[name] = {'cases': cases, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()}
    print(name, '6/6 passed')
source_files += ['pc-port/src/JSystem/JKernel/JKRArchive.hpp', 'pc-port/src/resource/RarcArchive.hpp', 'src/JSystem/JKernel/JKRFileFinder.cpp']
(HERE / 'archive-evidence.json').write_text(json.dumps({'finder_retail_proof': records, 'native_tests': tests,
    'source_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in source_files},
    'scope': 'Typed archive catalog and actual original finder; ResourceHolder migration not activated.'}, indent=2) + '\n')
