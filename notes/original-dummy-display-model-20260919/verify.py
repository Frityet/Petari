#!/usr/bin/env python3
"""Compile the recovered donor and verify its code/table against the local retail DOL."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NOTE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-dummy-display-model-20260919'
DECOMP = ROOT / 'decomp'
RETAIL = DECOMP / 'build/original-resource-holder-20260903/retail/obj/Game/MapObj/DummyDisplayModel.o'
ASM = DECOMP / 'build/original-resource-holder-20260903/retail/asm/Game/MapObj/DummyDisplayModel.s'
SOURCE = DECOMP / 'src/Game/MapObj/DummyDisplayModel.cpp'
HEADER = DECOMP / 'include/Game/MapObj/DummyDisplayModel.hpp'
TABLE = 'cDummyDisplayModelInfoTable__31@unnamed@DummyDisplayModel_cpp@'
CREATE = 'tryCreateDummyModel__31@unnamed@DummyDisplayModel_cpp@FP9LiveActorRC12JMapInfoIterli'
helper = runpy.run_path(str(ROOT / 'notes/mario-update-restoration-20260903/verify-object.py'))
Elf, dol_bytes = helper['Elf'], helper['dol_bytes']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def object_table(elf):
    _, start, size, section = next(s for s in elf.symbols if s[0] == TABLE)
    assert size == 420
    data = bytearray(elf.section_data(section)[start:start + size])
    strings = {}
    for ref in elf.references(TABLE):
        assert ref['kind'] == 1
        offset = int(ref['offset'], 0)
        assert offset % 28 in (0, 20)
        strings[offset] = bytes.fromhex(ref['value_hex']).removesuffix(b'\0').decode('ascii')
        data[offset:offset + 4] = b'\0' * 4
    return bytes(data), strings


def main():
    BUILD.mkdir(exist_ok=True)
    command_record = json.loads((NOTE / 'candidate-command.json').read_text())
    command = command_record['command']
    result = subprocess.run(command, cwd=DECOMP, capture_output=True, text=True)
    (NOTE / 'candidate-compile.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    candidate = BUILD / 'DummyDisplayModel-wii.o'
    subprocess.run([str(DECOMP / 'build/tools/objdiff-cli'), 'diff', '-1', str(RETAIL),
                    '-2', str(candidate), '-o', str(BUILD / 'diff-wii.json'), '--format', 'json-pretty'], check=True)
    diff = json.loads((BUILD / 'diff-wii.json').read_text())
    original, recovered = Elf(RETAIL), Elf(candidate)
    dol = (DECOMP / 'orig/RMGK01/sys/main.dol').read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    instruction_count = 0
    for address, hex_bytes in re.findall(r'/\* ([0-9A-F]{8}) [0-9A-F]{8}  ([0-9A-F ]{11}) \*/', ASM.read_text()):
        assert dol_bytes(dol, int(address, 16), 4) == bytes.fromhex(hex_bytes)
        instruction_count += 1
    assert instruction_count == 0x940 // 4
    original_data, original_strings = object_table(original)
    recovered_data, recovered_strings = object_table(recovered)
    assert (original_data, original_strings) == (recovered_data, recovered_strings)
    dol_table = bytearray(dol_bytes(dol, 0x80533150, 420))
    dol_strings = {}
    for offset, expected in original_strings.items():
        address = struct.unpack_from('>I', dol_table, offset)[0]
        actual = dol_bytes(dol, address, len(expected) + 1)
        assert actual == expected.encode('ascii') + b'\0'
        dol_strings[offset] = actual[:-1].decode('ascii')
        dol_table[offset:offset + 4] = b'\0' * 4
    assert (bytes(dol_table), dol_strings) == (recovered_data, recovered_strings)
    assert not any(s[0].startswith('__sinit') for s in recovered.symbols)
    # The remaining six creation-function differences consistently exchange the
    # model-ID and table-row registers. No instruction, operand, or branch differs.
    sides = [next(s for s in diff[k]['symbols'] if s['name'] == CREATE) for k in ('left', 'right')]
    instructions = [[i['instruction']['formatted'] for i in side['instructions']] for side in sides]
    swapped = [re.sub(r'\br(29|30)\b', lambda m: 'r30' if m[1] == '29' else 'r29', i) for i in instructions[1]]
    assert instructions[0] == swapped
    references = [elf.references(CREATE) for elf in (original, recovered)]
    assert references[0] == references[1]
    functions = []
    baseline_path = BUILD / 'diff-before.json'
    baseline = json.loads(baseline_path.read_text()) if baseline_path.exists() else None
    for symbol in diff['left']['symbols']:
        if not symbol.get('instructions'):
            continue
        assert symbol['match_percent'] >= 99.0
        row = {'name': symbol['name'], 'retail_size': int(symbol['size']), 'match_percent': symbol['match_percent']}
        if baseline:
            old = next(s for s in baseline['left']['symbols'] if s['name'] == symbol['name'])
            row['baseline_match_percent'] = old.get('match_percent')
        functions.append(row)
    rows = []
    for idx in range(15):
        at = idx * 28
        rows.append({'id': idx, 'model': recovered_strings[at],
                     'offset': struct.unpack_from('>3f', recovered_data, at + 4),
                     'default_draw_category': struct.unpack_from('>I', recovered_data, at + 16)[0],
                     'animation': recovered_strings.get(at + 20), 'color_change': bool(recovered_data[at + 24])})
    report = {'scope': 'Retail donor code/table proof only; no native import, actor runtime or cage progression claimed.',
              'dol_sha1': hashlib.sha1(dol).hexdigest(), 'source_sha256': digest(SOURCE), 'header_sha256': digest(HEADER),
              'retail_object_sha256': digest(RETAIL), 'candidate_object_sha256': digest(candidate),
              'retail_asm_instructions_verified_against_dol': instruction_count,
              'table_address': '0x80533150', 'table_rows': 15, 'table_bytes': 420,
              'relocation_normalized_table_sha256': hashlib.sha256(recovered_data).hexdigest(),
              'table_string_pointer_count': len(recovered_strings), 'table_exact_including_float_and_padding_bytes': True,
              'added_dynamic_initializers': 0, 'creation_function_address': '0x801D085C',
              'creation_function_exact_after_register_29_30_swap': True,
              'creation_function_references': references[1], 'rows': rows, 'functions': functions}
    (NOTE / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: {instruction_count} retail instructions verified against DOL; 15/15 table rows exact; no dynamic initializer;')
    print(f'creation {sides[0]["match_percent"]}% (244 bytes, only consistent r29/r30 exchange); {len(functions)} functions >=99%.')


if __name__ == '__main__':
    main()
