#!/usr/bin/env python3
"""Verify recovered reset against all five RMGK01 instructions and its SDA load."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-model-manager-render-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
SOURCE = ROOT / 'src/Game/Effect/SyncBckEffectChecker.cpp'


def dol_bytes(data, address, size):
    for i in range(18):
        offset = struct.unpack_from('>I', data, i * 4)[0]
        base = struct.unpack_from('>I', data, 0x48 + i * 4)[0]
        length = struct.unpack_from('>I', data, 0x90 + i * 4)[0]
        if length and base <= address and address + size <= base + length:
            return data[offset + address - base:offset + address - base + size]
    raise AssertionError(hex(address))


def elf_symbol_bytes(path, wanted):
    data = path.read_bytes()
    offset = struct.unpack_from('>I', data, 0x20)[0]
    size, count = struct.unpack_from('>HH', data, 0x2e)
    sections = [struct.unpack_from('>10I', data, offset + i * size) for i in range(count)]
    symtab = next(s for s in sections if s[1] == 2)
    strings = sections[symtab[6]]
    names = data[strings[4]:strings[4] + strings[5]]
    for offset in range(symtab[4], symtab[4] + symtab[5], symtab[9]):
        name, at, length, info, _, section = struct.unpack_from('>IIIBBH', data, offset)
        if names[name:names.index(0, name)].decode() == wanted:
            contents = sections[section]
            return data[contents[4] + at:contents[4] + at + length]
    raise AssertionError(wanted)


def main():
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    # Establish r2 from the actual startup register initializer, independently
    # of the reset's relocation and the checked-in constant address.
    words = struct.unpack('>36I', dol_bytes(dol, 0x800041a8, 0x90))
    index = next(i for i, word in enumerate(words) if word & 0xffff0000 == 0x3c400000)
    assert words[index + 1] & 0xffff0000 == 0x60420000
    sda2 = ((words[index] & 65535) << 16) | (words[index + 1] & 65535)
    assert sda2 == 0x806bfc20
    command = json.loads((HERE / 'width-fix-evidence.json').read_text())['commands']['after']
    command[-3] = str(SOURCE)
    output = BUILD / 'SyncBckEffectChecker.ppc.o'
    command[-1] = str(output)
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (HERE / 'effect-reset.compile.log').write_text(result.stdout)
    result.check_returncode()
    spec = importlib.util.spec_from_file_location('width', HERE / 'verify-width-fixes.py')
    width = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(width)
    function = width.elf_functions(output)['reset__20SyncBckEffectCheckerFv']
    assert function['size'] == 20
    assert len(function['relocations']) == 1
    relocation = function['relocations'][0]
    assert relocation['offset'] == 0 and relocation['kind'] == 109 and relocation['addend'] == 0
    compiled_constant = elf_symbol_bytes(output, relocation['symbol'])
    assert compiled_constant == bytes(4)
    retail = dol_bytes(dol, 0x800cb748, 20)
    instruction = struct.unpack_from('>I', retail)[0]
    assert instruction & 0xffff0000 == 0xc0020000  # lfs f0, SDA(r2)
    constant_address = sda2 + struct.unpack('>h', struct.pack('>H', instruction & 65535))[0]
    assert dol_bytes(dol, constant_address, 4) == compiled_constant
    relocated = bytearray.fromhex(function['bytes'])
    value = struct.unpack_from('>I', relocated)[0]
    # R_PPC_EMB_SDA21 sets the actual r2 base and signed offset.
    value = (value & ~0x1fffff) | (2 << 16) | ((constant_address - sda2) & 65535)
    struct.pack_into('>I', relocated, 0, value)
    assert relocated == retail
    report = {'dol_sha1': hashlib.sha1(dol).hexdigest(),
              'source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              'address': '0x800cb748', 'size': 20, 'instructions_equal': 5,
              'r2_initializer_address': hex(0x800041a8 + index * 4),
              'r2_initializer_words': [hex(words[index]), hex(words[index + 1])],
              'sda2': hex(sda2), 'constant_address': hex(constant_address),
              'constant_hex': compiled_constant.hex(), 'relocated_hex': relocated.hex(),
              'unrelocated': function, 'command': command}
    (HERE / 'effect-reset-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    (HERE / 'effect-reset.asm').write_text(
        '800cb748 c002a168  lfs f0,-24216(r2) ; r2=806bfc20 -> 806b9d88 = +0.0f\n'
        '800cb74c 38000001  li r0,1\n'
        '800cb750 98030008  stb r0,8(r3)\n'
        '800cb754 d0030004  stfs f0,4(r3)\n'
        '800cb758 4e800020  blr\n')
    print('PASS: all 5 reset instructions and the actual SDA constant match retail')


if __name__ == '__main__':
    main()
