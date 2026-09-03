#!/usr/bin/env python3
"""Verify the unchanged full player import with the original compiler."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import subprocess
import types

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / "build/original-xanime-player-20260903"
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("resource_verifier", ROOT / "pc-port/notes/original-xanime-lifecycle-20260903/verify-resource-import.py")
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)

sources = {"root": "src/Game/Animation/XanimePlayer.cpp", "native": "pc-port/src/Game/Animation/XanimePlayer.cpp"}
assert (ROOT / sources["root"]).read_bytes() == (ROOT / sources["native"]).read_bytes()
assert (ROOT / "include/Game/Animation/XanimePlayer.hpp").read_bytes() == (ROOT / "pc-port/src/Game/Animation/XanimePlayer.hpp").read_bytes()
for node in ast.parse((ROOT / "configure.py").read_text()).body:
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                     {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
        break
objects = {}
for label, source in sources.items():
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command += ["-c", source, "-o", str(BUILD / (label + ".o"))]
    (BUILD / (label + ".command.json")).write_text(json.dumps(command, indent=2) + "\n")
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (label + ".compile.log")).write_text(result.stdout)
    result.check_returncode()
    objects[label] = helper.Elf(BUILD / (label + ".o"))

functions = []
reference, imported = objects["root"], objects["native"]
for symbol in reference.symbols:
    name, start, size, index = symbol
    if name.startswith(".") or size == 0 or index == 0 or ("__12XanimePlayer" not in name and "__15XanimeFrameCtrl" not in name):
        continue
    other = next(s for s in imported.symbols if s[0] == name)
    expected_code, expected_relocations = reference.function(symbol)
    actual_code, actual_relocations = imported.function(other)
    assert actual_code == expected_code, (name, "instructions")
    assert actual_relocations == expected_relocations, (name, "relocations")
    functions.append({"symbol": name, "bytes": size, "relocations": len(expected_relocations)})
report = {
    "comparison": "Byte-identical root/native source and header; original-compiler instruction/relocation correspondence, not a full retail matching claim.",
    "source_sha256": hashlib.sha256((ROOT / sources["root"]).read_bytes()).hexdigest(),
    "function_count": len(functions),
    "instruction_count": sum(f["bytes"] // 4 for f in functions),
    "relocation_count": sum(f["relocations"] for f in functions),
    "functions": functions,
}
(HERE / "import-evidence.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps({k: v for k, v in report.items() if k != "functions"}, indent=2))
