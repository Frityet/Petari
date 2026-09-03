#!/usr/bin/env python3
"""Compile the isolated recovery without editing the parent-owned root TU."""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / "build/mario-random-texture-20260903"
NAME = "updateRandomTexture__10MarioActorFf"

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value

compiler = module("compiler", "pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py")
reader = module("reader", "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
BUILD.mkdir(parents=True, exist_ok=True)
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
source = (ROOT / "src/Game/Player/MarioActorSpecialDraw.cpp").read_text()
method = (NOTES / "method.cpp").read_text().rstrip()
placeholder = "// void MarioActor::updateRandomTexture(f32) {}"
if placeholder in source:
    source = source.replace(placeholder, method)
else:
    assert method in source, "Root recovery no longer matches the reviewed method"
if '#include "Game/Util/MathUtil.hpp"' not in source:
    source = source.replace('#include "Game/Util/SchedulerUtil.hpp"',
        '#include "Game/Util/MathUtil.hpp"\n#include "Game/Util/SchedulerUtil.hpp"')
if "#include <JSystem/JUtility/JUTTexture.hpp>" not in source:
    source = source.replace("#include <revolution/gd/GDBase.h>",
        "#include <JSystem/JUtility/JUTTexture.hpp>\n#include <revolution/gd/GDBase.h>")
if "#include <revolution/os/OSCache.h>" not in source:
    source = source.replace("#include <revolution/gx/GXPixel.h>",
        "#include <revolution/gx/GXPixel.h>\n#include <revolution/os/OSCache.h>")
path = BUILD / "MarioActorSpecialDraw.cpp"
path.write_text(source)
obj = BUILD / "MarioActorSpecialDraw.o"
command = compiler.compiler("cflags_game") + ["-c", str(path), "-o", str(obj)]
with (BUILD / "compile.log").open("w") as log:
    subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
retail = ROOT / "build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Player/MarioActorSpecialDraw.o"
subprocess.run([str(ROOT / "build/tools/objdiff-cli"), "diff", "-1", str(retail), "-2", str(obj),
                "-o", str(BUILD / "diff.json"), "--format", "json-pretty"], cwd=ROOT, check=True)
diff = json.loads((BUILD / "diff.json").read_text())
sides = [next(s for s in diff[side]["symbols"] if s["name"] == NAME) for side in ("left", "right")]
refs = [reader.Elf(path).references(NAME) for path in (retail, obj)]
calls = [[r["symbol"] for r in side if r["kind"] == 10] for side in refs]
constants = [{r["value_hex"] for r in side if "value_hex" in r} for side in refs]
assert calls[0] == calls[1]
assert constants[0] == constants[1] == {"00000000", "3f800000", "447a0000"}
assert int(sides[0]["size"]) == int(sides[1]["size"]) == 0x110
assert sides[0]["match_percent"] >= 99.0
assert [i["instruction"]["parts"][0] for i in sides[0]["instructions"]] == [
    i["instruction"]["parts"][0] for i in sides[1]["instructions"]]
evidence = {
    "dol_sha1": hashlib.sha1(dol).hexdigest(), "address": "0x802C2BFC", "bytes": 0x110,
    "objdiff_match_percent": sides[0]["match_percent"], "same_instruction_opcode_sequence": True,
    "original_call_order": calls[0], "constants": sorted(constants[0]),
    "retail_references": refs[0], "compiled_references": refs[1],
    "method_sha256": hashlib.sha256(method.encode()).hexdigest(),
    "compiled_source_sha256": hashlib.sha256(source.encode()).hexdigest(), "compiler_command": command,
    "retail_function_sha256": hashlib.sha256(compiler.dol_bytes(dol, 0x802C2BFC, 0x110)).hexdigest(),
}
(NOTES / "compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
print("PASS", NAME, sides[0]["match_percent"], "same 68 opcodes, calls and constants")
