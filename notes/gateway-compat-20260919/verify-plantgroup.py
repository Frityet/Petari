#!/usr/bin/env python3
"""Reproduce the Wii recovery comparison without invoking the native build."""

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
UNIT = "Game/MapObj/PlantGroup"

commands = subprocess.check_output(
    ["ninja", "-t", "commands", f"build/RMGK01/src/{UNIT}.o"], cwd=DECOMP, text=True
)
command = shlex.split(commands.splitlines()[-1].split(" && ")[0])
command.remove("-MMD")
command[command.index("-o") + 1] = str(HERE / "PlantGroup-wii.o")
compiled = subprocess.run(command, cwd=DECOMP, capture_output=True, text=True)
(HERE / "plantgroup-compile.log").write_text(compiled.stdout + compiled.stderr)
(HERE / "plantgroup-wii-command.json").write_text(
    json.dumps({"cwd": str(DECOMP), "command": command, "exit_code": compiled.returncode}, indent=2) + "\n"
)
compiled.check_returncode()

target = DECOMP / f"build/RMGK01/obj/{UNIT}.o"
result = subprocess.check_output(
    [str(DECOMP / "build/tools/objdiff-cli"), "diff", "-1", str(target),
     "-2", str(HERE / "PlantGroup-wii.o"), "-o", "-"], cwd=DECOMP
)
with gzip.GzipFile(str(HERE / "plantgroup-objdiff.json.gz"), "wb", mtime=0) as output:
    output.write(result)
comparison = json.loads(result)

# Check that the reference ELF really contains the retail instructions. Ignore
# only bits rewritten by its explicit ELF relocations, not entire instructions.
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
start = 0x8020C7E0
dol_offsets = struct.unpack_from(">18I", dol, 0)
dol_addresses = struct.unpack_from(">18I", dol, 0x48)
dol_sizes = struct.unpack_from(">18I", dol, 0x90)
retail = None
for offset, address, size in zip(dol_offsets, dol_addresses, dol_sizes):
    if address <= start and start + len(code) <= address + size:
        retail = dol[offset + start - address:offset + start - address + len(code)]
        break
assert retail is not None
assert len(retail) == len(code) == 7092
assert all((a & bits) == (b & bits) for a, b, bits in zip(code, retail, mask))

copies = {}
for stem in ("PlantGroup", "CutBushModelObj"):
    for extension, folder in (("cpp", "src"), ("hpp", "include")):
        reference = DECOMP / folder / "Game/MapObj" / f"{stem}.{extension}"
        native = ROOT / "src/Game/MapObj" / f"{stem}.{extension}"
        assert reference.read_bytes() == native.read_bytes()
        copies[str(native.relative_to(ROOT))] = hashlib.sha256(native.read_bytes()).hexdigest()

summary = {
    "dol_sha1": hashlib.sha1(dol).hexdigest(),
    "retail_start": hex(start),
    "retail_end": hex(start + len(code)),
    "retail_text_bytes": len(code),
    "retail_reference_comparison": "PASS (only ELF relocation bits masked)",
    "relocations": relocation_count,
    "sections": [{k: s[k] for k in ("name", "size", "match_percent") if k in s}
                 for s in comparison["left"]["sections"]],
    "functions": [{k: s[k] for k in ("name", "size", "match_percent") if k in s}
                  for s in comparison["left"]["symbols"]
                  if "Plant" in s["name"] and "instructions" in s],
    "identical_port_copies_sha256": copies,
}
(HERE / "plantgroup-match-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps({k: summary[k] for k in (
    "dol_sha1", "retail_text_bytes", "retail_reference_comparison", "sections"
)}, indent=2))
