"""Independent scalar dataflow and reciprocal bit models for the retained proof."""
import ast
from pathlib import Path
import re
import struct


def operation(op, *args):
    if op in ('add', 'mul'):
        args = tuple(sorted(args, key=repr))
    return (op, *args)


def source_matrix_graph(body):
    values = {**{f'r{a}': ('rotation', a) for a in 'xyz'}, **{f't{a}': ('translation', a) for a in 'xyz'}}
    output = {}
    def expr(node):
        if isinstance(node, ast.Name):
            return values[node.id]
        if isinstance(node, ast.BinOp):
            return operation({ast.Add: 'add', ast.Sub: 'sub', ast.Mult: 'mul'}[type(node.op)], expr(node.left), expr(node.right))
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return operation('neg', expr(node.operand))
        if isinstance(node, ast.Call):
            axis = expr(node.args[0])[1]
            return ('sin' if node.func.id == 'JMASSin' else 'cos', axis)
        raise AssertionError(ast.dump(node))
    for axis in 'xyz':
        body = body.replace(f'tx.mRotation.{axis}', f'r{axis}').replace(f'tx.mTranslate.{axis}', f't{axis}')
    for line in body.splitlines():
        line = line.strip().removeprefix('f32 ')
        if not line or '=' not in line:
            continue
        for statement in line.removesuffix(';').split(', '):
            lhs, rhs = statement.split(' = ')
            value = expr(ast.parse(rhs, mode='eval').body)
            if lhs.startswith('dst['):
                row, col = map(int, re.findall(r'\d', lhs))
                output[row * 16 + col * 4] = value
            else:
                values[lhs] = value
    return output


def retail_matrix_graph(data, scalar):
    if scalar:
        gpr = {3: ('rotation', 'x'), 4: ('rotation', 'y'), 5: ('rotation', 'z'), 6: 'dst'}
        fpr = {1: ('translation', 'x'), 2: ('translation', 'y'), 3: ('translation', 'z')}
    else:
        gpr, fpr = {3: 'transform', 4: 'dst'}, {}
    output = {}
    table_address = 0x8060FC80
    def signed16(x):
        return x - 65536 if x & 32768 else x
    def add(a, b):
        if isinstance(a, int) and isinstance(b, int):
            return (a + b) & 0xFFFFFFFF
        assert a == table_address and b[0] == 'index', (a, b)
        return ('table', b[1])
    for word in struct.unpack('>' + str(len(data) // 4) + 'I', data):
        op = word >> 26
        d, a, b, c = [(word >> shift) & 31 for shift in (21, 16, 11, 6)]
        if word == 0x4E800020:
            break
        if op == 15:
            assert a == 0
            gpr[d] = (signed16(word & 65535) << 16) & 0xFFFFFFFF
        elif op == 14:
            if a != 1:
                gpr[d] = add(gpr[a], signed16(word & 65535))
        elif op == 42:
            assert gpr[a] == 'transform' and (word & 65535) in (12, 14, 16)
            gpr[d] = ('rotation', 'xyz'[((word & 65535) - 12) // 2])
        elif op == 21:
            assert b == 1 and ((word >> 6) & 31) == 15 and ((word >> 1) & 31) == 28
            assert gpr[d][0] == 'rotation'
            gpr[a] = ('index', gpr[d][1])
        elif op == 31:
            xo = (word >> 1) & 1023
            if xo == 266:
                gpr[d] = add(gpr[a], gpr[b])
            elif xo == 535:
                pointer = add(gpr[a], gpr[b])
                fpr[d] = ('sin', pointer[1])
            else:
                raise AssertionError(hex(word))
        elif op == 48:
            pointer, offset = gpr[a], word & 65535
            if pointer == 'transform':
                assert offset in (20, 24, 28)
                fpr[d] = ('translation', 'xyz'[(offset - 20) // 4])
            else:
                assert pointer[0] == 'table' and offset == 4
                fpr[d] = ('cos', pointer[1])
        elif op == 52:
            assert gpr[a] == 'dst'
            output[word & 65535] = fpr[d]
        elif op == 59:
            xo = (word >> 1) & 31
            if xo == 25:
                fpr[d] = operation('mul', fpr[a], fpr[c])
            else:
                assert xo in (20, 21)
                fpr[d] = operation('sub' if xo == 20 else 'add', fpr[a], fpr[b])
        elif op == 63:
            assert ((word >> 1) & 1023) == 40
            fpr[d] = operation('neg', fpr[b])
        else:
            # Register-save/restore traffic to the stack does not enter output.
            assert op in (37, 50, 54, 56, 60) and a == 1, hex(word)
    assert sorted(output) == list(range(0, 48, 4))
    return output


def reciprocal_cases(root):
    oracle_source = (root / 'pc-port/dolphin/Source/Core/Common/FloatUtils.cpp').read_text()
    source = (root / 'pc-port/src/compat/J3DJointCompat.cpp').read_text()
    def read_table(text, marker):
        text = text[text.index(marker):]
        text = text[:text.index('};')]
        return [(int(a, 16), int(b, 16)) for a, b in re.findall(r'\{(0x[0-9a-f]+), (0x[0-9a-f]+)\}', text)]
    oracle_table = read_table(oracle_source, 'fres_expected')
    native_table = read_table(source, 'reciprocalEstimateTable')
    assert oracle_table == native_table and len(native_table) == 32
    def native(bits):
        sign, fraction, exponent = bits & 0x80000000, bits & 0x7FFFFF, (bits >> 23) & 255
        if exponent == 255:
            return bits | 0x400000 if fraction else sign
        if exponent == 0:
            if fraction == 0:
                return sign | 0x7F800000
            if fraction < 0x200000:
                return sign | 0x7F7FFFFF
            shift = 2 if fraction < 0x400000 else 1
            fraction, exponent = (fraction << shift) & 0x7FFFFF, 1 - shift
        if exponent >= 253:
            return sign
        base, decrement = native_table[fraction >> 18]
        return sign | ((253 - exponent) << 23) | (base - ((decrement * ((fraction >> 8) & 1023) + 1) >> 1))
    def dolphin(bits):
        value = struct.unpack('>f', struct.pack('>I', bits))[0]
        raw = struct.unpack('>Q', struct.pack('>d', value))[0]
        mantissa, exponent, sign = raw & ((1 << 52) - 1), (raw >> 52) & 2047, bits & 0x80000000
        if mantissa == 0 and exponent == 0:
            return sign | 0x7F800000
        if exponent == 2047:
            return bits | 0x400000 if bits & 0x7FFFFF else sign
        if exponent < 895:
            return sign | 0x7F7FFFFF
        if exponent >= 1149:
            return sign
        index = mantissa >> 37
        base, decrement = oracle_table[index // 1024]
        result = ((raw >> 63) << 63) | ((2045 - exponent) << 52) | ((base - (decrement * (index % 1024) + 1) // 2) << 29)
        return struct.unpack('>I', struct.pack('>f', struct.unpack('>d', struct.pack('>Q', result))[0]))[0]
    count = 0
    for exponent in (0, 1, 2, 126, 127, 128, 252, 253, 254, 255):
        for fraction in range(0, 1 << 23, 256):
            for tail in (0, 255):
                for sign in (0, 0x80000000):
                    bits = sign | (exponent << 23) | fraction | tail
                    assert native(bits) == dolphin(bits), hex(bits)
                    count += 1
    samples = (0, 0x80000000, 1, 0x1FFFFF, 0x200000, 0x3FFFFF, 0x400000, 0x7FFFFF,
               0x3F800000, 0x40000000, 0x40800000, 0x7E800000, 0x7F800000, 0x7F800001, 0xFF800001)
    for bits in samples:
        assert native(bits) == dolphin(bits)
    return {'source_model_vs_oracle_cases': count, 'representative_bits': {f'{b:08x}': f'{native(b):08x}' for b in samples}}
