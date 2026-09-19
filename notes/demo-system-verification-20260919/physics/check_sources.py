#!/usr/bin/env python3
"""Reproduce the bounded physics source audit; no compiler/runtime claims."""
import difflib
import hashlib
import importlib.util
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
spec = importlib.util.spec_from_file_location("provider_audit", ROOT / "scripts/source_provider_audit.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def unwrap_literals(values):
    result, index = [], 0
    while index < len(values):
        if values[index] != "CP932":
            result.append(values[index])
            index += 1
            continue
        if values[index + 1] != "(":
            raise ValueError("Malformed CP932 wrapper")
        end = index + 2
        while end < len(values) and re.fullmatch(r'"(?:\\[\s\S]|[^"\\])*"', values[end]):
            end += 1
        if end == index + 2 or end >= len(values) or values[end] != ")":
            raise ValueError("CP932 accepts only ordinary string literal tokens here")
        result.extend(values[index + 2:end])
        index = end + 1
    return result


records = json.loads((HERE / "selected-source-checks.json").read_text())
for file, anchors in {
    "LiveActor/Spine.cpp": ["void Spine::update("],
    "Scene/SceneExecutor.cpp": ["void SceneFunction::executeMovementList(", "void SceneFunction::executeCalcAnimList("],
    "System/GameSystem.cpp": ["void GameSystem::frameLoop("],
    "System/GameSystemFrameControl.cpp": ["void GameSystemFrameControl::setMovement60fps("],
    "System/MainLoopFramework.cpp": ["void MainLoopFramework::waitForRetrace("],
}.items():
    records.extend(dict(file=file, anchor=anchor) for anchor in anchors)

differences, results = [], []
for record in records:
    file, anchor = record["file"], record["anchor"]
    original_path = ROOT / "decomp/src/Game" / file
    native_path = ROOT / "src/Game" / file
    original = audit.definition(original_path.read_text(), anchor)
    native = unwrap_literals(audit.definition(native_path.read_text(), anchor))
    result = dict(file=file, anchor=anchor, tokens_equal=(original == native),
                  canonical_file_sha256=hashlib.sha256(original_path.read_bytes()).hexdigest(),
                  native_file_sha256=hashlib.sha256(native_path.read_bytes()).hexdigest())
    results.append(result)
    if original != native:
        differences.append(file + " " + anchor + "\n" + "\n".join(difflib.unified_diff(original, native, fromfile="canonical", tofile="native")))
(HERE / "current-source-checks.json").write_text(json.dumps(results, indent=2) + "\n")
(HERE / "current-token-differences.txt").write_text("\n\n".join(differences) + "\n")

binder = ROOT / "src/Game/LiveActor/Binder.cpp"
historical = json.loads((ROOT / "notes/original-binder-reaction-20260903/runtime-evidence.json").read_text())
current_sha = hashlib.sha256(binder.read_bytes()).hexdigest()
proof = dict(
    status="source-restoration-only; no fresh MWCC/native build in this audit yet",
    native_source_sha256=current_sha,
    canonical_source_equal=(ROOT / "decomp/src/Game/LiveActor/Binder.cpp").read_bytes() == binder.read_bytes(),
    historical_retail_proof="notes/original-binder-reaction-20260903/runtime-evidence.json",
    historical_source_equal=current_sha == historical["source_sha256"],
    historical_dol_sha1=historical["dol_sha1"],
    historical_matches=[dict(method=row["method"], percent=row["objdiff_match_percent"])
                        for row in historical["functions"]],
)
(HERE / "binder-restoration-provenance.json").write_text(json.dumps(proof, indent=2) + "\n")
assert proof["canonical_source_equal"] and proof["historical_source_equal"]
print(f"{sum(row['tokens_equal'] for row in results)}/{len(results)} selected definitions token-exact; Binder historical source identity verified")
