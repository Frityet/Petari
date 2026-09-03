#!/usr/bin/env python3
"""Record current jump source coverage; optionally parse original units for PC.

No production source/build configuration is changed. The syntax pass uses the
existing MarioMove compile database entry, replacing only compile/output/source
arguments; it does not link, execute gameplay, or establish behavior parity.
"""

import argparse
import ast
import hashlib
import json
from pathlib import Path
import subprocess
import shlex
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/original-mario-jump-20260903"
UNITS = (
    "Mario", "MarioMove", "MarioJump", "MarioCollision", "MarioSlope",
    "MarioActorGravity", "MarioWall", "MarioHang", "MarioSwim", "MarioSpin",
    "MarioBee", "MarioEnforce", "MarineSnow",
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command_prefix():
    entries = json.loads((ROOT / "pc-port/compile_commands.json").read_text())
    entry = next(e for e in entries if e["file"].endswith("/MarioMove.cpp"))
    args = entry["arguments"]
    result = []
    index = 0
    while index < len(args):
        item = args[index]
        index += 1
        if item == "-o":
            index += 1
        elif item not in ("-c", entry["file"]):
            result.append(item)
    return result + ["-fsyntax-only", "-ferror-limit=12", "-fno-color-diagnostics"], entry["directory"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--syntax", action="store_true")
    parser.add_argument("--original", action="store_true")
    options = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    report = {"scope": "Source identity and optional isolated syntax diagnostics; no link/runtime proof", "units": []}
    prefix, cwd = command_prefix()
    original_prefix = None
    if options.original:
        for node in ast.parse((ROOT / "configure.py").read_text()).body:
            if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
                flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                             {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
                break
        else:
            raise AssertionError("Configured Game flags absent")
        original_prefix = [str(ROOT / path) for path in (
            "build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe")]
        for flag in flags:
            original_prefix.extend(shlex.split(flag))
    report["compile_database_sha256"] = digest(ROOT / "pc-port/compile_commands.json")
    for name in UNITS:
        source = ROOT / "src/Game/Player" / (name + ".cpp")
        native = ROOT / "pc-port/src/Game/Player" / (name + ".cpp")
        record = {"unit": name, "root_sha256": digest(source), "native_sha256": digest(native),
                  "source_identical": source.read_bytes() == native.read_bytes()}
        if options.syntax:
            command = prefix + [str(source)]
            result = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            logfile = BUILD / (name + ".syntax.log")
            logfile.write_text(result.stdout)
            (BUILD / (name + ".command.json")).write_text(json.dumps(command, indent=2) + "\n")
            errors = [line for line in result.stdout.splitlines() if "error:" in line]
            record.update({"syntax_exit": result.returncode, "errors": errors,
                           "log": str(logfile.relative_to(ROOT))})
            print(name, "PASS" if result.returncode == 0 else "FAIL", *errors[:3], sep="\n  ", flush=True)
        if options.original:
            command = original_prefix + ["-c", str(source), "-o", str(BUILD / (name + ".original.o"))]
            result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            (BUILD / (name + ".original.log")).write_text(result.stdout)
            (BUILD / (name + ".original-command.json")).write_text(json.dumps(command, indent=2) + "\n")
            record["original_exit"] = result.returncode
            print(name, "original", "PASS" if result.returncode == 0 else "FAIL", flush=True)
            if result.returncode:
                print(result.stdout[-3000:], flush=True)
        report["units"].append(record)
    (BUILD / "source-probe.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
