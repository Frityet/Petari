#!/usr/bin/env python3
"""Compile only CollisionArea with MWCC and verify its retail reference."""

import gzip
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
DECOMP = ROOT / "decomp"
UNIT = "Game/AreaObj/CollisionArea"
commands = subprocess.check_output(
    ["ninja", "-t", "commands", f"build/RMGK01/src/{UNIT}.o"], cwd=DECOMP, text=True
)
command = shlex.split(commands.splitlines()[-1].split(" && ")[0])
command.remove("-MMD")
command[command.index("-o") + 1] = str(HERE / "CollisionArea-wii.o")
compiled = subprocess.run(command, cwd=DECOMP, capture_output=True, text=True)
(HERE / "wii-compile.log").write_text(compiled.stdout + compiled.stderr)
(HERE / "wii-command.json").write_text(
    json.dumps({"cwd": str(DECOMP), "command": command, "exit_code": compiled.returncode}, indent=2) + "\n"
)
compiled.check_returncode()

target = DECOMP / f"build/RMGK01/obj/{UNIT}.o"
report = subprocess.check_output(
    [str(DECOMP / "build/tools/objdiff-cli"), "diff", "-1", str(target),
     "-2", str(HERE / "CollisionArea-wii.o"), "-o", "-"], cwd=DECOMP
)
with gzip.GzipFile(str(HERE / "objdiff.json.gz"), "wb", mtime=0) as output:
    output.write(report)
comparison = json.loads(report)

# Retain every instruction bit except fields explicitly rewritten by the
# reference ELF's relocations, then compare all text against the retail DOL.
elf = target.read_bytes()
header = struct.unpack_from(">16sHHIIIIIHHHHHH", elf)
sections = [struct.unpack_from(">IIIIIIIIII", elf, header[6] + i * header[11])
            for i in range(header[12])]
names_section = sections[header[13]]
names = elf[names_section[4]:names_section[4] + names_section[5]]
text_index = next(i for i, section in enumerate(sections)
                  if names[section[0]:].split(b"\0", 1)[0] == b".text")
section = sections[text_index]
code = elf[section[4]:section[4] + section[5]]
mask = bytearray(b"\xff" * len(code))
relocation_count = 0
for section in sections:
    if section[1] != 4 or section[7] != text_index:
        continue
    for offset in range(section[4], section[4] + section[5], section[9]):
        location, info, addend = struct.unpack_from(">III", elf, offset)
        kind = info & 255
        if kind in (4, 6):  # ADDR16_LO / ADDR16_HA
            mask[location:location + 2] = b"\0\0"
        elif kind == 10:  # REL24; retain opcode and AA/LK bits
            struct.pack_into(">I", mask, location, 0xFC000003)
        elif kind == 109:  # EMB_SDA21; retain opcode and destination register
            struct.pack_into(">I", mask, location, 0xFFE00000)
        else:
            raise AssertionError(f"Unexpected relocation {kind}")
        relocation_count += 1

dol = (DECOMP / "orig/RMGK01/sys/main.dol").read_bytes()
start, end = 0x800207B0, 0x80021DB4
dol_offsets = struct.unpack_from(">18I", dol, 0)
dol_addresses = struct.unpack_from(">18I", dol, 0x48)
dol_sizes = struct.unpack_from(">18I", dol, 0x90)
retail = None
for offset, address, size in zip(dol_offsets, dol_addresses, dol_sizes):
    if address <= start and end <= address + size:
        retail = dol[offset + start - address:offset + end - address]
        break
assert retail is not None
assert len(retail) == len(code) == end - start
assert all((a & bits) == (b & bits) for a, b, bits in zip(code, retail, mask))

functions = [{k: s[k] for k in ("name", "size", "match_percent") if k in s}
             for s in comparison["left"]["symbols"]
             if "CollisionArea" in s["name"] or "AreaPolygon" in s["name"]]
for prefix in ("init__11AreaPolygon", "setSurface__11AreaPolygon", "hitCheck__13CollisionArea"):
    function = next(f for f in functions if f["name"].startswith(prefix))
    assert function["match_percent"] >= 95, function
summary = {
    "dol_sha1": hashlib.sha1(dol).hexdigest(),
    "retail_start": hex(start), "retail_end": hex(end),
    "retail_text_bytes": len(code),
    "retail_reference_comparison": "PASS (only ELF relocation bits masked)",
    "relocations": relocation_count,
    "sections": [{k: s[k] for k in ("name", "size", "match_percent") if k in s}
                 for s in comparison["left"]["sections"] if "match_percent" in s],
    "functions": functions,
    "canonical_source_sha256": hashlib.sha256((DECOMP / f"src/{UNIT}.cpp").read_bytes()).hexdigest(),
}
(HERE / "match-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
