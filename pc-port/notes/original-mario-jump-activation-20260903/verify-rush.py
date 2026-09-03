#!/usr/bin/env python3
"""Compile the root rush gate and resolve every relocation against retail."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import struct
import subprocess

NOTES = Path(__file__).resolve().parent
ROOT = NOTES.parents[2]
BUILD = ROOT / 'build/original-mario-jump-activation-20260903'
SOURCE = ROOT / 'src/Game/Player/MarioActorRushMsg.cpp'
NAME = 'tryStandardRush__10MarioActorFv'
ADDRESS = 0x802C88B0
SIZE = 0x104


def main():
    spec = importlib.util.spec_from_file_location('retail_helper', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    BUILD.mkdir(parents=True, exist_ok=True)
    output = BUILD / 'MarioActorRushMsg-retail.o'
    command = helper.compiler('cflags_game') + ['-c', str(SOURCE.relative_to(ROOT)), '-o', str(output)]
    source_sha = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / 'MarioActorRushMsg-retail.compile.log').write_text(result.stdout)
    (BUILD / 'MarioActorRushMsg-retail.command.json').write_text(json.dumps(command, indent=2) + '\n')
    result.check_returncode()
    assert source_sha == hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    addresses = {name: int(address, 16) for name, address in re.findall(r'^(\S+) = \.\w+:0x([0-9A-Fa-f]+);', (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
    obj = helper.Elf(output)
    _, start, size, index = next(s for s in obj.symbols if s[0] == NAME)
    assert size == SIZE
    code = bytearray(obj.section_data(index)[start:start + size])
    refs = []
    for section in obj.sections:
        if section[1] != 4 or section[7] != index:
            continue
        for cursor in range(section[4], section[4] + section[5], section[9]):
            at, info, addend = struct.unpack_from('>IIi', obj.data, cursor)
            if not start <= at < start + size:
                continue
            target, target_offset, _, target_section = obj.symbols[info >> 8]
            kind = info & 255
            offset = at - start
            if kind in (4, 6):
                # The only local data relocation names a real body sensor string.
                assert obj.section_data(target_section)[target_offset:target_offset + 5] == b'body\0'
                target_address = 0x805BAA10
                assert helper.dol_bytes(dol, target_address, 5) == b'body\0'
            else:
                target_address = addresses[target]
            value = target_address + addend
            if kind == 10:  # R_PPC_REL24: preserve opcode, absolute bit and link bit.
                word = struct.unpack_from('>I', code, offset)[0]
                displacement = value - (ADDRESS + offset)
                assert displacement % 4 == 0 and -(1 << 25) <= displacement < (1 << 25)
                struct.pack_into('>I', code, offset, (word & 0xFC000003) | (displacement & 0x03FFFFFC))
            elif kind == 6:  # R_PPC_ADDR16_HA
                struct.pack_into('>H', code, offset, ((value + 0x8000) >> 16) & 0xFFFF)
            elif kind == 4:  # R_PPC_ADDR16_LO
                struct.pack_into('>H', code, offset, value & 0xFFFF)
            else:
                raise AssertionError((target, kind))
            refs.append({'offset': offset, 'kind': kind, 'symbol': target, 'target_address': hex(target_address), 'addend': addend})
    retail = helper.dol_bytes(dol, ADDRESS, SIZE)
    assert code == retail
    evidence = {'function': NAME, 'source': str(SOURCE.relative_to(ROOT)), 'source_sha256': source_sha,
                'compiler': 'GC/3.0a3 with configure.py cflags_game', 'address': hex(ADDRESS), 'size': SIZE,
                'instructions': SIZE // 4, 'fully_relocated_bytes_equal': True,
                'dol_sha1': hashlib.sha1(dol).hexdigest(), 'function_sha256': hashlib.sha256(retail).hexdigest(),
                'relocations': refs, 'command': command}
    (NOTES / 'rush-retail-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print(f'{NAME}: all {SIZE // 4} relocated instructions exactly equal supplied retail DOL')


if __name__ == '__main__':
    main()
