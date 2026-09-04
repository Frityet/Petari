#!/usr/bin/env python3
"""Check unchanged effect methods and existing reference-argument MSL clients."""
from pathlib import Path
import importlib.util, json, shutil, struct, subprocess
R = Path(__file__).resolve().parents[3]
N = Path(__file__).resolve().parent
B = R/'build/original-effect-keeper-completion-20260903'
spec = importlib.util.spec_from_file_location('compiler', R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
h = importlib.util.module_from_spec(spec); spec.loader.exec_module(h)
baseline = B/'baseline-msl'
shutil.copytree(R/'libs/MSL_C++/include', baseline, dirs_exist_ok=True)
shutil.copyfile(N/'baseline/functional.hpp', baseline/'functional.hpp')
flags = h.compiler('cflags_game')
old_flags = [str(baseline) if x == 'libs/MSL_C++/include' else x for x in flags]
def code(elf, row):
    name, start, size, index = row
    data = bytearray(elf.section_data(index)[start:start+size])
    for section in elf.sections:
        if section[1] != 4 or section[7] != index: continue
        for offset in range(section[4], section[4]+section[5], section[9]):
            at, info, addend = struct.unpack_from('>IIi', elf.data, offset)
            if not start <= at < start+size: continue
            at -= start; kind = info & 255
            if kind in (10, 109):
                word = struct.unpack_from('>I', data, at)[0]
                struct.pack_into('>I', data, at, word & (0xfc000003 if kind == 10 else 0xffe00000))
            elif kind in (4, 5, 6): data[at:at+2] = b'\0\0'
            else: raise AssertionError((name, kind))
    return data
rows = []
for source in ['src/Game/MapObj/ElectricBall.cpp', 'src/Game/Screen/GalaxyMap.cpp',
               'src/Game/AreaObj/AreaObj.cpp', 'src/Game/LiveActor/EffectKeeper.cpp']:
    pair = []
    for version, command in [('before', old_flags), ('after', flags)]:
        file = N/'baseline/EffectKeeper.cpp' if version == 'before' and source.endswith('/EffectKeeper.cpp') else R/source
        obj = B/(Path(source).stem+'-'+version+'.o')
        command = command + ['-c', str(file), '-o', str(obj)]
        run = subprocess.run(command, cwd=R, capture_output=True, text=True)
        obj.with_suffix('.log').write_text(run.stdout+run.stderr)
        if run.returncode: print(run.stdout, run.stderr); run.check_returncode()
        pair.append(h.Elf(obj))
    count = size = 0
    for symbol in pair[0].symbols:
        name, start, length, section = symbol
        if not length or not (pair[0].sections[section][2] & 4): continue
        other = next((x for x in pair[1].symbols if x[0] == name), None)
        assert other and other[2] == length and code(pair[0], symbol) == code(pair[1], other), (source, name)
        count += 1; size += length
    rows.append(dict(source=source,unchanged_functions=count,unchanged_instruction_bytes=size))
    print(source, count, size, 'unchanged instruction bytes after relocation normalization')
(N/'baseline-evidence.json').write_text(json.dumps(rows, indent=2)+'\n')
