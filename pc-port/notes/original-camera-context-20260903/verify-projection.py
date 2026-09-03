#!/usr/bin/env python3
"""Bounded scalar PPC oracle for the complete original projection update.

Both instruction streams use the same host tan call. This verifies projection
arithmetic and field writes, not a bit-exact Wii libm implementation.
"""
from pathlib import Path
import hashlib
import importlib.util
import itertools
import json
import math
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-camera-context-20260903'
NAME = 'updateProjectionMtx__13CameraContextFv'
ADDRESS = 0x80097708


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
proof = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')


def f32(value):
    return struct.unpack('>f', struct.pack('>f', value))[0]


def bits(value):
    return struct.pack('>f', value).hex()


class Program:
    def __init__(self, path):
        self.elf = reader.Elf(path)
        _, start, size, section = next(s for s in self.elf.symbols if s[0] == NAME)
        self.code = self.elf.section_data(section)[start:start + size]
        self.refs = {int(r['offset'], 16): r for r in self.elf.references(NAME)}

    def run(self, fovy, aspect, near, far, shake_x, shake_y):
        gpr = {1: 0x2000, 3: 0x1000}
        fpr = {}
        memory = {0x1000 + offset: f32(value) for offset, value in
                  ((0xB0, near), (0xB4, far), (0xB8, fovy), (0xBC, shake_x), (0xC0, shake_y))}
        writes = set()
        restoring = False
        for offset in range(0, len(self.code), 4):
            word = struct.unpack_from('>I', self.code, offset)[0]
            op = word >> 26
            rt, ra, rb, rc = (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31, (word >> 6) & 31
            imm = (word & 0xFFFF) - (0x10000 if word & 0x8000 else 0)
            if word == 0x4E800020:
                assert offset + 4 == len(self.code)
                break
            if op == 37:  # stwu: stack frame allocation
                assert rt == ra == 1
                gpr[1] += imm
            elif op == 14:  # addi: stack frame retirement
                assert restoring and rt == ra == 1
                gpr[1] += imm
            elif op == 31:
                xo = (word >> 1) & 1023
                if xo == 444:  # or (mr)
                    assert rt == rb
                    gpr[ra] = gpr[rt]
                else:
                    assert xo in (339, 467) and rt == 0  # mflr/mtlr
            elif op in (32, 36, 50, 54, 56, 60):
                # Only callee-save spill/restore instructions. They cannot
                # read projection temporaries or write the CameraContext.
                assert ra == 1 and imm >= 0x4C, (offset, hex(word))
                if op in (32, 50, 56):
                    restoring = True
            elif op == 18:  # bl, each resolved relocation required
                assert not restoring and word & 1
                ref = self.refs[offset]
                assert ref['kind'] == 10 and ref['addend'] == 0
                if ref['symbol'] == 'getAspect__13CameraContextCFv':
                    fpr[1] = f32(aspect)
                elif ref['symbol'] == 'tan':
                    fpr[1] = math.tan(fpr[1])
                else:
                    raise AssertionError(ref)
            elif op == 48:  # lfs
                assert not restoring
                if offset in self.refs:
                    ref = self.refs[offset]
                    assert ref['kind'] == 109 and ref['addend'] == 0
                    value = bytes.fromhex(ref['value_hex'])
                    assert len(value) == 4
                    fpr[rt] = struct.unpack('>f', value)[0]
                else:
                    fpr[rt] = memory[gpr[ra] + imm]
            elif op == 52:  # stfs
                assert not restoring
                address = gpr[ra] + imm
                assert (0x106C <= address <= 0x10A8) or (gpr[1] + 8 <= address <= gpr[1] + 0x44)
                memory[address] = f32(fpr[rt])
                writes.add(address)
            elif op == 59:
                assert not restoring
                xo = (word >> 1) & 31
                if xo == 18:
                    value = fpr[ra] / fpr[rb]
                elif xo == 20:
                    value = fpr[ra] - fpr[rb]
                elif xo == 21:
                    value = fpr[ra] + fpr[rb]
                elif xo == 25:
                    value = fpr[ra] * fpr[rc]
                else:
                    raise AssertionError((offset, hex(word), xo))
                fpr[rt] = f32(value)
            elif op == 63:
                assert not restoring
                xo = (word >> 1) & 1023
                if xo == 12:
                    fpr[rt] = f32(fpr[rb])
                elif xo == 40:
                    fpr[rt] = -fpr[rb]
                elif xo == 72:
                    fpr[rt] = fpr[rb]
                else:
                    raise AssertionError((offset, hex(word), xo))
            else:
                raise AssertionError((offset, hex(word), op))
        assert gpr[1] == 0x2000
        assert set(range(0x106C, 0x10AC, 4)).issubset(writes)
        return [bits(memory[address]) for address in range(0x106C, 0x10AC, 4)]


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    output = BUILD / 'CameraContext.o'
    command = compiler.compiler('cflags_game') + ['-c', str(ROOT / 'src/Game/Camera/CameraContext.cpp'), '-o', str(output)]
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / 'compile.log').write_text(result.stdout)
    result.check_returncode()
    retail_path = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Camera/CameraContext.o'
    retail, compiled = Program(retail_path), Program(output)
    dol = compiler.DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    relocated, relocations = proof.relocated(retail.elf, retail.elf, NAME, ADDRESS, len(retail.code), dol)
    assert relocated == reader.dol_bytes(dol, ADDRESS, len(retail.code))
    fovys = (1, 15, 30, 45, 60, 75, 90, 100, 120, 150, 175)
    aspects = (4 / 3, 16 / 9, 640 / 456, 1)
    clips = ((0.1, 100), (1, 1000), (100, 800000), (500, 900000), (10000, 10001))
    shakes = ((0, 0), (-0.0, -0.0), (-1, 1), (1, -1), (0.125, -0.375), (1e-5, -1e-5), (-2, 2))
    rows, differences = [], []
    for fovy, aspect, (near, far), (x, y) in itertools.product(fovys, aspects, clips, shakes):
        inputs = (fovy, aspect, near, far, x, y)
        a, b = retail.run(*inputs), compiled.run(*inputs)
        if a != b:
            differences.append({'input': inputs, 'retail': a, 'compiled': b})
        rows.append({'input': inputs, 'projection': a})
    evidence = {
        'scope': 'Bounded instruction-stream comparison; common host tan, no Wii libm or complete camera runtime claim.',
        'source_sha256': hashlib.sha256((ROOT / 'src/Game/Camera/CameraContext.cpp').read_bytes()).hexdigest(),
        'retail_object_sha256': hashlib.sha256(retail_path.read_bytes()).hexdigest(),
        'compiled_object_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
        'retail_address': hex(ADDRESS), 'retail_bytes': len(retail.code), 'compiled_bytes': len(compiled.code),
        'verified_relocations': relocations, 'cases': len(rows), 'matrix_components': 16 * len(rows),
        'differences': differences, 'retail_results_sha256': hashlib.sha256(json.dumps(rows).encode()).hexdigest(),
        'compiler_command': command,
    }
    (NOTES / 'projection-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    (BUILD / 'projection-cases.json').write_text(json.dumps(rows, indent=2) + '\n')
    print(f'{len(rows)} cases, {len(rows) * 16} components, {len(differences)} differing matrices')
    assert not differences

    native_command = ['/opt/homebrew/opt/llvm/bin/clang++', '-std=c++23', '-ffp-contract=off',
                      '-DAURORA', '-DTARGET_PC', '-Ipc-port/src', '-Ipc-port/aurora/include',
                      str(NOTES / 'ProjectionProbe.cpp'), '-o', str(BUILD / 'projection-probe')]
    subprocess.run(native_command, cwd=ROOT, check=True)
    result = subprocess.run([str(BUILD / 'projection-probe')], check=True, text=True, capture_output=True,
                            input='\n'.join(' '.join(map(str, row['input'])) for row in rows) + '\n')
    native_rows = result.stdout.splitlines()
    assert len(native_rows) == len(rows)
    native_differences = []
    for row, actual in zip(rows, native_rows):
        values = [f'{int(value, 16):08x}' for value in actual.split()]
        if values != row['projection']:
            native_differences.append({'input': row['input'], 'retail': row['projection'], 'native': values})
    native_evidence = {'cases': len(rows), 'components': len(rows) * 16, 'differences': native_differences,
                       'command': native_command, 'matrix_header_sha256': hashlib.sha256(
                           (ROOT / 'pc-port/src/JSystem/JGeometry/TMatrix.hpp').read_bytes()).hexdigest()}
    (NOTES / 'native-projection-evidence.json').write_text(json.dumps(native_evidence, indent=2) + '\n')
    print(f'{len(rows)} native cases, {len(native_differences)} differing matrices')
    assert not native_differences


if __name__ == '__main__':
    main()
