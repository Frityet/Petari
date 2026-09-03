#!/usr/bin/env python3
"""Original-compiler verification of the three recovered scale-blend methods."""
import importlib.util
import json
from pathlib import Path
import re
import struct

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('entrypoints', HERE / 'entrypoints-verify-object.py')
entrypoints = importlib.util.module_from_spec(spec)
spec.loader.exec_module(entrypoints)
ROOT = entrypoints.ROOT
BUILD = ROOT / 'build/xanime-core-matrix-calculation-20260903/scale'
FUNCTIONS = (('calcScaleBlendMaya', 0x80019F74, 0x548), ('calcScaleBlendSI', 0x8001A688, 0x4D0),
             ('calcScaleBlendSpecial', 0x8001AC5C, 0x98))
SDA = {2: 0x806BFC20, 13: 0x806B9620}


def verify_relocations(elf, dol):
    """Relocate the original-compiler instructions at their real retail addresses.

    The float-address lookup is read from the retail SDA instruction, followed by
    a comparison to the actual compiled constant bytes. Other addresses come from
    the supplied symbol map. No symbol-name-only constant acceptance is used.
    """
    symbols = {name: int(address, 16) for name, address in re.findall(
        r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);', (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
    records = []
    for short, address, size in FUNCTIONS:
        name, start, compiled_size, section = next(s for s in elf.symbols if s[0].startswith(short+'__10XanimeCore'))
        assert compiled_size == size
        code = bytearray(elf.section_data(section)[start:start+size])
        retail = entrypoints.helpers.reader.dol_bytes(dol, address, size)
        refs = elf.references(name)
        resolved = []
        for ref in refs:
            offset, kind = int(ref['offset'], 16), ref['kind']
            if 'value_hex' in ref:
                assert kind == 109
                word = struct.unpack_from('>I', retail, offset)[0]
                base = (word >> 16) & 31
                displacement = struct.unpack('>h', struct.pack('>H', word & 65535))[0]
                target = SDA[base] + displacement
                expected = bytes.fromhex(ref['value_hex'])
                assert entrypoints.helpers.reader.dol_bytes(dol, target, len(expected)) == expected
            else:
                target = symbols[ref['symbol']] + ref['addend']
            if kind == 10:  # R_PPC_REL24
                word = struct.unpack_from('>I', code, offset)[0]
                struct.pack_into('>I', code, offset, (word & 0xFC000003) | ((target-address-offset) & 0x3FFFFFC))
            elif kind in (4, 6):  # R_PPC_ADDR16_LO / HA
                value = target & 65535 if kind == 4 else ((target+0x8000) >> 16) & 65535
                struct.pack_into('>H', code, offset, value)
            elif kind == 109:  # R_PPC_EMB_SDA21
                word = struct.unpack_from('>I', code, offset)[0]
                base = 2 if 'value_hex' in ref else 13
                displacement = target - SDA[base]
                assert -32768 <= displacement < 32768
                struct.pack_into('>I', code, offset, (word & ~0x1FFFFF) | (base << 16) | (displacement & 65535))
            else:
                raise AssertionError(ref)
            resolved.append({'offset': ref['offset'], 'kind': kind, 'symbol': ref['symbol'],
                             'effective_retail_target': hex(target), **({'constant_bits': ref['value_hex']} if 'value_hex' in ref else {})})
        assert code == retail, short+' has a nonrelocation instruction difference'
        records.append({'method': short, 'instruction_count': size//4, 'relocations_checked': len(refs),
                        'every_relocated_instruction_equal_to_retail': True, 'resolved_references': resolved})
    return records


def main():
    # Reuse the established original-compiler/DTK/objdiff reader, but direct all
    # artifacts into this verifier's separate build folder and compare our methods.
    entrypoints.BUILD = BUILD
    entrypoints.FUNCTIONS = FUNCTIONS
    entrypoints.main()
    evidence_path = BUILD / 'entrypoints-compiler-evidence.json'
    evidence = json.loads(evidence_path.read_text())
    evidence['scope'] = 'Three root XanimeCore scale methods; original compiler, actual retail relocation verification, and bounded retail instruction interpreter. No PC activation/native build.'
    elf = entrypoints.helpers.reader.Elf(BUILD / 'XanimeCore.o')
    dol = entrypoints.DOL.read_bytes()
    evidence['verified_relocations'] = verify_relocations(elf, dol)
    spec = importlib.util.spec_from_file_location('scale_behavior', HERE / 'scale-behavior.py')
    behavior = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(behavior)
    evidence['behavior_cases'] = behavior.verify(dol, entrypoints.helpers.reader.dol_bytes)
    out = BUILD / 'scale-evidence.json'
    out.write_text(json.dumps(evidence, indent=2)+'\n')
    print(f"All {sum(x['instruction_count'] for x in evidence['verified_relocations'])} relocated instructions match retail; {len(evidence['behavior_cases'])} numeric state cases passed. Evidence: {out}")


if __name__ == '__main__':
    main()
