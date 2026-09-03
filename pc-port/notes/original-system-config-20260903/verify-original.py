#!/usr/bin/env python3
"""Verify the existing complete SDK setting accessors before native selection."""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-system-config-20260903'


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
proof = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
BUILD.mkdir(parents=True, exist_ok=True)
source = ROOT / 'src/RVL_SDK/sc/scapi.c'
output = BUILD / 'scapi.o'
command = compiler.compiler('cflags_sdk') + ['-c', str(source), '-o', str(output)]
subprocess.run(command, cwd=ROOT, check=True)
target = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/RVL_SDK/sc/scapi.o'
a, b = reader.Elf(target), reader.Elf(output)
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
rows = []
for name, offset, size, section in a.symbols:
    if not name.startswith('SC') or not size:
        continue
    address = 0x804D1BD8 + offset
    relocated, refs = proof.relocated(b, a, name, address, size, dol)
    assert relocated == reader.dol_bytes(dol, address, size), name
    rows.append({'name': name, 'address': hex(address), 'size': size,
                 'relocated_instructions_equal': True, 'verified_relocations': refs})
assert len(rows) == 19
(NOTES / 'original-accessor-evidence.json').write_text(json.dumps({
    'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'compiler_command': command, 'methods': rows}, indent=2) + '\n')
print(f'{len(rows)} original SC accessors; {sum(row["size"] for row in rows)} bytes exactly match relocated DOL')
