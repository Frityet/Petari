#!/usr/bin/env python3
"""Bounded retail magnitude witnesses for the unchanged OnlyCamera thresholds."""
import ctypes, hashlib, importlib.util, json, re, struct, subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-only-camera-20260903/math-thresholds'
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('reader', ROOT / 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
reader = importlib.util.module_from_spec(spec); spec.loader.exec_module(reader)
dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
# psq_l xy; load .5; ps_mul xy; load z; zero; ps_madd(z,z,xy);
# ps_sum0; zero branch; frsqrte; load3; square estimate; halve estimate;
# fnmsubs; multiply refined estimate; multiply square magnitude; return.
code = bytes.fromhex('e0030000c082250010000032c0230008ec4420281021007a10210014fc0110004d820020fc000834c0622504ec400032ec000132ec42187cec020032ec2100324e800020')
assert reader.dol_bytes(dol, 0x804B90D8, 0x44) == code
assert reader.dol_bytes(dol, 0x806C2120, 8) == bytes.fromhex('3f00000040400000')
floatutils = ROOT / 'pc-port/dolphin/Source/Core/Common/FloatUtils.cpp'
table = [(int(a,16), -int(b,16)) for a,b in re.findall(r'\{(0x[0-9a-f]+), -(0x[0-9a-f]+)\}', floatutils.read_text().split('frsqrte_expected',1)[1].split('}};',1)[0])]
assert len(table) == 32
f32 = lambda x: struct.unpack('>f', struct.pack('>f', x))[0]
bits = lambda x: struct.unpack('>I', struct.pack('>f', x))[0]
value = lambda x: struct.unpack('>f', struct.pack('>I', x))[0]
system = ctypes.CDLL(None); system.fmaf.argtypes = [ctypes.c_float]*3; system.fmaf.restype = ctypes.c_float

def estimate(x):
    raw = struct.unpack('>Q', struct.pack('>d', x))[0]
    exponent, mantissa = raw & (2047 << 52), raw & ((1 << 52)-1)
    assert x > 0 and exponent not in (0, 2047 << 52)
    index = ((exponent & (1 << 52)) | mantissa) >> 37
    base, dec = table[index // 2048]
    exp = ((1023 << 52) - (exponent - (1022 << 52)) // 2) & (2047 << 52)
    return struct.unpack('>d', struct.pack('>Q', exp | ((base + dec*(index % 2048)) << 26)))[0]

def axis_magnitude(x):
    squared = f32(x*x)
    e = estimate(squared)
    work = f32(e*e)
    half = f32(e*0.5)
    corrected = float(system.fmaf(-work, squared, 3.0))
    return f32(squared*f32(corrected*half))

command = ['/opt/homebrew/opt/llvm/bin/clang', '-std=c11', '-O2', '-dynamiclib', '-Ipc-port/aurora/include', 'pc-port/aurora/lib/dolphin/mtx/vec.c', '-o', str(BUILD/'libmagnitude.dylib')]
subprocess.run(command, cwd=ROOT, check=True)
lib = ctypes.CDLL(str(BUILD/'libmagnitude.dylib'))
lib.PSVECMag.argtypes = [ctypes.POINTER(ctypes.c_float)]; lib.PSVECMag.restype = ctypes.c_float
rows = []
for source, expected in ((0x3F800000,0x3F7FFFFF),(0x3F800001,0x3F800000),(0x3F800008,0x3F800007),(0x40000000,0x3FFFFFFF)):
    x = value(source)
    oracle = bits(axis_magnitude(x)); actual = bits(lib.PSVECMag((ctypes.c_float*3)(x,0,0)))
    assert oracle == actual == expected, (hex(source),hex(oracle),hex(actual),hex(expected))
    rows.append({'input_x_bits':hex(source),'retail_sequence_bits':hex(oracle),'actual_native_bits':hex(actual)})
report = {'scope':'Four finite axis magnitude witnesses; original DOL instruction/constant identity plus independent hardware-estimate table and rounded arithmetic. No universal numerical emulation claim.', 'dol_sha1':hashlib.sha1(dol).hexdigest(),'address':'0x804b90d8','size':68,'retail_bytes':code.hex(),'command':command,'witnesses':rows,'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [floatutils, ROOT/'pc-port/aurora/lib/dolphin/mtx/vec.c', ROOT/'pc-port/aurora/include/dolphin/ppc_math.h', ROOT/'pc-port/tests/OnlyCameraTests.cpp']}}
(HERE/'vector-threshold-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS four native PSVECMag threshold witnesses match the verified retail instruction sequence')
