#!/usr/bin/env python3
"""Verify source MSL string behavior and the native variadic boundary."""
import ast
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-msl-format-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
RETAIL = ROOT / 'build/original-resource-holder-20260903/retail/obj/MSL_C'
spec = importlib.util.spec_from_file_location('reader', HERE.parent / 'mario-update-restoration-20260903/verify-object.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)

def run(command, log):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            env=dict(os.environ, ASAN_OPTIONS='halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1'))
    (HERE / log).write_text(result.stdout)
    result.check_returncode()
    return result.stdout

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

FUNCTIONS = {
    'parse_format': (0x8051C1E8, 0x5FC), '__pformatter': (0x8051D96C, 0x85C),
    '__StringWrite': (0x8051E220, 0x6C), 'vsnprintf': (0x8051E3C8, 0x84),
    'vsprintf': (0x8051E44C, 0x80), 'snprintf': (0x8051E4CC, 0xD8),
    'sprintf': (0x8051E5A4, 0xD0), '__wctomb_noconv': (0x8051BB08, 0x1C),
    'wcstombs': (0x8051BBE4, 0xB8),
}
dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
for node in ast.parse((ROOT / 'configure.py').read_text()).body:
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_msl' for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                     {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
        break
records, commands = {}, {}
for unit, compiler in [('printf', '3.0a5.2'), ('mbstring', '3.0a3')]:
    command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', f'build/compilers/GC/{compiler}/mwcceppc.exe']
    for flag in flags: command.extend(shlex.split(flag))
    command += ['-c', f'src/MSL_C/{unit}.c', '-o', BUILD / (unit + '.o')]
    commands[unit] = [str(x) for x in command]
    run(command, unit + '-compile.log')
    diff = BUILD / (unit + '-objdiff.json')
    run(['build/tools/objdiff-cli', 'diff', '-1', RETAIL / (unit + '.o'), '-2', BUILD / (unit + '.o'),
         '-o', diff, '--format', 'json-pretty'], unit + '-objdiff.log')
    for item in json.loads(diff.read_text())['left']['symbols']:
        name = item['name']
        if name in FUNCTIONS and item.get('match_percent') is not None:
            assert item['match_percent'] == 100, name
            address, size = FUNCTIONS[name]
            records[name] = {'address': hex(address), 'size': size,
                             'objdiff_match_percent': item['match_percent']}
assert set(records) == set(FUNCTIONS)

# Verify the actual effective empty-string argument and fixed locale pointers,
# rather than merely finding matching data somewhere in the executable.
def word(address): return struct.unpack('>I', reader.dol_bytes(dol, address, 4))[0]
def half(address): return struct.unpack('>h', reader.dol_bytes(dol, address, 2))[0]
assert word(0x8051D980) == 0x3FA08056 and word(0x8051D994) == 0x3BBD2888
assert word(0x8051DEFC) == 0x2C1A0000 and word(0x8051DF00) == 0x40820008
assert word(0x8051DF04) == 0x3B5D002A
empty = ((word(0x8051D980) & 65535) << 16) + half(0x8051D996) + half(0x8051DF06)
assert empty == 0x805628B2 and reader.dol_bytes(dol, empty, 1) == b'\0'
locale = word(0x80609B80 + 0x38)
encoder = word(locale + 0x24)
assert locale == 0x80609A08 and encoder == 0x8051BB08
assert reader.dol_bytes(dol, encoder, 0x1c).hex() == '2c0300004082000c386000004e80002098830000386000014e800020'
for name, address, size in [('string-branch', 0x8051DEB0, 0x118), ('wide-encoder', 0x8051BB08, 0x1c)]:
    run(['python3', 'build/compat-math-oracle/disassemble_dol.py', hex(address), hex(size), BUILD / name], name + '.asm')

sources = ['pc-port/src/compat/MslPrintfCompat.cpp', 'pc-port/tests/MslPrintfTests.cpp', 'pc-port/tests/MslPrintfAliasTests.cpp']
base = ['/opt/homebrew/opt/llvm/bin/clang++', '-std=c++20', '-O2', '-g', '-Ipc-port/src',
        '-fno-builtin-sprintf', '-fno-builtin-snprintf', '-fno-builtin-vsprintf', '-fno-builtin-vsnprintf']
native = {}
for name, extra in [('normal', []), ('address-undefined', ['-fsanitize=address,undefined']), ('thread', ['-fsanitize=thread'])]:
    binary = BUILD / ('tests-' + name)
    run(base + extra + sources + ['-o', binary], name + '-compile.log')
    output = run([binary], name + '.log')
    cases = [line.removeprefix('PASS ') for line in output.splitlines() if line.startswith('PASS ')]
    assert len(cases) == 9 and 'runtime error:' not in output and 'WARNING: ThreadSanitizer' not in output
    native[name] = {'cases': cases, 'binary_sha256': sha(binary)}
    print(name, '9/9 passed')
alias_object = BUILD / 'aliases.o'
run(base + ['-c', sources[2], '-o', alias_object], 'alias-compile.log')
undefined = run(['nm', '-u', alias_object], 'alias-symbols.log')
for name in ('sprintf', 'snprintf', 'vsprintf', 'vsnprintf'):
    assert '_smgpc_msl_' + name in undefined
    assert not re.search(r'\s_' + name + r'\s*$', undefined, re.M)
forced = (ROOT / 'pc-port/src/compat/MetrowerksStdCompat.hpp').read_text()
assert forced.index('#include "compat/MetrowerksPrintf.hpp"') < forced.index('#include "Game/')
sources += ['pc-port/src/compat/MetrowerksPrintf.hpp', 'pc-port/src/compat/MslPrintfCompat.hpp',
            'src/MSL_C/printf.c', 'src/MSL_C/mbstring.c', 'src/MSL_C/locale.c']
(HERE / 'evidence.json').write_text(json.dumps({'original_compiler_commands': commands,
    'original_functions': records, 'retail_empty_string': hex(empty), 'retail_locale': hex(locale),
    'retail_wide_encoder': hex(encoder), 'native_tests': native,
    'source_sha256': {p: sha(ROOT / p) for p in sources},
    'scope': 'MSL string conversions and string-output sinks; native numeric conversions and host-only grammar remain libc behavior.'}, indent=2) + '\n')
