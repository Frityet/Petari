#!/usr/bin/env python3
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-alpha-clear-20260903'

def module(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value

compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
source = ROOT / 'src/Game/Util/DrawUtil.cpp'
BUILD.mkdir(parents=True, exist_ok=True)
command = compiler.compiler('cflags_game') + ['-c', str(source), '-o', str(BUILD / 'DrawUtil.o')]
result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
(BUILD / 'compile.log').write_text(result.stdout)
result.check_returncode()
retail = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Util/DrawUtil.o'
subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2', str(BUILD / 'DrawUtil.o'), '-o', str(BUILD / 'diff.json'), '--format', 'json-pretty'], check=True, stdout=subprocess.DEVNULL)
diff = json.loads((BUILD / 'diff.json').read_text())
elves = [reader.Elf(retail), reader.Elf(BUILD / 'DrawUtil.o')]
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
records = []
for name, address, size in [('clearAlphaBuffer__2MRFUc', 0x803CC10C, 0x78), ('clearAlphaBuffer__2MRFUcRCQ29JGeometry8TVec2<f>RCQ29JGeometry8TVec2<f>', 0x803CC184, 0x2E8)]:
    refs = [e.references(name) for e in elves]
    assert [r['symbol'] for r in refs[0] if r['kind'] == 10] == [r['symbol'] for r in refs[1] if r['kind'] == 10]
    sides = [next(s for s in diff[k]['symbols'] if s['name'] == name) for k in ('left', 'right')]
    assert sides[0]['match_percent'] >= 98
    assert int(sides[0]['size']) == int(sides[1]['size']) == size
    records.append({'name': name, 'address': hex(address), 'bytes': size, 'match_percent': sides[0]['match_percent'], 'same_direct_call_order': True, 'retail_references': refs[0], 'compiled_references': refs[1], 'retail_sha256': hashlib.sha256(compiler.dol_bytes(dol, address, size)).hexdigest()})
text = source.read_text()
a = text.index('    void clearAlphaBuffer(u8 alpha)')
b = text.index('    void fillScreenSetup(', a)
body = text[a:b].rstrip()
native = '#include "Game/Util/DrawUtil.hpp"\n#include "Game/Util/ScreenUtil.hpp"\n#include <JSystem/JUtility/JUTVideo.hpp>\n#include <dolphin/gx.h>\n#include <dolphin/mtx.h>\n\nnamespace MR {\n' + body + '\n}\n'
path = ROOT / 'pc-port/src/compat/OriginalAlphaClear.cpp'
assert path.read_text() == native
(NOTES / 'evidence.json').write_text(json.dumps({'dol_sha1': hashlib.sha1(dol).hexdigest(), 'source_sha256': hashlib.sha256(text.encode()).hexdigest(), 'native_body_sha256': hashlib.sha256(body.encode()).hexdigest(), 'command': command, 'functions': records}, indent=2) + '\n')
print([(r['name'], r['match_percent']) for r in records])
