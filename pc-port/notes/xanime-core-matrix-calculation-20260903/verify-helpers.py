#!/usr/bin/env python3
"""Source/retail graph checks for Xanime's shared vector/matrix helpers.

Compiles only exact original-source function extracts with GC 3.0a3. No native
build, native runtime, or Dolphin execution is claimed by this verifier.
"""
import ast
import ctypes
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import sys
import types

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / 'build/xanime-core-math-helpers-20260903'
spec = importlib.util.spec_from_file_location('model_evidence', Path(__file__).with_name('verify-model-foundation.py'))
model = importlib.util.module_from_spec(spec)
spec.loader.exec_module(model)
VEC = 'Q29JGeometry8TVec3<f>'
FUNCTIONS = {
    'psblend': ('MathUtil', 'void PSvecBlend(', f'PSvecBlend__2MRFPC{VEC}PC{VEC}P{VEC}ff', 0x803E75C8, 0x2C),
    'blend': ('MathUtil', 'void vecBlend(', f'vecBlend__2MRFRC{VEC}RC{VEC}P{VEC}f', 0x803E75F4, 0x10),
    'set-scalar': ('MtxUtil', 'void setMtxTrans(MtxPtr mtx, f32', 'setMtxTrans__2MRFPA4_ffff', 0x803ECFB0, 0x10),
    'set-vector': ('MtxUtil', 'void setMtxTrans(MtxPtr mtx, const TVec3f&', f'setMtxTrans__2MRFPA4_fRC{VEC}', 0x801B5BF0, 0x10),
}


def extract(text, marker):
    start = text.index(marker)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def normalized(text):
    return re.sub(r'\s+', '', text)


def original_bodies():
    result = {}
    for name, (unit, marker, symbol, address, length) in FUNCTIONS.items():
        source = (ROOT / f'src/Game/Util/{unit}.cpp').read_text()
        body = extract(source, marker)
        native_file = 'GameMathCompat.cpp' if unit == 'MathUtil' else 'MatrixTranslationCompat.cpp'
        native = extract((ROOT / 'pc-port/src/compat' / native_file).read_text(), marker)
        assert normalized(body) == normalized(native), name
        result[name] = body
    return result


def retail_ps_graph():
    code = model.read_dol(0x803E75C8, 0x2C)
    registers = {1: ('weightFrom', 'weightFrom'), 2: ('weightTo', 'weightTo')}
    outputs = {}
    loaded = set()
    stores_started = False
    operations = []
    for word, in struct.iter_unpack('>I', code):
        op = word >> 26
        d, a, b, c = [(word >> n) & 31 for n in (21, 16, 11, 6)]
        if op == 56:
            assert not stores_started, 'Retail source read after output write'
            assert a in (3, 4)
            offset = word & 0xFFF
            assert offset in (0, 8)
            one = (word >> 15) & 1
            name = 'from' if a == 3 else 'to'
            indices = [offset // 4] if one else [offset // 4, offset // 4 + 1]
            values = [(name, 'xyz'[index]) for index in indices]
            loaded.update(values)
            registers[d] = tuple(values) if not one else (values[0], 1.0)
            operations.append('load_z' if one else 'load_xy')
        elif op == 4:
            xo = (word >> 1) & 31
            assert xo in (12, 14), hex(word)
            if xo == 12:
                registers[d] = tuple(('mul32', x, registers[c][0]) for x in registers[a])
                operations.append('multiply_scalar_lane0')
            else:
                registers[d] = tuple(('fma32', x, registers[c][0], y) for x, y in zip(registers[a], registers[b]))
                operations.append('fused_multiply_add_scalar_lane0')
        elif op == 60:
            assert a == 5
            assert loaded == {(which, axis) for which in ('from', 'to') for axis in 'xyz'}
            stores_started = True
            offset = word & 0xFFF
            one = (word >> 15) & 1
            for index in range(1 if one else 2):
                outputs['xyz'[offset // 4 + index]] = registers[d][index]
            operations.append('store_z' if one else 'store_xy')
        else:
            assert word == 0x4E800020, hex(word)
            operations.append('return')
    return outputs, operations


def native_ps_graph(body):
    native = body.split('#else', 1)[1].split('#endif', 1)[0]
    values = {'a4': 'weightFrom', 'a5': 'weightTo'}
    outputs = {}
    loaded = set()
    stores_started = False
    for line in native.splitlines():
        line = line.strip()
        if not line:
            continue
        match = re.fullmatch(r'const f32 (\w+) = a([12])->([xyz]);', line)
        if match:
            assert not stores_started
            name, pointer, axis = match.groups()
            values[name] = ('from' if pointer == '1' else 'to', axis)
            loaded.add(values[name])
            continue
        match = re.fullmatch(r'const f32 (\w+) = (\w+) \* (\w+);', line)
        if match:
            name, left, right = match.groups()
            values[name] = ('mul32', values[left], values[right])
            continue
        match = re.fullmatch(r'a3->([xyz]) = std::fma\((\w+), (\w+), (\w+)\);', line)
        assert match, line
        assert loaded == {(which, axis) for which in ('from', 'to') for axis in 'xyz'}
        stores_started = True
        axis, a, b, c = match.groups()
        outputs[axis] = ('fma32', values[a], values[b], values[c])
    return outputs


def compiler_command():
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_game' for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            break
    result = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
    for flag in flags:
        result.extend(shlex.split(flag))
    return result


def compile_and_compare(bodies):
    results = {}
    for name, (unit, marker, symbol, address, length) in FUNCTIONS.items():
        source = BUILD / (name + '.cpp')
        source.write_text(f'#include "Game/Util/{unit}.hpp"\nnamespace MR {{\n' + bodies[name] + '\n}\n')
        output = BUILD / (name + '.o')
        command = compiler_command() + ['-c', str(source), '-o', str(output)]
        (BUILD / (name + '-compile-command.json')).write_text(json.dumps(command, indent=2) + '\n')
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / (name + '-compile.log')).write_text(result.stdout)
        result.check_returncode()
        actual = list(struct.unpack('>' + 'I' * (len(model.elf_function(output, symbol)) // 4), model.elf_function(output, symbol)))
        expected = list(struct.unpack('>' + 'I' * (length // 4), model.read_dol(address, length)))
        assert len(actual) == len(expected), (name, actual, expected)
        if name == 'blend':
            # The one literal is 1.0f at retail r2 + 0x1b40; the object has an SDA relocation.
            assert model.read_dol(0x806BFC20 + 0x1B40, 4) == struct.pack('>f', 1.0)
            assert actual[0] == 0xC0000000 and expected[0] == 0xC0021B40
            actual[0] &= 0xFFE00000
            expected[0] &= 0xFFE00000
        if name in ('blend', 'set-vector'):
            # Tail-call relocation; original destination is checked independently.
            displacement = expected[-1] & 0x03FFFFFC
            if displacement & 0x02000000:
                displacement -= 0x04000000
            destination = address + length - 4 + displacement
            assert destination == (0x803E75C8 if name == 'blend' else 0x803ECFB0)
            actual[-1] &= 0xFC000003
            expected[-1] &= 0xFC000003
        assert actual == expected, name
        results[name] = {'address': hex(address), 'size': length, 'all_instructions_equal_modulo_relocations': True}
    return results


def numerical_witness():
    fma = ctypes.CDLL(None).fmaf
    fma.argtypes = [ctypes.c_float] * 3
    fma.restype = ctypes.c_float
    f32 = lambda value: struct.unpack('>f', struct.pack('>f', value))[0]
    from_value, to_value, first, second = [f32(value) for value in (1.0000001192092896, -1.0, 1.0000001192092896, 1.000000238418579)]
    product = f32(from_value * first)
    result = fma(to_value, second, product)
    rounded = {'inputs_float_bits': [struct.pack('>f', value).hex() for value in (from_value, to_value, first, second)],
               'rounded_first_product_bits': struct.pack('>f', product).hex(),
               'result_bits': struct.pack('>f', result).hex(),
               'first_product_must_round': result != f32(from_value * first + to_value * second)}
    assert rounded['first_product_must_round']
    from_value, to_value, first, second = [f32(value) for value in (1.0, 1.0000001192092896, -1.0, 0.9999998807907104)]
    product = f32(from_value * first)
    result = fma(to_value, second, product)
    fused = {'inputs_float_bits': [struct.pack('>f', value).hex() for value in (from_value, to_value, first, second)],
             'result_bits': struct.pack('>f', result).hex(),
             'second_product_must_fuse': result != f32(f32(to_value * second) + product)}
    assert fused['second_product_must_fuse']
    return {'rounded_first_product': rounded, 'fused_second_product': fused}


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    bodies = original_bodies()
    retail_graph, operations = retail_ps_graph()
    assert native_ps_graph(bodies['psblend']) == retail_graph
    # Both paths have loaded all six components before their first output store,
    # so the same graph applies when destination equals either or both inputs.
    cases = ['distinct', 'destination=from', 'destination=to', 'from=to=destination']
    compiler = compile_and_compare(bodies)
    evidence = {'scope': 'Original-compiler instructions and source/retail arithmetic graph; no native runtime claim',
                'dol_sha1': model.SHA1, 'matching_original_and_native_bodies': 4,
                'psblend_operations': operations, 'psblend_output_graph': retail_graph,
                'psblend_alias_cases': cases, 'compiler': compiler, 'numerical_witness': numerical_witness()}
    Path(__file__).with_name('helpers-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
