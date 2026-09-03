#!/usr/bin/env python3
"""Verify the original MarioTask data-symbol correction without replacing storage."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "build/original-player-identity-20260903"

def load(name, file):
    spec = importlib.util.spec_from_file_location(name, ROOT / file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

compiler = load("original_compiler", "pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py")
reader = load("elf_reader", "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
OUT.mkdir(parents=True, exist_ok=True)
baseline = subprocess.check_output(["git", "show", "ab048d55d:src/Game/Player/MarioTask.cpp"], cwd=ROOT)
(OUT / "MarioTask.baseline.cpp").write_bytes(baseline)
objects = []
for label, source in (("before", OUT / "MarioTask.baseline.cpp"), ("after", ROOT / "src/Game/Player/MarioTask.cpp")):
    obj = OUT / (label + ".o")
    command = compiler.compiler("cflags_game") + ["-c", str(source), "-o", str(obj)]
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (OUT / (label + ".log")).write_bytes(result.stdout)
    result.check_returncode()
    objects.append(reader.Elf(obj))
name = "startHipDropBlur__5MarioFv"
def extract(obj):
    _, start, size, section = next(s for s in obj.symbols if s[0] == name)
    references = obj.references(name)
    for reference in references:
        if reference["symbol"] == "lbl_806B6288":
            reference["symbol"] = "gIsLuigi"
    return obj.section_data(section)[start:start + size], references
before, after = map(extract, objects)
assert before == after
assert len(after[0]) == 240
evidence = dict(function=name, code_bytes=len(after[0]), code_sha256=hashlib.sha256(after[0]).hexdigest(),
                identical_code_and_references=True, old_name="lbl_806B6288", actual_owner="gIsLuigi",
                retail_address="0x806B6288", retail_owning_source="Game/Player/MarioActor.cpp", references=after[1])
Path(__file__).with_name("evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
print("MarioTask original instructions and canonicalized references are identical (240 bytes)")
