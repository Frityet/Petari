#!/usr/bin/env python3
"""Prove the existing counter reset and stage the complete original sensor TU.

No production source changes, shared build, actor construction, or execution.
The whole-object link is deliberately diagnostic and must remain unexecuted.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / "build/original-mario-init-for-jump-20260903"
STAGE = BUILD / "staged"
INCLUDE = STAGE / "include"
STAGE.mkdir(parents=True, exist_ok=True)
commands = []


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def run(command, label, cwd=ROOT, expected=0):
    commands.append(command)
    result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    (BUILD / (label + ".command.json")).write_text(json.dumps(command, indent=2) + "\n")
    (BUILD / (label + ".log")).write_text(result.stdout)
    assert result.returncode == expected, (label, result.returncode, result.stdout)
    return result.stdout


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


compiler = module("compiler", "pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py")
reader = module("reader", "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
root_source = ROOT / "src/Game/Player/MarioActorSensor.cpp"
native_source = ROOT / "pc-port/src/Game/Player/MarioActorSensor.cpp"
assert root_source.read_bytes() == native_source.read_bytes()
source = root_source.read_text()
baseline = STAGE / "MarioActorSensor.cpp"
baseline.write_text(source)
root_object = BUILD / "MarioActorSensor.ppc.o"
run(compiler.compiler("cflags_game") + ["-c", str(root_source), "-o", str(root_object)], "original")
elf = reader.Elf(root_object)
name = "initForJump__10MarioActorFv"
_, start, size, section = next(s for s in elf.symbols if s[0] == name)
code = elf.section_data(section)[start:start + size]
dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
assert size == 16 and code == bytes.fromhex("3800000098030988980309894e800020")
assert code == reader.dol_bytes(dol, 0x802BFB74, size)
assert not elf.references(name)
probe = BUILD / "layout.cpp"
probe.write_text('#include "Game/Player/MarioActor.hpp"\n#include <stddef.h>\n'
                 'typedef char Check988[(offsetof(MarioActor,_988)==0x988)?1:-1];\n'
                 'typedef char Check989[(offsetof(MarioActor,_989)==0x989)?1:-1];\n')
run(compiler.compiler("cflags_game") + ["-c", str(probe), "-o", str(BUILD / "layout.o")], "layout")

# Use the actual configured Game compiler options, with only input/output and
# staged include paths changed. Do not silently add the entire root include tree.
entries = json.loads((ROOT / "pc-port/compile_commands.json").read_text())
entry = next(e for e in entries if e["file"].endswith("/XanimePlayer.cpp"))
prefix = []
skip = False
for argument in entry["arguments"]:
    if skip:
        skip = False
        continue
    if argument == "-o":
        skip = True
        continue
    if argument not in ("-c", entry["file"]):
        prefix.append(argument)
prefix += ["-fno-color-diagnostics", "-ferror-limit=0"]
baseline_log = run(prefix + ["-c", str(baseline), "-o", str(BUILD / "baseline.o")],
                   "native-baseline", Path(entry["directory"]), expected=1)
assert "CollectCounter.hpp' file not found" in baseline_log
for path in ("Game/MapObj/CollectCounter.hpp", "Game/Player/MarioActor.hpp"):
    target = INCLUDE / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes((ROOT / "include" / path).read_bytes())
prefix[1:1] = ["-I" + str(INCLUDE)]
assert source.count("_468 != nullptr") == 3 and source.count("startPadVib(0ul)") == 1
adapted = STAGE / "MarioActorSensor.adapted.cpp"
adapted.write_text(source.replace("_468 != nullptr", "_468 != 0")
                  .replace("startPadVib(0ul)", "startPadVib(static_cast<u32>(0))"))
native_object = BUILD / "MarioActorSensor.adapted.o"
run(prefix + ["-c", str(adapted), "-o", str(native_object)], "native-adapted", Path(entry["directory"]))
adapted_object = BUILD / "MarioActorSensor.adapted.ppc.o"
run(compiler.compiler("cflags_game") + ["-c", str(adapted), "-o", str(adapted_object)], "original-adapted")
other = reader.Elf(adapted_object)
equivalence = []
normalize = lambda refs: [{k: v for k, v in r.items() if k != "symbol" or not r["symbol"].startswith("@")} for r in refs]
for symbol, offset, length, section in elf.symbols:
    if symbol.startswith(".") or "__10MarioActor" not in symbol or not length:
        continue
    _, offset2, length2, section2 = next(s for s in other.symbols if s[0] == symbol)
    assert length == length2
    assert elf.section_data(section)[offset:offset + length] == other.section_data(section2)[offset2:offset2 + length2]
    assert normalize(elf.references(symbol)) == normalize(other.references(symbol))
    equivalence.append({"symbol": symbol, "size": length, "original_and_adapted_code_and_relocations_equal": True})
assert len(equivalence) == 11

# Retain the complete TU, including the actual original nerve methods. Link
# failure exposes the real next providers; never run this diagnostic binary.
main = BUILD / "main.cpp"
main.write_text("int main() { return 0; }\n")
run(prefix + ["-c", str(main), "-o", str(BUILD / "main.o")], "main", Path(entry["directory"]))
deps = ROOT / "pc-port/build/.deps/smg-pc-showcase/macosx/arm64/debug/smg-pc-showcase.d"
values = re.search(r"values = \{(.*?)\n    \}", deps.read_text(), re.S)[1]
flags = re.findall(r'"((?:\\.|[^"\\])*)"', values)
flags = [x for x in flags if x != "-Wl,-dead_strip"]
link = [flags[0], str(BUILD / "main.o"), str(native_object),
        str(ROOT / "pc-port/build/.objs/smg-pc-showcase/macosx/arm64/debug/aurora/lib/compat.cpp.o"),
        *flags[1:], "-o", str(BUILD / "link-probe")]
log = run(link, "link", ROOT / "pc-port", expected=1)
unresolved = re.findall(r'^  "(.*?)", referenced from:', log, re.M)
assert "MarioActor::initForJump()" not in unresolved
direct = []
for block in re.findall(r'^  (".*?", referenced from:\n(?:.*\n)*?)(?=^  "|^ld:)', log, re.M):
    lines = block.splitlines()
    callers = [line.strip() for line in lines[1:] if " in MarioActorSensor.adapted.o" in line]
    if callers:
        direct.append({"symbol": lines[0].split('\", referenced')[0].strip('"'), "callers": callers})
report = {
    "scope": "Existing root method proof; staged complete source/native link frontier only. No production edits or execution.",
    "dol_sha1": hashlib.sha1(dol).hexdigest(),
    "source_sha256": {str(p.relative_to(ROOT)): sha(p) for p in
                      (root_source, native_source, ROOT / "include/Game/Player/MarioActor.hpp",
                       ROOT / "pc-port/src/Game/Player/MarioActor.hpp", ROOT / "include/Game/MapObj/CollectCounter.hpp")},
    "method": {"symbol": name, "address": "0x802bfb74", "size": 16, "word_hex": code.hex(),
               "original_compiler_bytes_equal_retail_without_normalization": True, "relocations": [],
               "original_member_offsets": {"_988": 2440, "_989": 2441}},
    "staged_source_adaptation_equivalence": equivalence,
    "native_link_frontier": {"all_unresolved_count": len(unresolved), "all_unresolved": unresolved,
                             "direct_unresolved_count": len(direct), "direct_unresolved": direct},
    "commands": commands,
}
(HERE / "evidence.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"PASS initForJump: all four retail words exact; 11 source adaptations equivalent; native full TU compiles; {len(direct)} direct link edges remain")
