#!/usr/bin/env python3
"""Original compiler check; BAS methods compared against every retail byte."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-mario-sound-20260903'
EXACT = ['startBas__5MarioFPCcbff', 'isRunningBas__5MarioCFPCc', 'skipBas__5MarioFf']

def main():
    spec = importlib.util.spec_from_file_location('helper', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    output = BUILD / 'MarioSound-retail.o'
    command = helper.compiler('cflags_game') + ['-c', 'src/Game/Player/MarioSound.cpp', '-o', str(output)]
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / 'retail.log').write_text(result.stdout)
    result.check_returncode()
    target = ROOT / 'build/mario-update-restoration-20260903/retail/obj/Game/Player/MarioSound.o'
    subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(target), '-2', str(output), '-o', str(BUILD / 'MarioSound.diff.json'), '--format', 'json-pretty'], check=True)
    diff = json.loads((BUILD / 'MarioSound.diff.json').read_text())
    compiled, original = helper.Elf(output), helper.Elf(target)
    dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    addresses = {name: int(address, 16) for name, address in re.findall(r'^(\S+) = \.\w+:0x([0-9A-Fa-f]+);', (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
    evidence = {'command': command, 'source_sha256': hashlib.sha256((ROOT / 'src/Game/Player/MarioSound.cpp').read_bytes()).hexdigest(), 'dol_sha1': hashlib.sha1(dol).hexdigest(), 'exact': [], 'fuzzy_only': []}
    for name in EXACT:
        normalized, refs = compiled.function(name)
        assert (normalized, refs) == original.function(name)
        address = addresses[name]
        code = bytearray(normalized)
        for ref in refs:
            assert ref['kind'] == 10
            offset = ref['offset']
            displacement = addresses[ref['symbol']] + ref['addend'] - (address + offset)
            word = struct.unpack_from('>I', code, offset)[0]
            struct.pack_into('>I', code, offset, word | (displacement & 0x03FFFFFC))
        assert code == helper.dol_bytes(dol, address, len(code))
        evidence['exact'].append({'name': name, 'address': hex(address), 'bytes': len(code), 'fully_relocated_bytes_equal': True, 'relocations': refs})
        print(name, len(code) // 4, 'fully relocated instructions equal')
    for name in ['initSoundTable__5MarioFP9SoundListUl', 'playSoundJ__5MarioFPCcl', 'stopSoundJ__5MarioFPCcUl']:
        symbol = next(s for s in diff['left']['symbols'] if s['name'] == name)
        evidence['fuzzy_only'].append({'name': name, 'match_percent': symbol['match_percent'], 'bytes': symbol['size']})
    (NOTES / 'retail-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')

if __name__ == '__main__':
    main()
