#!/usr/bin/env python3
"""Compare the real native PSVECKillElement to decoded retail instructions."""
import ctypes
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-binder-native-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
ORACLE = ROOT / 'pc-port/notes/xanime-core-matrix-calculation-20260903/math-oracle.py'
spec = importlib.util.spec_from_file_location('retail_math', ORACLE)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
mod.SPECS['kill'] = (0x803E7500, 0x34)
command = ['clang++', '-std=c++23', '-O2', '-fPIC', '-ffp-contract=off', '-Wno-register',
           '-DTARGET_PC', '-DMTX_USE_PS', '-Ipc-port/aurora/include', '-Ipc-port/src',
           '-include', 'pc-port/src/compat/MetrowerksStdCompat.hpp', '-dynamiclib',
           '-Wl,-dead_strip', '-Wl,-exported_symbol,_native_kill', 'pc-port/src/compat/GameMathCompat.cpp',
           str(HERE / 'math-harness.cpp'), '-o', str(BUILD / 'libmath.dylib')]
compiled = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
(BUILD / 'math-build.log').write_text(compiled.stdout)
compiled.check_returncode()
lib = ctypes.CDLL(str(BUILD / 'libmath.dylib'))
ptr = ctypes.POINTER(ctypes.c_float)
lib.native_kill.argtypes = [ptr, ptr, ptr]
lib.native_kill.restype = ctypes.c_float
rng = random.Random(0x803E7500)
cases = [([1,2,0], [2,0,0]), ([0,0,0],[0,0,0]), ([-0.0,0,-0.0],[0,-0.0,0])]
for _ in range(5000):
    cases.append(tuple([mod.f32(math.ldexp(rng.uniform(-1,1), rng.randrange(-10,11))) for _ in range(3)] for _ in range(2)))
dol = mod.DOL.read_bytes()
count = 0
for source, direction in cases:
    for alias in (None, 'p', 'q'):
        oracle = mod.Oracle(dol, [])
        for base in (mod.P, mod.Q, mod.OUT):
            oracle.memory[base + 12] = 0.0
        expected = oracle.execute('kill', p=source, q=direction, alias=alias)[:3]
        expected.append(oracle.f[1][0])
        a = (ctypes.c_float * 3)(*source)
        b = (ctypes.c_float * 3)(*direction)
        out = a if alias == 'p' else b if alias == 'q' else (ctypes.c_float * 3)()
        scalar = lib.native_kill(a, b, out)
        actual = [*out, scalar]
        assert list(map(mod.bits, actual)) == list(map(mod.bits, expected)), (source, direction, alias, actual, expected)
        count += 1
sources = ['src/Game/Util/MathUtil.cpp', 'pc-port/src/compat/GameMathCompat.cpp',
           'pc-port/notes/xanime-core-matrix-calculation-20260903/math-oracle.py']
evidence = {'dol_sha1': hashlib.sha1(dol).hexdigest(), 'address':'0x803E7500', 'size':52,
            'native_calls':count, 'finite_outputs_compared':count*4,
            'alias_modes':['separate', 'source', 'direction'], 'all_output_bits_equal':True,
            'scope':'5003 finite moderate-range input pairs; includes signed zero and nonunit directions. Does not assert NaN payload/sign, subnormal flushing, or FPSCR.',
            'compile_command':command,
            'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}}
(HERE/'math-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print(f'PASS {count} native calls / {count*4} output values against retail instructions')
