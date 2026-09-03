#!/usr/bin/env python3
"""Compile real root TUs and compare imported methods with the verified retail DOL.

No native/shared build or header overlays. Reuses the independently split retail
objects when present; otherwise extracts them from the verified DOL with dtk.
"""
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
BUILD = ROOT / 'build/original-j3d-model-owner-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
RET = ROOT / 'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj'
UNITS = {
    'J3DModel': 'src/JSystem/J3DGraphAnimator/J3DModel.cpp',
    'J3DModelX': 'src/Game/Player/J3DModelX.cpp',
    'J3DCluster': 'src/JSystem/J3DGraphAnimator/J3DCluster.cpp',
    'J3DSkinDeform': 'src/JSystem/J3DGraphAnimator/J3DSkinDeform.cpp',
    'J3DMaterial': 'src/JSystem/J3DGraphBase/J3DMaterial.cpp',
    'J3DModelData': 'src/JSystem/J3DGraphAnimator/J3DModelData.cpp',
    'J3DTevs': 'src/JSystem/J3DGraphBase/J3DTevs.cpp',
}
MATERIAL = ('initialize', 'countDLSize', 'makeDisplayList_private', 'makeDisplayList',
            'makeSharedDisplayList', 'load', 'loadSharedDL', 'patch', 'diff', 'calc',
            'calcDiffTexMtx', 'setCurrentMtx', 'calcCurrentMtx', 'copy', 'reset', 'change',
            'newSharedDisplayList', 'newSingleSharedDisplayList')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, name):
    proc = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True)
    (BUILD / (name + '.log')).write_text(proc.stdout)
    proc.check_returncode()


def compiler(unit):
    wanted = 'cflags_game' if unit == 'J3DModelX' else 'cflags_jsys'
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == wanted for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            break
    else:
        raise AssertionError(wanted)
    command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
    for value in flags:
        command.extend(shlex.split(value))
    return command


def dol_bytes(dol, address, size):
    for i in range(18):
        offset, base, length = [struct.unpack_from('>I', dol, f + 4*i)[0] for f in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            return dol[offset + address-base:offset + address-base + size]
    raise AssertionError(hex(address))


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b'\x7fELF\x01\x02'
        offset = struct.unpack_from('>I', self.data, 0x20)[0]
        stride, count = struct.unpack_from('>HH', self.data, 0x2E)
        self.sections = [struct.unpack_from('>10I', self.data, offset+i*stride) for i in range(count)]
        section = next(s for s in self.sections if s[1] == 2)
        names = self.section_data(section[6])
        self.symbols = []
        for off in range(section[4], section[4]+section[5], section[9]):
            name, value, size, info, other, index = struct.unpack_from('>IIIBBH', self.data, off)
            self.symbols.append((names[name:names.index(0, name)].decode(), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4]+section[5]]

    def code_and_refs(self, name):
        _, start, size, index = next(s for s in self.symbols if s[0] == name)
        code = self.section_data(index)[start:start+size]
        refs = []
        for section in self.sections:
            if section[1] != 4 or section[7] != index:
                continue
            for off in range(section[4], section[4]+section[5], section[9]):
                at, info, addend = struct.unpack_from('>IIi', self.data, off)
                if start <= at < start+size:
                    refs.append({'offset': at-start, 'kind': info & 255,
                                 'symbol': self.symbols[info >> 8][0], 'addend': addend})
        return code, refs


def selected(unit, name):
    if unit == 'J3DModelX': return name == '__dt__8J3DModelFv'
    if unit == 'J3DModel': return '__8J3DModel' in name
    if unit == 'J3DCluster': return name.startswith(('deform__', 'deform_Vtx', 'normalizeWeight__'))
    if unit == 'J3DSkinDeform': return name.startswith(('deform__13J3DSkinDeform', 'calc__15J3DVtxColorCalc'))
    if unit == 'J3DMaterial': return name.split('__')[0] in MATERIAL and '__11J3DMaterial' in name
    if unit == 'J3DModelData': return '__12J3DModelData' in name
    if unit == 'J3DTevs': return name == 'loadNBTScale__FR11J3DNBTScale'
    return False


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    retail = RET
    if not all((retail / p[4:].replace('.cpp', '.o')).exists() for p in UNITS.values()):
        config = (ROOT / 'config/RMGK01/config.yml').read_text()
        config = config.replace('object_base: orig/RMGK01', 'object_base: '+str(DOL.parent))
        config = config.replace('object: sys/main.dol', 'object: '+DOL.name)
        config = config.replace('symbols: config/', 'symbols: '+str(ROOT / 'config')+'/')
        config = config.replace('splits: config/', 'splits: '+str(ROOT / 'config')+'/')
        (BUILD / 'config.yml').write_text(config)
        run(['build/tools/dtk', 'dol', 'split', '--no-update', '-j', '2', str(BUILD/'config.yml'),
             str(BUILD/'retail')], 'dtk')
        retail = BUILD / 'retail/obj'
    symbols = {}
    for match in re.finditer(r'^(\S+) = \.text:(0x[0-9A-Fa-f]+); // type:function size:(0x[0-9A-Fa-f]+)',
                             (ROOT/'config/RMGK01/symbols.txt').read_text(), re.M):
        symbols[match[1]] = (int(match[2], 16), int(match[3], 16))
    evidence = {'dol_sha1': hashlib.sha1(dol).hexdigest(), 'compiler': 'GC/3.0a3; actual configured SDK flags (Game flags for ModelX); no overlays',
                'scope': 'Original imported methods and recovered inline calcNrmMtx; percentages are measured, not assertions of full equivalence.',
                'source_sha256': {}, 'headers_sha256': {}, 'functions': [], 'inline_calcNrmMtx': {}}
    commands = {}
    for unit, source in UNITS.items():
        target = retail / source[4:].replace('.cpp', '.o')
        obj = BUILD / (unit+'-original.o')
        commands[unit] = compiler(unit) + ['-c', source, '-o', str(obj)]
        evidence['source_sha256'][source] = sha(ROOT/source)
        run(commands[unit], unit+'-original')
        assert evidence['source_sha256'][source] == sha(ROOT/source)
        run(['build/tools/objdiff-cli', 'diff', '-1', str(target), '-2', str(obj),
             '-o', str(BUILD/(unit+'.objdiff.json')), '--format', 'json-pretty'], unit+'-objdiff')
        diff = json.loads((BUILD/(unit+'.objdiff.json')).read_text())
        right = {s['name']: s for s in diff['right']['symbols']}
        for original in diff['left']['symbols']:
            name = original['name']
            if name not in symbols or not selected(unit, name) or name not in right: continue
            address, size = symbols[name]
            evidence['functions'].append({'unit': unit, 'name': name, 'address': hex(address),
                'retail_size': size, 'compiled_size': int(right[name]['size']),
                'objdiff_match_percent': original.get('match_percent'),
                'retail_sha256': hashlib.sha256(dol_bytes(dol, address, size)).hexdigest()})
        if unit == 'J3DModel':
            for side, path in [('retail', target), ('compiled', obj)]:
                code, refs = Elf(path).code_and_refs('viewCalc__8J3DModelFv')
                calls = [ref for ref in refs if ref['symbol'] == 'calcNrmMtx__12J3DMtxBufferFv']
                assert len(calls) == 2 and all(ref['kind'] == 10 for ref in calls)
                # Both original inlined calls first load the actual buffer from
                # mMtxBuffer at the Wii model offset0x84 in retained this=r31.
                assert all(struct.unpack_from('>I', code, ref['offset']-4)[0] == 0x807F0084 for ref in calls)
                evidence['inline_calcNrmMtx'][side] = calls
    for source in ['libs/JSystem/include/JSystem/J3DGraphAnimator/J3DModel.hpp',
                   'libs/JSystem/include/JSystem/J3DGraphBase/J3DMaterial.hpp']:
        evidence['headers_sha256'][source] = sha(ROOT/source)
    (BUILD/'compiler-evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
    (BUILD/'original-commands.json').write_text(json.dumps(commands, indent=2)+'\n')
    for item in evidence['functions']:
        print(f"{item['name']}: {item['objdiff_match_percent']}% ({item['compiled_size']}/{item['retail_size']} bytes)")
    print('Both recovered inline calcNrmMtx loads and call targets agree with retail.')


if __name__ == '__main__': main()
