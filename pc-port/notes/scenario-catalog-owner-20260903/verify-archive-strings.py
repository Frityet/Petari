#!/usr/bin/env python3
"""Prove staged source corrections against retail, without production edits."""
from pathlib import Path
import difflib
import hashlib
import importlib.util
import json
import struct
import subprocess

NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
BUILD = ROOT / 'build/scenario-catalog-owner-20260903'
spec = importlib.util.spec_from_file_location('helpers', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
DOL = h.DOL.read_bytes()
assert hashlib.sha1(DOL).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
SOURCE = Path('src/Game/Util/StringUtil.cpp')
staged = BUILD / 'staged-root' / SOURCE
original = (ROOT / SOURCE).read_text()
changed = original.replace('#include <cstdio>', '#include <cstdio>\n#include <stdint.h>')
if '#include <stdint.h>' not in changed:
    index = changed.index('\n')
    changed = changed[:index] + '\n#include <stdint.h>' + changed[index:]
old = 'pExtSeparator < pDirSeparator || pDirSeparator + 1 == pExtSeparator'
new = ('reinterpret_cast<uintptr_t>(pExtSeparator) < reinterpret_cast<uintptr_t>(pDirSeparator) ||\n'
       '            reinterpret_cast<uintptr_t>(pDirSeparator) + 1 == reinterpret_cast<uintptr_t>(pExtSeparator)')
assert changed.count(old) == 1
changed = changed.replace(old, new)
begin = changed.index('    const char* getBasename(')
end = changed.index('\n    void extractString', begin)
body = changed[begin:end].replace('return pBasename;', 'return pPath;')
assert body != changed[begin:end]
changed = changed[:begin] + body + changed[end:]
changed = changed.replace('    // FIXME: Missing stack accesses.\n', '')
staged.parent.mkdir(parents=True, exist_ok=True)
staged.write_text(changed)
commands = []
for name, source in [('before', ROOT / SOURCE), ('after', staged)]:
    command = h.compiler('cflags_game') + ['-c', str(source), '-o', str(BUILD / ('StringUtil.' + name + '.o'))]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    (BUILD / ('StringUtil.' + name + '.log')).write_text(result.stdout + result.stderr)
    result.check_returncode()
    commands.append(command)
elf = h.Elf(BUILD / 'StringUtil.after.o')
addresses = {'strrchr': 0x8051FF4C, 'snprintf': 0x8051E4CC, 'strncpy': 0x8051FD50}

def linked_function(name, address):
    _, start, size, index = next(s for s in elf.symbols if s[0] == name)
    code = bytearray(elf.section_data(index)[start:start + size])
    refs = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != index:
            continue
        for at in range(section[4], section[4] + section[5], section[9]):
            offset, info, addend = struct.unpack_from('>IIi', elf.data, at)
            if not start <= offset < start + size:
                continue
            offset -= start
            symbol = elf.symbols[info >> 8]
            kind = info & 255
            word = struct.unpack_from('>I', code, offset)[0]
            if kind == 10:
                target = addresses[symbol[0]] + addend
                word = (word & 0xFC000003) | ((target - address - offset) & 0x03FFFFFC)
            elif kind == 109:
                assert addend == 0 and symbol[2] == 3
                assert elf.section_data(symbol[3])[symbol[1]:symbol[1] + symbol[2]] == b'%s\0'
                target = 0x806B26B0
                assert h.dol_bytes(DOL, target, 3) == b'%s\0'
                # SDA21 sets rA to r13; all other opcode/register bits retained.
                assert word & 0xFFFF0000 == 0x38A00000
                word = (word & 0xFFE00000) | (13 << 16) | ((target - 0x806B9620) & 0xFFFF)
            else:
                raise AssertionError(('unexpected relocation', kind, symbol))
            struct.pack_into('>I', code, offset, word)
            refs.append(dict(offset=offset, type=kind, target=hex(target), symbol=symbol[0]))
    return bytes(code), refs

def sx(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value

def basename_result(code, address, path, last_slash):
    """Execute the actual small PPC body; strrchr's result is the only oracle input."""
    registers = [0] * 32
    registers[1], registers[3] = 0x90001000, path
    lr, pc, equal, memory, calls = 0xDEADBEEF, address, False, {}, []
    for _ in range(40):
        word = struct.unpack_from('>I', code, pc - address)[0]
        op, rt, ra = word >> 26, (word >> 21) & 31, (word >> 16) & 31
        immediate, next_pc = sx(word & 0xFFFF, 16), pc + 4
        if word == 0x4E800020:
            assert lr == 0xDEADBEEF and registers[1] == 0x90001000 and calls == [(path, 47)]
            return registers[3]
        if word == 0x7C0802A6: registers[0] = lr
        elif word == 0x7C0803A6: lr = registers[0]
        elif op == 14: registers[rt] = ((registers[ra] if ra else 0) + immediate) & 0xFFFFFFFF
        elif op in (36, 37):
            location = (registers[ra] + immediate) & 0xFFFFFFFF
            memory[location] = registers[rt]
            if op == 37: registers[ra] = location
        elif op == 32: registers[rt] = memory[(registers[ra] + immediate) & 0xFFFFFFFF]
        elif op == 31 and ((word >> 1) & 1023) == 444:
            rb = (word >> 11) & 31
            registers[ra] = registers[rt] | registers[rb]
        elif op == 11:
            assert rt == 0
            equal = registers[ra] == (immediate & 0xFFFFFFFF)
        elif op == 16:
            assert ra == 2 and rt in (4, 12)
            if equal == (rt == 12): next_pc = pc + sx(word & 0xFFFC, 16)
        elif op == 18:
            target = pc + sx(word & 0x03FFFFFC, 26)
            if word & 1:
                assert target == addresses['strrchr']
                calls.append((registers[3], registers[4]))
                lr, registers[3] = pc + 4, last_slash
            else: next_pc = target
        else: raise AssertionError((hex(pc), hex(word)))
        pc = next_pc
    raise AssertionError('unexpected loop')

records = []
for name, address, size in [
    ('removeExtensionString__2MRFPcUlPCc', 0x803FE828, 124),
    ('copyString__2MRFPcPCcUl', 0x803FE918, 4),
    ('getBasename__2MRFPCc', 0x803FEA90, 68),
]:
    compiled, references = linked_function(name, address)
    retail = h.dol_bytes(DOL, address, size)
    assert len(compiled) == size
    exact = compiled == retail
    record = dict(symbol=name, address=hex(address), bytes=size,
                  relocated_bytes_exact=exact, references=references)
    if name.startswith('getBasename'):
        cases = 0
        for path in [0x1000, 0x80000000, 0xFFFFFF00]:
            for slash in [0] + [path + n for n in range(128)]:
                expected = path if slash == 0 else slash + 1
                assert basename_result(retail, address, path, slash) == expected
                assert basename_result(compiled, address, path, slash) == expected
                cases += 1
        record['actual_ppc_branch_return_cases'] = cases
    else: assert exact
    records.append(record)

retail_object = ROOT / 'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/Game/Util/StringUtil.o'
command = ['build/tools/objdiff-cli', 'diff', '-1', str(retail_object), '-2', str(BUILD / 'StringUtil.after.o'),
           '-o', str(BUILD / 'StringUtil.objdiff.json'), '--format', 'json-pretty']
subprocess.run(command, cwd=ROOT, check=True, capture_output=True)
diff = json.loads((BUILD / 'StringUtil.objdiff.json').read_text())
for record in records:
    record['objdiff_match_percent'] = next(s['match_percent'] for s in diff['left']['symbols'] if s['name'] == record['symbol'])
(NOTE / 'archive-string-root.patch').write_text('diff --git a/' + str(SOURCE) + ' b/' + str(SOURCE) + '\n' +
    ''.join(difflib.unified_diff(original.splitlines(True), changed.splitlines(True), 'a/' + str(SOURCE), 'b/' + str(SOURCE))))
(NOTE / 'archive-string-evidence.json').write_text(json.dumps(dict(
    dol_sha1=hashlib.sha1(DOL).hexdigest(), commands=commands, functions=records), indent=2) + '\n')
for record in records: print(record['symbol'], record['objdiff_match_percent'], 'percent', record.get('actual_ppc_branch_return_cases', 0), 'PPC cases')
