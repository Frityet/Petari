#!/usr/bin/env python3
"""Compile root Mario.cpp and compare six functions to the verified RMGK01 DOL.

Uses the configured original compiler, Shift-JIS wrapper, dtk, and objdiff.
Writes generated objects, split output, logs, and full diffs only under build/.
"""

import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/mario-update-restoration-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
NAMES = ("doExtraServices", "checkForceGrounding", "inputStick", "update", "actionMain", "updateGroundInfo")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    if result.stdout:
        print(result.stdout, end="")
    result.check_returncode()


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b"\x7fELF\x01\x02"
        offset = struct.unpack_from(">I", self.data, 0x20)[0]
        size, count = struct.unpack_from(">HH", self.data, 0x2E)
        self.sections = [struct.unpack_from(">10I", self.data, offset + i * size) for i in range(count)]
        section = next(s for s in self.sections if s[1] == 2)
        names = self.section_data(section[6])
        self.symbols = []
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", self.data, offset)
            self.symbols.append((names[name:names.index(0, name)].decode(), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]

    def references(self, function):
        _, start, size, index = next(s for s in self.symbols if s[0] == function)
        result = []
        for section in self.sections:
            if section[1] != 4 or section[7] != index:
                continue
            for offset in range(section[4], section[4] + section[5], section[9]):
                at, info, addend = struct.unpack_from(">IIi", self.data, offset)
                if not start <= at < start + size:
                    continue
                name, value, length, target_section = self.symbols[info >> 8]
                kind = info & 255
                record = {"offset": hex(at - start), "kind": kind, "symbol": name, "addend": addend}
                if kind != 10 and target_section and length:
                    payload = self.section_data(target_section)[value + addend:value + length]
                    # dtk sometimes includes the following strings in a string symbol's size.
                    # These references all point to ordinary zero-terminated char strings.
                    if length not in (4, 8):
                        payload = payload[:payload.index(0) + 1]
                    record["value_hex"] = payload.hex()
                result.append(record)
        return result


def dol_bytes(dol, address, size):
    for i in range(18):
        offset, base, length = [struct.unpack_from(">I", dol, field + i * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            return dol[offset + address - base:offset + address - base + size]
    raise AssertionError((hex(address), size))


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game compiler flags absent")
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command.extend(["-c", "src/Game/Player/Mario.cpp", "-o", str(BUILD / "Mario.o")])
    (BUILD / "Mario.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "Mario.compile.log")

    # Preserve production symbols/splits and use only the locally verified disc.
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
         str(BUILD / "retail")], "dtk.log")
    target = BUILD / "retail/obj/Game/Player/Mario.o"
    compiled = BUILD / "Mario.o"
    run(["build/tools/objdiff-cli", "diff", "-1", str(target), "-2", str(compiled), "-o",
         str(BUILD / "objdiff.json"), "--format", "json-pretty"], "objdiff.log")
    diff = json.loads((BUILD / "objdiff.json").read_text())
    elves = (Elf(target), Elf(compiled))
    retail_symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        match = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);.*size:(0x[0-9A-F]+)", line)
        if match:
            retail_symbols[match[1]] = (int(match[2], 0), int(match[3], 0))

    evidence = {
        "scope": "Six root Mario methods; fuzzy comparison plus reference checks, not complete gameplay validation.",
        "source_sha256": sha256(ROOT / "src/Game/Player/Mario.cpp"),
        "header_sha256": sha256(ROOT / "include/Game/Player/Mario.hpp"),
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "compiler": "GC/3.0a3, configure.py cflags_game, VERSION=0; sjiswrap v1.2.2",
        "tools": {p: sha256(ROOT / p) for p in ("build/compilers/GC/3.0a3/mwcceppc.exe",
                  "build/tools/sjiswrap.exe", "build/tools/dtk", "build/tools/objdiff-cli")},
        "tool_versions": {"dtk": "v1.8.3", "objdiff-cli": "v3.6.1", "sjiswrap": "v1.2.2"},
        "target_object_sha256": sha256(target), "compiled_object_sha256": sha256(compiled),
        "functions": [],
    }
    aligned = []
    for name in NAMES:
        symbol = name + "__5MarioFv"
        address, size = retail_symbols[symbol]
        sides = [next(s for s in diff[k]["symbols"] if s["name"] == symbol) for k in ("left", "right")]
        refs = [elf.references(symbol) for elf in elves]
        calls = [[r["symbol"] for r in rs if r["kind"] == 10] for rs in refs]
        assert calls[0] == calls[1], (name, "Direct call ordering changed")
        values = [{r["value_hex"] for r in rs if "value_hex" in r} for rs in refs]
        assert values[0] == values[1], (name, "Referenced constant/string values changed")
        globals_ = [Counter((r["kind"], r["symbol"], r["addend"]) for r in rs
                            if r["kind"] != 10 and "value_hex" not in r) for rs in refs]
        assert globals_[0] == globals_[1], (name, "External data/table references changed")
        assert sides[0]["match_percent"] >= 90.0
        record = {"name": symbol, "address": hex(address), "retail_size": size,
                  "compiled_size": int(sides[1]["size"]), "objdiff_match_percent": sides[0]["match_percent"],
                  "retail_function_sha256": hashlib.sha256(dol_bytes(dol, address, size)).hexdigest(),
                  "direct_calls_in_original_order": calls[0], "constant_and_string_values_hex": sorted(values[0]),
                  "retail_references": refs[0], "compiled_references": refs[1]}
        evidence["functions"].append(record)
        print(f"{name}: {sides[0]['match_percent']:.6f}%, {size}/{sides[1]['size']} bytes, "
              f"{len(calls[0])} direct calls in original order; constant/string/table references agree")
        aligned.append(f"\n{symbol} retail {address:#x}: {sides[0]['match_percent']}%")
        for left, right in zip(sides[0]["instructions"], sides[1]["instructions"]):
            texts = []
            for side, entry in enumerate((left, right)):
                inst = entry.get("instruction", {})
                relative = int(inst["address"]) - int(sides[side]["address"]) if inst else None
                location = f"{address + relative:08x}" if inst and side == 0 else f"+{relative:04x}" if inst else ""
                texts.append(f"{entry.get('diff_kind', ''):18} {location:9} {inst.get('formatted', '')}")
            aligned.append(f"{texts[0]:120} | {texts[1]}")
    (BUILD / "six-function-comparison.txt").write_text("\n".join(aligned) + "\n")
    (BUILD / "compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
