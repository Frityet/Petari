#!/usr/bin/env python3
from pathlib import Path
import hashlib, json
r = Path(__file__).resolve().parents[3]
checks = []
def equal(a, b, extract=lambda s: s):
    source, native = r/a, r/b
    assert extract(source.read_text()) == extract(native.read_text()), (a, b)
    checks.append({"source": a, "native": b, "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "native_sha256": hashlib.sha256(native.read_bytes()).hexdigest()})
for name in ["AnmPlayer", "BtkPlayer", "BrkPlayer", "BtpPlayer", "BpkPlayer", "BvaPlayer"]:
    equal(f"src/Game/Animation/{name}.cpp", f"pc-port/src/Game/Animation/{name}.cpp")
    equal(f"include/Game/Animation/{name}.hpp", f"pc-port/src/Game/Animation/{name}.hpp")
equal("src/JSystem/J3DGraphAnimator/J3DMaterialAttach.cpp", "pc-port/src/compat/J3DMaterialAttachCompat.cpp", lambda s: s[s.index("int J3DMaterialTable::removeMatColorAnimator"):])
equal("src/JSystem/J3DGraphBase/J3DSys.cpp", "pc-port/src/compat/J3DDrawInitCompat.cpp", lambda s: s[s.index("void J3DSys::setTexCacheRegion"):])
Path(__file__).with_name("animation-import-evidence.json").write_text(json.dumps(checks, indent=2)+"\n")
print(f"PASS {len(checks)} exact original animation/SDK imports")
