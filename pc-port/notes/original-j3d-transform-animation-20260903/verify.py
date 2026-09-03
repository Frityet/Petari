#!/usr/bin/env python3
"""Verify original J3D transform source correspondence and retail arithmetic.

Only invokes the original PowerPC compiler and comparison tools; never a native
build. All object files, extracted code, and full objdiff results stay in build/.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / 'build/original-j3d-transform-animation-20260903'
SOURCE = ROOT / 'src/JSystem/J3DGraphAnimator/J3DAnimation.cpp'
PC_SOURCE = ROOT / 'pc-port/src/compat/J3DTransformAnimationCompat.cpp'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
SHA1 = '25c5959534b3c21246c6c7e42021b916b41fb578'
FUNCTIONS = {
    'getTransform__19J3DAnmTransformFullCFUsP16J3DTransformInfo': (0x8043436C, 0x360),
    'getTransform__27J3DAnmTransformFullWithLerpCFUsP16J3DTransformInfo': (0x804346CC, 0x824),
    'calcTransform__18J3DAnmTransformKeyCFfUsP16J3DTransformInfo': (0x80434EF0, 0x428),
    '__dt__19J3DAnmTransformFullFv': (0x80437524, 0x40),
    'J3DHermiteInterpolation__FfPCsPCsPCsPCsPCsPCs': (0x804354E8, 0x54),
    'J3DGetKeyFrameInterpolation<s>__FfP18J3DAnmKeyTableBasePs_f': (0x80435318, 0x1D0),
    'J3DGetKeyFrameInterpolation<f>__FfP18J3DAnmKeyTableBasePf_f': (0x8043553C, 0x120),
}


def block(text, marker):
    start = text.index(marker)
    begin = text.index('{', start)
    depth = 1
    end = begin + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def normalized(text):
    text = re.sub(r'//[^\n]*|/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'\s+', '', text)


def source_correspondence():
    root = SOURCE.read_text()
    pc = PC_SOURCE.read_text() + (ROOT / 'pc-port/src/compat/J3DAnimationInterpolation.hpp').read_text()
    root_header = (ROOT / 'libs/JSystem/include/JSystem/J3DGraphAnimator/J3DAnimation.hpp').read_text()
    pc_header = (ROOT / 'pc-port/src/JSystem/J3DGraphAnimator/J3DAnimation.hpp').read_text()
    checked = []
    for marker in ['struct J3DAnmKeyTableBase {', 'struct J3DAnmTransformKeyTable {',
                   'struct J3DAnmTransformFullTable {', 'class J3DAnmTransformKey :',
                   'class J3DAnmTransformFull :', 'class J3DAnmTransformFullWithLerp :']:
        assert normalized(block(root_header, marker)) == normalized(block(pc_header, marker)), marker
        checked.append(marker)
    for marker in ['J3DAnmTransform::J3DAnmTransform(', 'J3DAnmTransformFull::~J3DAnmTransformFull()',
                   'void J3DAnmTransformFull::getTransform(', 'void J3DAnmTransformFullWithLerp::getTransform(',
                   'void J3DAnmTransformKey::calcTransform(', 'f32 J3DGetKeyFrameInterpolation(',
                   'inline f32 J3DHermiteInterpolation(f32 p1,']:
        expected = normalized(block(root, marker))
        expected = expected.replace('u32frame=(int)(mFrame+0.5f);', 'u32frame=static_cast<u32>(truncatePpcInteger(mFrame+0.5f));')
        expected = expected.replace('intframe=(int)mFrame;', 'intframe=truncatePpcInteger(mFrame);')
        expected = expected.replace('u32next_frame=frame+1;', 'u32next_frame=static_cast<u32>(frame)+1U;')
        expected = expected.replace('intdelta=rot2-rot1;', 'intdelta=static_cast<int>(rot2)-static_cast<int>(rot1);')
        for axis in 'xyz':
            expected = expected.replace(f'pTransform->mRotation.{axis}=(u32)((f32)rot1+rate*(f32)delta);',
                                        f'pTransform->mRotation.{axis}=narrowPpcRotation(static_cast<u32>((f32)rot1+rate*(f32)delta));')
            upper = axis.upper()
            expected = expected.replace(f'mRotData[entry{upper}->mRotationInfo.mOffset]<<mDecShift',
                                        f'shiftPpcRotation(mRotData[entry{upper}->mRotationInfo.mOffset],mDecShift)')
            interp = f'J3DGetKeyFrameInterpolation(frame,&entry{upper}->mRotationInfo,&mRotData[entry{upper}->mRotationInfo.mOffset])'
            expected = expected.replace(f'(int){interp}<<mDecShift', f'shiftPpcRotation(truncatePpcInteger({interp}),mDecShift)')
        assert expected == normalized(block(pc, marker)), marker
        checked.append(marker)
    portable = block(root, 'inline f32 J3DHermiteInterpolation(__REGISTER').split('#else', 1)[1].split('#endif', 1)[0]
    pc_portable = block(pc, 'inline f32 J3DHermiteInterpolation(f32 pp1,').split('{', 1)[1].rsplit('}', 1)[0]
    assert normalized(portable) == normalized(pc_portable)
    assert '#pragma clang fp contract(off)' in pc and '#pragma GCC optimize("fp-contract=off")' in pc
    return checked, portable


def read_dol(data, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from('>I', data, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start:start + size]
    raise AssertionError((hex(address), size))


def operation(op, *args):
    # Commutative scalar multiply operands are exchanged by register scheduling.
    if op in ('mul', 'add'):
        args = tuple(sorted(args, key=repr))
    return (op, *args)


def portable_graph(body):
    values = {'pp1': 'frame', 'p1': 'frame', **{f'p{i}': f'arg{i}' for i in range(2, 8)}}
    def expr(node):
        if isinstance(node, ast.Name):
            return values[node.id]
        if isinstance(node, ast.BinOp):
            op = {ast.Add: 'add', ast.Sub: 'sub', ast.Mult: 'mul', ast.Div: 'div'}[type(node.op)]
            return operation(op, expr(node.left), expr(node.right))
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return operation('neg', expr(node.operand))
        if isinstance(node, ast.Call) and node.func.id == 'fma':
            return operation('fma', *map(expr, node.args))
        raise AssertionError(ast.dump(node))
    body = re.sub(r'//[^\n]*', '', body)
    for line in body.splitlines():
        line = line.strip().replace('std::fma', 'fma')
        line = re.sub(r'\*pp([2-7])', r'p\1', line)
        if not line:
            continue
        if line.startswith('return '):
            return expr(ast.parse(line[7:-1], mode='eval').body)
        assignment = re.fullmatch(r'(?:f32 )?(\w+) = (.+);', line)
        assert assignment, line
        values[assignment[1]] = expr(ast.parse(assignment[2], mode='eval').body)
    raise AssertionError('Portable Hermite return absent')


def retail_graph(data):
    regs = {1: 'frame'}
    ops = []
    for index, word in enumerate(struct.unpack('>' + str(len(data) // 4) + 'I', data)):
        if word == 0x4E800020:
            assert index == len(data) // 4 - 1
            break
        op = word >> 26
        d, a, b, c = [(word >> shift) & 31 for shift in (21, 16, 11, 6)]
        if op == 56:  # psq_l, W=1, GQR5, offset=0
            assert ((word >> 12) & 15) == 13 and (word & 0xFFF) == 0
            assert 3 <= a <= 8
            regs[d] = f'arg{a - 1}'
            ops.append('psq_l_s16')
        elif op == 48:  # lfs through an input pointer, offset=0
            assert (word & 0xFFFF) == 0 and 3 <= a <= 8
            regs[d] = f'arg{a - 1}'
            ops.append('lfs')
        else:
            assert op == 59
            xo = (word >> 1) & 31
            if xo in (18, 20, 21):
                name = {18: 'div', 20: 'sub', 21: 'add'}[xo]
                regs[d] = operation(name, regs[a], regs[b])
            elif xo == 25:
                name = 'mul'
                regs[d] = operation(name, regs[a], regs[c])
            elif xo == 29:
                name = 'fma'
                regs[d] = operation(name, regs[a], regs[c], regs[b])
            elif xo == 28:
                name = 'fmsub'
                regs[d] = operation('fma', regs[a], regs[c], operation('neg', regs[b]))
            elif xo == 30:
                name = 'negated_fmsub'
                regs[d] = operation('neg', operation('fma', regs[a], regs[c], operation('neg', regs[b])))
            else:
                raise AssertionError(hex(word))
            ops.append(name)
    return regs[1], ops


def compile_original():
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            break
    command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
    for flag in flags:
        command.extend(shlex.split(flag))
    command.extend(['-c', str(SOURCE), '-o', str(BUILD / 'J3DAnimation.o')])
    (BUILD / 'compile-command.json').write_text(json.dumps(command, indent=2) + '\n')
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / 'compile.log').write_text(result.stdout)
    result.check_returncode()


def compare(target):
    if target is None:
        target = BUILD / 'retail/obj/JSystem/J3DGraphAnimator/J3DAnimation.o'
        if not target.exists():
            config = (ROOT / 'config/RMGK01/config.yml').read_text()
            config = config.replace('object_base: orig/RMGK01', 'object_base: ' + str(DOL.parent))
            config = config.replace('object: sys/main.dol', 'object: ' + DOL.name)
            for kind in ('symbols', 'splits'):
                config = config.replace(kind + ': config/', kind + ': ' + str(ROOT / 'config') + '/')
            (BUILD / 'config.yml').write_text(config)
            with (BUILD / 'dtk.log').open('w') as log:
                subprocess.run([str(ROOT / 'build/tools/dtk'), 'dol', 'split', '--no-update', '-j', '2',
                                str(BUILD / 'config.yml'), str(BUILD / 'retail')], cwd=ROOT, check=True,
                               stdout=log, stderr=subprocess.STDOUT)
    subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(target), '-2', str(BUILD / 'J3DAnimation.o'),
                    '-o', str(BUILD / 'objdiff.json'), '--format', 'json-pretty'], cwd=ROOT, check=True)
    diff = json.loads((BUILD / 'objdiff.json').read_text())
    result = {}
    for name in FUNCTIONS:
        left, right = [next(s for s in diff[side]['symbols'] if s['name'] == name) for side in ('left', 'right')]
        result[name] = {'match_percent': left.get('match_percent'), 'retail_size': int(left['size']), 'compiled_size': int(right['size'])}
        if name in list(FUNCTIONS)[:4]:
            assert left['match_percent'] == 100.0, name
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-object', type=Path, help='Reuse a DTK target split of the verified supplied DOL')
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == SHA1
    checked, portable = source_correspondence()
    graph, ops = retail_graph(read_dol(dol, 0x804354E8, 0x54))
    assert graph == portable_graph(portable), 'Portable signed16 Hermite dataflow differs from retail'
    float_graph, float_ops = retail_graph(read_dol(dol, 0x8043565C, 0x50))
    jma = (ROOT / 'pc-port/src/JSystem/JMath/JMath.hpp').read_text()
    jma_body = block(jma, 'inline f32 JMAHermiteInterpolation(').split('{', 1)[1].rsplit('}', 1)[0]
    assert float_graph == portable_graph(jma_body), 'Shared JMA helper dataflow differs from retail float Hermite'
    # Verify exact sample instructions and constants at the points motivating
    # the native substitutions, not merely the presence of opcodes elsewhere.
    checks = {
        0x8043509C: 0x7C641AAE, 0x804350A0: 0x7C600030, 0x804350A4: 0xB01C000C,
        0x804350C8: 0xFC00081E, 0x804350D8: 0x7C600030, 0x804350DC: 0xB01C000C,
        0x80434454: 0xEC00082A, 0x80434458: 0xFC00001E,
        0x804347D8: 0xFC00101E, 0x80434A70: 0x3B7C0001,
        0x80434ACC: 0xEC000828, 0x80434AD0: 0xEC1F0032, 0x80434AD4: 0xEC01002A,
        0x80434B84: 0xEC1F0032, 0x80434B88: 0xEC21002A, 0x80434B90: 0xB07A000C,
    }
    for address, word in checks.items():
        assert read_dol(dol, address, 4) == struct.pack('>I', word), hex(address)
    assert read_dol(dol, 0x806BFC20 + 0x1F2C, 4) == struct.pack('>f', 0.5)
    # The GQR5 load fields select unscaled signed16 conversion. J3DSys's
    # drawInit initializes that register with 0x00070007, as OSInitFastCast does.
    draw = read_dol(dol, 0x804228A0, 0x6C0)
    gqr = bytes.fromhex('38600007 64630007 7c75e3a6')
    assert draw.count(gqr) == 1
    compile_original()
    evidence = {
        'scope': 'Original compiler/source and retail numerical dataflow; native runtime tests owned by parent',
        'dol_sha1': SHA1,
        'sources_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in
                           (SOURCE, PC_SOURCE, ROOT / 'pc-port/src/JSystem/J3DGraphAnimator/J3DAnimation.hpp')},
        'source_correspondence': checked,
        'integer_and_lerp_oracle_words': {hex(a): f'{w:08x}' for a, w in checks.items()},
        'gqr5_initialization_address': hex(0x804228A0 + draw.index(gqr)),
        'signed16_hermite': {'operations': ops, 'portable_and_retail_graph_equal': True,
                            'graph_sha256': hashlib.sha256(repr(graph).encode()).hexdigest()},
        'float_hermite': {'operations': float_ops, 'shared_jma_and_retail_graph_equal': True,
                          'graph_sha256': hashlib.sha256(repr(float_graph).encode()).hexdigest()},
        'original_compiler_objdiff': compare(args.target_object),
        'retail_functions': {n: {'address': hex(a), 'size': size,
                                'sha256': hashlib.sha256(read_dol(dol, a, size)).hexdigest()}
                             for n, (a, size) in FUNCTIONS.items()},
    }
    (BUILD / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
