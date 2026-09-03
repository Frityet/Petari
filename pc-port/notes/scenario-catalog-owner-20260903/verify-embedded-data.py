#!/usr/bin/env python3
"""Stage exact embedded data and prove declaration-only source adaptations.

Writes ignored staged-root files, never production root/native sources.
"""
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
STAGED = BUILD / 'staged-root'
BUILD.mkdir(parents=True, exist_ok=True)

def module(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

helpers = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
dol = helpers.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
data = helpers.dol_bytes(dol, 0x8053DE00, 0xD20)
assert hashlib.sha256(data).hexdigest() == '44b3ace292ed8f9f7c60f0ddd49a89cdd0648c67613b5974e85658bfd18a2d43'
lines = ['#include <revolution/types.h>', '',
         'extern const u8 GalaxyIDBCSV[0xD20] ATTRIBUTE_ALIGN(4) = {']
lines += ['    ' + ', '.join('0x%02X' % byte for byte in data[i:i + 16]) + ','
          for i in range(0, len(data), 16)]
lines += ['};', '']
resource_path = Path('src/Game/System/GalaxyIDBCSV.cpp')
destination = STAGED / resource_path
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text('\n'.join(lines))

def compile_source(source, output):
    command = helpers.compiler('cflags_game')
    if source.is_relative_to(STAGED):
        command[3:3] = ['-i', str(STAGED / 'include')]
    command += ['-c', str(source), '-o', str(output)]
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    output.with_suffix('.log').write_text(result.stdout)
    result.check_returncode()
    return helpers.Elf(output)

def object_image(elf, symbol):
    _, start, size, index = next(item for item in elf.symbols if item[0] == symbol)
    code = elf.section_data(index)[start:start + size]
    relocations = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != index:
            continue
        for at in range(section[4], section[4] + section[5], section[9]):
            offset, info, addend = struct.unpack_from('>IIi', elf.data, at)
            if start <= offset < start + size:
                relocations.append((offset - start, info & 255, elf.symbols[info >> 8][0], addend))
    return code, relocations

obj = compile_source(destination, BUILD / 'GalaxyIDBCSV.o')
compiled_data, relocations = object_image(obj, 'GalaxyIDBCSV')
assert compiled_data == data and not relocations
records = []
patches = ['diff --git a/' + str(resource_path) + ' b/' + str(resource_path) + '\nnew file mode 100644\n' +
           ''.join(difflib.unified_diff([], destination.read_text().splitlines(True),
                                       '/dev/null', 'b/' + str(resource_path)))]
for name, symbol in [
    ('GalaxyNameSortTable', 'getGalaxySortIndex__19GalaxyNameSortTableFPCc'),
    ('GameEventFlagTable', 'getGalaxyDependedFlags__18GameEventFlagTableFPPCciPCc'),
]:
    relative = Path('src/Game/System') / (name + '.cpp')
    original = (ROOT / relative).read_text()
    assert original.count('extern const JMapData GalaxyIDBCSV;') == 1
    replacement = original.replace('extern const JMapData GalaxyIDBCSV;', 'extern const u8 GalaxyIDBCSV[];')
    destination = STAGED / relative
    destination.write_text(replacement)
    before = object_image(compile_source(ROOT / relative, BUILD / (name + '.before.o')), symbol)
    after = object_image(compile_source(destination, BUILD / (name + '.after.o')), symbol)
    assert before == after, symbol
    records.append(dict(symbol=symbol,unchanged_bytes=len(before[0]),unchanged_relocations=len(before[1])))
    patches.append('diff --git a/' + str(relative) + ' b/' + str(relative) + '\n' +
                   ''.join(difflib.unified_diff(original.splitlines(True), replacement.splitlines(True),
                                               'a/' + str(relative), 'b/' + str(relative))))
    print(symbol, len(before[0]), 'bytes and relocations unchanged')
header = Path('include/Game/System/GameDataConst.hpp')
original_header = (ROOT / header).read_text()
changed_header = original_header.replace('extern const JMapData GalaxyIDBCSV;', 'extern const u8 GalaxyIDBCSV[];')
assert original_header != changed_header
(STAGED / header).parent.mkdir(parents=True, exist_ok=True)
(STAGED / header).write_text(changed_header)
patches.append('diff --git a/' + str(header) + ' b/' + str(header) + '\n' +
               ''.join(difflib.unified_diff(original_header.splitlines(True), changed_header.splitlines(True),
                                           'a/' + str(header), 'b/' + str(header))))
relative = Path('src/Game/System/GameDataConst.cpp')
(STAGED / relative).write_bytes((ROOT / relative).read_bytes())
before = compile_source(ROOT / relative, BUILD / 'GameDataConst.before.o')
after = compile_source(STAGED / relative, BUILD / 'GameDataConst.after.o')
for symbol in ['getPowerStarNumToOpenGalaxy__13GameDataConstFPCc', 'getIncludedGrandGalaxyId__13GameDataConstFPCc']:
    a, b = object_image(before, symbol), object_image(after, symbol)
    assert a == b
    records.append(dict(symbol=symbol,unchanged_bytes=len(a[0]),unchanged_relocations=len(a[1])))
    print(symbol, len(a[0]), 'bytes and relocations unchanged')
(NOTE / 'embedded-data-root.patch').write_text(''.join(patches))
(NOTE / 'embedded-data-evidence.json').write_text(json.dumps(dict(
    retail_address='0x8053DE00',byte_count=len(data),sha256=hashlib.sha256(data).hexdigest(),
    compiled_bytes_equal_retail=True,header=struct.unpack_from('>4I',data),functions=records),indent=2) + '\n')
print('Exact 0xD20-byte resource compiled; production sources unchanged.')
