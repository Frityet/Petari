#!/usr/bin/env python3
"""Record the read-only collision lifecycle audit's bounded binary evidence."""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[3]
DOL = ROOT / "build/compat-math-oracle/main.dol"
data = DOL.read_bytes()
assert hashlib.sha1(data).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"


def word(address):
    offsets = struct.unpack_from(">18I", data, 0)
    addresses = struct.unpack_from(">18I", data, 0x48)
    lengths = struct.unpack_from(">18I", data, 0x90)
    for offset, start, length in zip(offsets, addresses, lengths):
        if start <= address and address + 4 <= start + length:
            return struct.unpack_from(">I", data, offset + address - start)[0]
    raise ValueError(f"Unmapped DOL address {address:#x}")


checks = {
    0x801831A8: (0x480001E9, "setData calls isBinaryInitialized at 0x80183390"),
    0x801831AC: (0x2C030000, "compare initialization result with zero"),
    0x801831B0: (0x40820044, "nonzero branches past relocation to 0x801831F4"),
    0x80183390: (0x80030000, "predicate loads first 32-bit word"),
    0x80183394: (0x54030FFE, "predicate returns bit 31"),
    0x80183398: (0x4E800020, "predicate returns immediately"),
    0x80173BDC: (0x38E00020, "keeper constructs 32 HitInfo elements"),
    0x80173C54: (0x801D00E0, "keeper movement reads parts category at 0xE0"),
    0x80173C5C: (0x4082000C, "different category skips parts updateMtx"),
    0x80173C64: (0x48002549, "matching category calls updateMtx at 0x801761AC"),
    0x80174B7C: (0x48282D25, "getZone calls MR getZoneNum at 0x803F78A0"),
    0x80174BCC: (0x981C00A0, "getZone marks complete zone array initialized"),
    0x80174BDC: (0x80630018, "getZone returns directly indexed zone pointer"),
}
for address, (expected, _) in checks.items():
    assert word(address) == expected, f"Retail word changed at {address:#x}"

sources = [
    "include/Game/Map/KCollision.hpp",
    "src/Game/Map/KCollision.cpp",
    "src/Game/Map/CollisionParts.cpp",
    "src/Game/Map/CollisionCategorizedKeeper.cpp",
    "src/Game/Map/CollisionDirector.cpp",
    "src/Game/Map/HitInfo.cpp",
    "src/Game/Camera/GameCameraCreator.cpp",
    "pc-port/src/compat/CollisionPartsCompat.cpp",
    "pc-port/src/compat/HitInfoCompat.cpp",
    "pc-port/src/compat/GameMapCollisionCompat.cpp",
    "pc-port/src/compat/OriginalCollisionPartsCompat.cpp",
    "pc-port/src/compat/PlanetMapRuntimeCompat.cpp",
    "pc-port/src/Game/Util/JMapInfo.cpp",
    "pc-port/src/scene/StagePlacementResolver.cpp",
]
result = {
    "scope": "Read-only audit; no collision source changes or runtime activation",
    "dol_sha1": hashlib.sha1(data).hexdigest(),
    "retail_word_checks": [
        {"address": f"0x{address:08X}", "word": f"0x{expected:08X}", "meaning": meaning}
        for address, (expected, meaning) in checks.items()
    ],
    "source_sha256": {
        source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest()
        for source in sources
    },
}
Path(__file__).with_name("source-evidence.json").write_text(json.dumps(result, indent=2) + "\n")
print(f"Verified {len(checks)} retail instruction words; recorded {len(sources)} source hashes.")
