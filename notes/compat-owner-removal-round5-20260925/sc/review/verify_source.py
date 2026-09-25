#!/usr/bin/env python3
"""Read-only migration comparison against root's pre-edit snapshots."""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
BEFORE = ROOT / "notes/compat-owner-removal-round5-20260925/sc/before"
checks = []


def read(path):
    return path.read_text()


def tokens(source):
    source = re.sub(r'^\s*#include[^\n]*$', '', source, flags=re.M)
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|\w+|[^\s]'
    return [s for s in re.findall(pattern, source) if not s.startswith(('//', '/*'))]


def check(name, result):
    checks.append({"name": name, "passed": bool(result)})


sdk = "aurora/include/revolution/sc.h"
check("SDK declarations copied byte for byte", (ROOT / sdk).read_bytes() == (BEFORE / "src/revolution/sc.h").read_bytes())
for new, old in [
    ("aurora/lib/dolphin/sc/SCapi.cpp", "src/compat/OriginalSystemConfigAccessors.cpp"),
    ("aurora/lib/dolphin/sc/SCProductInfo.cpp", "src/compat/OriginalSystemProductInfo.cpp"),
]:
    check(new + " body tokens preserved", tokens(read(ROOT / new)) == tokens(read(BEFORE / old)))

catalog = read(ROOT / "aurora/lib/dolphin/sc/SCSystem.cpp")
old_catalog = read(BEFORE / "src/runtime/SystemConfigService.cpp")
old_catalog = old_catalog.replace("namespace smgpc::runtime", "namespace aurora").replace("SystemConfigService", "SystemConfiguration")
old_catalog = old_catalog.replace("compat::JkrHostAllocationScope", "aurora::allocation::HostAllocationScope")
check("Complete catalog body preserved except namespace and actual allocator alias", tokens(catalog.split('extern "C"')[0]) == tokens(old_catalog))
check("Old allocator name is exactly new Aurora allocator alias", "using JkrHostAllocationScope = aurora::allocation::HostAllocationScope;" in read(ROOT / "src/compat/JkrAllocationDomain.hpp"))

bridge = catalog[catalog.index('extern "C"'):]
old_bridge = read(BEFORE / "src/compat/SystemConfigSdkCompat.cpp")
old_bridge = old_bridge[old_bridge.index('extern "C"'):]
old_bridge = old_bridge.replace("InterruptScope interrupts;", "const aurora::InterruptScope interrupts;")
old_bridge = old_bridge.replace("smgpc::runtime::SystemConfigService", "aurora::SystemConfiguration")
check("All six C entry bodies preserve outer interrupt scope before lookup", tokens(bridge) == tokens(old_bridge) and bridge.count("const aurora::InterruptScope interrupts;") == 6)

old_header = read(BEFORE / "src/runtime/SystemConfigService.hpp").replace("namespace smgpc::runtime", "namespace aurora").replace("SystemConfigService", "SystemConfiguration")
check("Catalog declaration/layout preserved except namespace", tokens(old_header) == tokens(read(ROOT / "aurora/include/aurora/system_config.hpp")))

for path in ["src/app/OriginalGameApplication.cpp", "src/runtime/RuntimeContext.cpp", "src/runtime/RuntimeContext.hpp"]:
    expected = read(BEFORE / path).replace('#include "runtime/SystemConfigService.hpp"', '#include <aurora/system_config.hpp>')
    expected = expected.replace("class SystemConfigService;", "")
    expected = expected.replace("runtime::SystemConfigService", "aurora::SystemConfiguration").replace("SystemConfigService", "aurora::SystemConfiguration")
    check(path + " retains owner lifetime and all behavior", tokens(expected) == tokens(read(ROOT / path)))

test_paths = sorted((BEFORE / "tests").glob("*.cpp"))
for old_path in test_paths:
    path = "tests/" + old_path.name
    expected = read(old_path).replace("smgpc::runtime::SystemConfigService", "aurora::SystemConfiguration")
    expected = expected.replace("runtime::SystemConfigService", "aurora::SystemConfiguration").replace("SystemConfigService", "aurora::SystemConfiguration")
    check(path + " retains test logic", tokens(expected) == tokens(read(ROOT / path)))

files = ["aurora/include/aurora/system_config.hpp", sdk] + ["aurora/lib/dolphin/sc/" + name + ".cpp" for name in ("SCSystem", "SCapi", "SCProductInfo")]
for path in files:
    check(path + " has no Game/compat dependency", not re.search(r'#include[^\n]*(?:Game/|compat/)|\bsmgpc\b', read(ROOT / path)))
for build in ["aurora/xmake.lua", "aurora/cmake/aurora_os.cmake"]:
    content = read(ROOT / build)
    for name in ("SCSystem", "SCapi", "SCProductInfo"):
        check(build + " has one " + name + " source", content.count("lib/dolphin/sc/" + name + ".cpp") == 1)

result = {
    "review_scope": "Source comparison and ownership review only; no build or runtime execution",
    "checks": checks,
    "check_count": len(checks),
    "all_passed": all(c["passed"] for c in checks),
    "source_sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in files},
}
out = Path(__file__).with_name("source-evidence.json")
out.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({"check_count": len(checks), "all_passed": result["all_passed"], "failed": [c for c in checks if not c["passed"]]}))
raise SystemExit(0 if result["all_passed"] else 1)
