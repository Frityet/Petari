#!/usr/bin/env python3
"""Original-compiler before/after proof for two native pointer-width fixes."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import struct
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-model-manager-render-20260903/width-proof'
SOURCE = ROOT / 'src/Game/Util/LiveActorUtil.cpp'


def elf_functions(path):
    data = path.read_bytes()
    assert data[:6] == b'\x7fELF\x01\x02'
    offset = struct.unpack_from('>I', data, 0x20)[0]
    size, count = struct.unpack_from('>HH', data, 0x2e)
    sections = [struct.unpack_from('>10I', data, offset + i * size) for i in range(count)]
    symtab = next(s for s in sections if s[1] == 2)
    strings = sections[symtab[6]]
    names = data[strings[4]:strings[4] + strings[5]]
    symbols = []
    for offset in range(symtab[4], symtab[4] + symtab[5], symtab[9]):
        n, at, length, info, other, index = struct.unpack_from('>IIIBBH', data, offset)
        symbols.append((names[n:names.index(0, n)].decode(), at, length, info, index))
    result = {}
    for name, at, length, info, index in symbols:
        if not index or info & 15 != 2:
            continue
        sec = sections[index]
        code = data[sec[4] + at:sec[4] + at + length]
        relocs = []
        for reloc in sections:
            if reloc[1] != 4 or reloc[7] != index:
                continue
            for where in range(reloc[4], reloc[4] + reloc[5], reloc[9]):
                offset, kind, addend = struct.unpack_from('>IIi', data, where)
                if at <= offset < at + length:
                    relocs.append({'offset': offset - at, 'kind': kind & 255,
                                   'symbol': symbols[kind >> 8][0], 'addend': addend})
        result[name] = {'size': length, 'bytes': code.hex(), 'relocations': relocs}
    return result


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location('access', HERE / 'verify-native-access.py')
    access = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(access)
    text = SOURCE.read_text()
    names = ['getModelResourceHolder', 'getModelResName', 'setJointTransformLocalMtx']
    bodies = '\n\n'.join(body for _, body, _ in access.functions(text, names))
    original = bodies.replace('getResName(static_cast< u32 >(0))', 'getResName(0UL)')
    original = original.replace('pTransform->_68 = pMtx;', 'pTransform->_68 = (u32)pMtx;')
    assert bodies != original
    includes = '\n'.join('#include "' + name + '"' for name in [
        'Game/Animation/XanimeCore.hpp', 'Game/LiveActor/LiveActor.hpp',
        'Game/LiveActor/ModelManager.hpp', 'Game/Util/LiveActorUtil.hpp',
        'Game/System/ResourceHolder.hpp']) + '\n\nnamespace MR {\n'
    old_header = ROOT / 'include/Game/Animation/XanimeCore.hpp'
    old_text = old_header.read_text()
    assert old_text.count('MtxPtr _68;') == 1
    shadow = BUILD / 'old-header/Game/Animation/XanimeCore.hpp'
    shadow.parent.mkdir(parents=True, exist_ok=True)
    # The removed integer assignment used the former original-width integer
    # field. Recreate only that historical type at the ILP32 compiler boundary.
    shadow.write_text(old_text.replace('MtxPtr _68;', 'u32 _68;'))
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_game' for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            break
    else:
        raise AssertionError('Configured Game compiler flags not found')
    outputs, commands = {}, {}
    for label, content in [('before', original), ('after', bodies)]:
        source = BUILD / (label + '.cpp')
        source.write_text(includes + content + '\n}\n')
        command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
        if label == 'before':
            command += ['-i', str(BUILD / 'old-header')]
        for flag in flags:
            command += shlex.split(flag)
        command += ['-c', str(source), '-o', str(BUILD / (label + '.o'))]
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (HERE / ('width-' + label + '.compile.log')).write_text(result.stdout)
        commands[label] = command
        result.check_returncode()
        outputs[label] = elf_functions(BUILD / (label + '.o'))
    names = ['getModelResName__2MRFPC9LiveActor', 'setJointTransformLocalMtx__2MRFPC9LiveActorPCcPA4_f']
    evidence = {}
    for name in names:
        before, after = (outputs[side][name] for side in ['before', 'after'])
        assert before == after, (name, before, after)
        evidence[name] = {'identical_code_and_relocation_targets': True, **after}
    report = {'scope': 'ILP32 GC3.0a3 code and relocation equivalence before/after. Not a new retail fuzzy-match claim.',
              'root_source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              'original_pointer_field_reconstruction': 'Only _68 is u32 in baseline shadow header; corrected original field is MtxPtr. Both are 32 bits on Wii.',
              'commands': commands, 'functions': evidence}
    (HERE / 'width-fix-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: both changes preserve original-compiler bytes and relocation targets')


if __name__ == '__main__':
    main()
