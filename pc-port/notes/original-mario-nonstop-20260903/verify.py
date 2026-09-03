#!/usr/bin/env python3
"""Recover the original non-stop wrapper, with a separate MarioWait build gate."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-mario-nonstop-20260903'
METHODS = [('changeAnimationNonStop__11MarioModuleFPCc', 0x802E8F84, 0x6C),
           ('changeAnimationWithAttr__11MarioModuleFPCcUl', 0x802E8FF0, 0x68)]

def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location('helper', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    records = []
    for name in ['MarioModule', 'MarioWait']:
        source = ROOT / ('src/Game/Player/' + name + '.cpp')
        command = helper.compiler('cflags_game') + ['-c', str(source.relative_to(ROOT)), '-o', str(BUILD / (name + '.o'))]
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / (name + '.compile.log')).write_text(result.stdout)
        records.append({'source': str(source.relative_to(ROOT)), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'command': command, 'returncode': result.returncode})
        print(name, 'original compiler:', result.returncode)
        result.check_returncode()
    compiled = helper.Elf(BUILD / 'MarioModule.o')
    original = helper.Elf(ROOT / 'build/mario-update-restoration-20260903/retail/obj/Game/Player/MarioModule.o')
    addresses = {name: int(address, 16) for name, address in re.findall(r'^(\S+) = \.\w+:0x([0-9A-Fa-f]+);', (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
    dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    methods = []
    for name, address, size in METHODS:
        normalized, refs = compiled.function(name)
        assert (normalized, refs) == original.function(name)
        assert len(normalized) == size
        code = bytearray(normalized)
        for ref in refs:
            assert ref['kind'] == 10
            offset = ref['offset']
            displacement = addresses[ref['symbol']] + ref['addend'] - (address + offset)
            word = struct.unpack_from('>I', code, offset)[0]
            struct.pack_into('>I', code, offset, word | (displacement & 0x03FFFFFC))
        assert code == helper.dol_bytes(dol, address, size)
        methods.append({'function': name, 'address': hex(address), 'bytes': size, 'fully_relocated_bytes_equal': True, 'relocations': refs})
    stage = BUILD / 'staged'
    stage.mkdir(exist_ok=True)
    source = (ROOT / 'src/Game/Player/MarioModule.cpp').read_text()
    start = source.index('void MarioModule::changeAnimationNonStop(')
    next_start = source.index('void MarioModule::changeAnimationWithAttr(', start)
    end = source.index('\n}', next_start) + 2
    body = source[start:end]
    native = '#include "Game/Player/MarioModule.hpp"\n#include "Game/Player/MarioActor.hpp"\n#include "Game/Player/MarioAnimator.hpp"\n\n' + body + '\n'
    path = stage / 'OriginalMarioModuleAnimationFlags.cpp'
    path.write_text(native)
    (NOTES / 'OriginalMarioModuleAnimationFlags.cpp').write_text(native)
    entry = next(e for e in json.loads((ROOT / 'pc-port/compile_commands.json').read_text()) if e['file'].endswith('/MarioMove.cpp'))
    command = []
    skip = False
    for arg in entry['arguments']:
        if skip: skip = False
        elif arg == '-o': skip = True
        elif arg not in ('-c', entry['file']): command.append(arg)
    command[1:1] = ['-I' + str(ROOT / 'pc-port/src'), '-I' + str(ROOT / 'pc-port/aurora/include')]
    command += ['-c', str(path), '-o', str(BUILD / 'OriginalMarioModuleAnimationFlags.o')]
    result = subprocess.run(command, cwd=entry['directory'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / 'native.compile.log').write_text(result.stdout)
    result.check_returncode()
    evidence = {'methods': methods,
                'dol_sha1': hashlib.sha1(dol).hexdigest(), 'body_sha256': hashlib.sha256(body.encode()).hexdigest(),
                'retail_compiles': records, 'native_command': command, 'native_returncode': result.returncode,
                'header_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT / 'include/Game/Player/Mario.hpp', ROOT / 'include/Game/Player/MarioModule.hpp']}}
    (NOTES / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print('Both animation wrappers: all 53 fully relocated instructions equal retail; literal native methods compile')

if __name__ == '__main__':
    main()
