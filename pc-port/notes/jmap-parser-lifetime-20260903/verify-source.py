#!/usr/bin/env python3
"""Check the actual disposer provider and record the native ownership sources."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[3]
original = root / "src/JSystem/JKernel/JKRDisposer.cpp"
native = root / "pc-port/src/compat/JKRDisposerCompat.cpp"
assert original.read_bytes() == native.read_bytes(), "JKRDisposer provider differs from root"
paths = [original, native] + [root / path for path in (
    "pc-port/src/compat/NativeJkrDisposer.hpp",
    "pc-port/src/Game/Util/JMapInfo.hpp",
    "pc-port/src/Game/Util/JMapInfo.cpp",
    "pc-port/tests/JMapHeapLifetimeTests.cpp",
)]
result = {
    "original_disposer_provider_byte_identical": True,
    "source_sha256": {
        str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in paths
    },
}
print(json.dumps(result, indent=2))
