#!/usr/bin/env python3
"""Verify the real ModelData lifecycle imports with the original compiler.

This does not build or execute native code. Retail objects and compiler outputs
remain in the ignored build directory; the compact evidence is shareable.
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
BUILD = ROOT / 'build/xanime-core-model-data-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
SHA1 = '25c5959534b3c21246c6c7e42021b916b41fb578'
PROVIDER = ROOT / 'pc-port/src/compat/J3DModelDataCompat.cpp'
GROUPS = {
    'J3DGraphAnimator/J3DModelData': {
        'void J3DModelData::clear()': 'clear__12J3DModelDataFv',
        'J3DModelData::J3DModelData()': '__ct__12J3DModelDataFv',
        'J3DModelData::~J3DModelData()': '__dt__12J3DModelDataFv',
    },
    'J3DGraphAnimator/J3DMaterialAttach': {
        'void J3DMaterialTable::clear()': 'clear__16J3DMaterialTableFv',
        'J3DMaterialTable::J3DMaterialTable()': '__ct__16J3DMaterialTableFv',
        'J3DMaterialTable::~J3DMaterialTable()': '__dt__16J3DMaterialTableFv',
    },
    'J3DGraphBase/J3DVertex': {
        'J3DVertexData::J3DVertexData()': '__ct__13J3DVertexDataFv',
    },
}


def body(text, marker):
    start = text.index(marker)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return re.sub(r'\s+', '', text[start:end])


def correspondence():
    imported = PROVIDER.read_text()
    for unit, functions in GROUPS.items():
        source = (ROOT / 'src/JSystem' / (unit + '.cpp')).read_text()
        for marker in functions:
            assert body(source, marker) == body(imported, marker), marker
    for name in ('J3DModelData.hpp', 'J3DMaterialAttach.hpp', 'J3DShapeTable.hpp'):
        subpath = Path('JSystem/J3DGraphAnimator') / name
        source = (ROOT / 'libs/JSystem/include' / subpath).read_text()
        if name == 'J3DShapeTable.hpp':
            source = source.replace('#include "JSystem/J3DGraphBase/J3DShape.hpp"\n', '')
            source = source.replace('class JUTNameTab;',
                                    'class J3DDrawMtxData;\nclass J3DShape;\nclass J3DVertexData;\nclass JUTNameTab;')
        assert source == (ROOT / 'pc-port/src' / subpath).read_text(), name


def read_dol(address, size):
    data = DOL.read_bytes()
    assert hashlib.sha1(data).hexdigest() == SHA1
    for index in range(18):
        offset, base, length = [struct.unpack_from('>I', data, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            return data[offset + address - base:offset + address - base + size]
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
            code = sections[index]
            return data[code[4] + at:code[4] + at + length]
    raise AssertionError(name)


def compile_original():
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            break
    base = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
    for flag in flags:
        base.extend(shlex.split(flag))
    for unit in GROUPS:
        name = Path(unit).name
        command = base + ['-c', str(ROOT / 'src/JSystem' / (unit + '.cpp')), '-o', str(BUILD / (name + '.o'))]
        (BUILD / (name + '-compile-command.json')).write_text(json.dumps(command, indent=2) + '\n')
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / (name + '-compile.log')).write_text(result.stdout)
        result.check_returncode()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-jsystem', type=Path,
                        default=ROOT / 'build/original-j3d-joint-traversal-20260903/retail/obj/JSystem')
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    correspondence()
    retail_vertex = read_dol(0x804237F4, 0x80)
    compile_original()
    results = {}
    for unit, functions in GROUPS.items():
        name = Path(unit).name
        output = BUILD / (name + '-objdiff.json')
        subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(args.target_jsystem / (unit + '.o')),
                        '-2', str(BUILD / (name + '.o')), '-o', str(output), '--format', 'json-pretty'], cwd=ROOT, check=True)
        diff = json.loads(output.read_text())
        for function in functions.values():
            left, right = [next(s for s in diff[side]['symbols'] if s['name'] == function) for side in ('left', 'right')]
            results[function] = {'match_percent': left.get('match_percent'), 'retail_size': int(left['size']),
                                 'compiled_size': int(right['size'])}
    vertex = results['__ct__13J3DVertexDataFv']
    assert vertex['match_percent'] >= 95, vertex
    exact = elf_function(BUILD / 'J3DVertex.o', '__ct__13J3DVertexDataFv') == retail_vertex
    evidence = {'scope': 'Original compiler and exact source imports; native construction/runtime tested separately by parent',
                'dol_sha1': SHA1, 'imported_bodies': sum(map(len, GROUPS.values())),
                'model_and_material_headers': 'byte-identical',
                'shape_header_change': 'unnecessary Shape include replaced by three forward declarations',
                'vertex_ctor': {'address': '0x804237f4', 'retail_size': 128, 'exact_bytes': exact},
                'original_compiler_objdiff': results}
    output = Path(__file__).with_name('model-foundation-evidence.json')
    output.write_text(json.dumps(evidence, indent=2) + '\n')
    print(json.dumps(evidence, indent=2))


if __name__ == '__main__':
    main()
