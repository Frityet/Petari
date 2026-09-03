#!/usr/bin/env python3
"""Original-compiler, source correspondence, and retail math verification.

No native build. Compiler objects and full disassembly comparisons stay in the
ignored build directory; only compact evidence is intended for version control.
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
import sys
sys.dont_write_bytecode = True
from math_oracle import reciprocal_cases, retail_matrix_graph, source_matrix_graph

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / 'build/original-j3d-joint-traversal-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
SHA1 = '25c5959534b3c21246c6c7e42021b916b41fb578'
UNITS = {'J3DJoint': 'J3DGraphAnimator', 'J3DTransform': 'J3DGraphBase', 'JMath': 'JMath'}


def block(text, marker):
    start = text.index(marker)
    begin = text.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def normalized(text):
    return re.sub(r'\s+', '', re.sub(r'//[^\n]*|/\*.*?\*/', '', text, flags=re.S))


def read_dol(data, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from('>I', data, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start:start + size]
    raise AssertionError(hex(address))


def elf_function(path, name):
    data = path.read_bytes()
    assert data[:6] == b'\x7fELF\x01\x02'
    offset = struct.unpack_from('>I', data, 0x20)[0]
    size, count = struct.unpack_from('>HH', data, 0x2E)
    sections = [struct.unpack_from('>10I', data, offset + i * size) for i in range(count)]
    section = next(s for s in sections if s[1] == 2)
    strings = sections[section[6]]
    names = data[strings[4]:strings[4] + strings[5]]
    for offset in range(section[4], section[4] + section[5], section[9]):
        n, at, length, info, other, index = struct.unpack_from('>IIIBBH', data, offset)
        if names[n:names.index(0, n)].decode() == name:
            body = sections[index]
            return data[body[4] + at:body[4] + at + length]
    raise AssertionError(name)


def correspondence():
    pc = (ROOT / 'pc-port/src/compat/J3DJointCompat.cpp').read_text()
    joint = (ROOT / 'src/JSystem/J3DGraphAnimator/J3DJoint.cpp').read_text()
    transform = (ROOT / 'src/JSystem/J3DGraphBase/J3DTransform.cpp').read_text()
    jmath = (ROOT / 'src/JSystem/JMath/JMath.cpp').read_text()
    markers = ['void J3DMtxCalcJ3DSysInitBasic::init(', 'void J3DMtxCalcJ3DSysInitMaya::init(',
               'inline s32 checkScaleOne(', 'void J3DMtxCalcCalcTransformBasic::calcTransform(',
               'void J3DMtxCalcCalcTransformSoftimage::calcTransform(', 'void J3DMtxCalcCalcTransformMaya::calcTransform(',
               'void J3DJoint::appendChild(', 'J3DJoint::J3DJoint()', 'void J3DJoint::recursiveCalc()']
    for marker in markers:
        assert normalized(block(joint, marker)) == normalized(block(pc, marker)), marker
    for source, extra in [(transform, ['void J3DGetTranslateRotateMtx(const J3DTransformInfo&', 'void J3DGetTranslateRotateMtx(s16']),
                          (jmath, ['void JMAMTXApplyScale('])]:
        for marker in extra:
            assert normalized(block(source, marker)) == normalized(block(pc, marker)), marker
            markers.append(marker)
    header = (ROOT / 'libs/JSystem/include/JSystem/J3DGraphAnimator/J3DJoint.hpp').read_text()
    native = (ROOT / 'pc-port/src/JSystem/J3DGraphAnimator/J3DJoint.hpp').read_text()
    header = header.replace('#include "JSystem/J3DGraphBase/J3DMaterial.hpp"', '#include "JSystem/J3DGraphBase/J3DSys.hpp"')
    mesh = block(header, '    inline void addMesh(')
    header = header.replace(mesh, '    void addMesh(J3DMaterial* pMesh);')
    assert normalized(header) == normalized(native)
    assert (ROOT / 'libs/JSystem/include/JSystem/J3DGraphAnimator/J3DMtxCalc.hpp').read_bytes() == \
           (ROOT / 'pc-port/src/JSystem/J3DGraphAnimator/J3DMtxCalc.hpp').read_bytes()
    assert '#pragma clang fp contract(off)' in pc
    assert 'return __fres(value);' in block(jmath, 'f32 JMath::fastReciprocal(')
    return markers, transform


def compile_original():
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            break
    base = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
    for flag in flags:
        base.extend(shlex.split(flag))
    for name, folder in UNITS.items():
        command = base + ['-c', str(ROOT / 'src/JSystem' / folder / (name + '.cpp')), '-o', str(BUILD / (name + '.o'))]
        (BUILD / (name + '-compile-command.json')).write_text(json.dumps(command, indent=2) + '\n')
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / (name + '-compile.log')).write_text(result.stdout)
        result.check_returncode()


def compare(target):
    if target is None:
        target = BUILD / 'retail/obj/JSystem'
        if not target.exists():
            config = (ROOT / 'config/RMGK01/config.yml').read_text()
            config = config.replace('object_base: orig/RMGK01', 'object_base: ' + str(DOL.parent))
            config = config.replace('object: sys/main.dol', 'object: ' + DOL.name)
            for key in ('symbols', 'splits'):
                config = config.replace(key + ': config/', key + ': ' + str(ROOT / 'config') + '/')
            (BUILD / 'config.yml').write_text(config)
            with (BUILD / 'dtk.log').open('w') as log:
                subprocess.run([str(ROOT / 'build/tools/dtk'), 'dol', 'split', '--no-update', '-j', '2',
                                str(BUILD / 'config.yml'), str(BUILD / 'retail')], cwd=ROOT, check=True,
                               stdout=log, stderr=subprocess.STDOUT)
    result = {}
    for name, folder in UNITS.items():
        dest = BUILD / (name + '-objdiff.json')
        subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(target / folder / (name + '.o')),
                        '-2', str(BUILD / (name + '.o')), '-o', str(dest), '--format', 'json-pretty'], cwd=ROOT, check=True)
        diff = json.loads(dest.read_text())
        for symbol in diff['left']['symbols']:
            n = symbol['name']
            if n.startswith(('init__25J3DMtxCalcJ3DSys', 'init__24J3DMtxCalcJ3DSys', 'calcTransform__28J3D',
                             'calcTransform__32J3D', 'calcTransform__27J3D', 'appendChild__8J3DJoint',
                             '__ct__8J3DJoint', 'recursiveCalc__8J3DJoint', 'JMAMTXApplyScale', 'J3DGetTranslateRotateMtx')):
                compiled = next(x for x in diff['right']['symbols'] if x['name'] == n)
                result[n] = {'match_percent': symbol.get('match_percent'), 'retail_size': int(symbol['size']), 'compiled_size': int(compiled['size'])}
    assert result['recursiveCalc__8J3DJointFv']['match_percent'] >= 90
    assert result['JMAMTXApplyScale__FPA4_CfPA4_ffff']['match_percent'] == 100
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-jsystem', type=Path, help='Reuse an existing DTK retail/obj/JSystem directory')
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == SHA1
    markers, transform = correspondence()
    graphs = {}
    for marker, address, size, scalar in [('void J3DGetTranslateRotateMtx(const J3DTransformInfo&', 0x80423CD4, 0xF0, False),
                                          ('void J3DGetTranslateRotateMtx(s16', 0x80423DC4, 0xB0, True)]:
        body = block(transform, marker).split('{', 1)[1].rsplit('}', 1)[0]
        graph = source_matrix_graph(body)
        assert graph == retail_matrix_graph(read_dol(dol, address, size), scalar), marker
        graphs[hex(address)] = {'all_12_float_outputs_equal': True, 'graph_sha256': hashlib.sha256(repr(graph).encode()).hexdigest()}
    default = struct.pack('>3f3h2x3f', 1, 1, 1, 0, 0, 0, 0, 0, 0)
    assert read_dol(dol, 0x8055C1B8, 0x20) == default
    compile_original()
    reciprocal_code = elf_function(BUILD / 'JMath.o', 'fastReciprocal__5JMathFf')
    assert reciprocal_code == read_dol(dol, 0x8001B140, 8) == bytes.fromhex('ec200830 4e800020')
    evidence = {'scope': 'Original compiler, source/dataflow verification, and Python reciprocal model; no native build or runtime claim',
                'dol_sha1': SHA1, 'source_correspondence': markers, 'translation_rotation_graphs': graphs,
                'default_transform_address': '0x8055c1b8', 'default_transform_sha256': hashlib.sha256(default).hexdigest(),
                'fast_reciprocal': {'root_compiler_matches_retail': True, 'retail_address': '0x8001b140', 'words': reciprocal_code.hex(),
                                    **reciprocal_cases(ROOT)}, 'original_compiler_objdiff': compare(args.target_jsystem)}
    files = [ROOT / 'src/JSystem' / folder / (name + '.cpp') for name, folder in UNITS.items()]
    files += [ROOT / 'pc-port/src' / p for p in ['compat/J3DJointCompat.cpp', 'JSystem/J3DGraphAnimator/J3DJoint.hpp',
                                               'JSystem/J3DGraphAnimator/J3DMtxCalc.hpp', 'JSystem/JMath/JMath.hpp']]
    evidence['source_sha256'] = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    (BUILD / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
