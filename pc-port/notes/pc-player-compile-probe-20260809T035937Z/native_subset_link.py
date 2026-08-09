#!/usr/bin/env python3
"""Compile and link-audit only the Player TUs that pass the syntax probe."""

from __future__ import annotations

import csv
import json
import pathlib
import subprocess
import tempfile
from collections import Counter

import compile_probe


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
ARCHIVE_DIR = REPO_ROOT / "pc-port/build/linux/x86_64/debug"


def symbols(path: pathlib.Path) -> tuple[set[str], set[str]]:
    output = subprocess.check_output(["llvm-nm", "-g", "-P", str(path)], text=True)
    definitions: set[str] = set()
    undefined: set[str] = set()
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue
        if fields[1] == "U":
            undefined.add(fields[0])
        else:
            definitions.add(fields[0])
    return definitions, undefined


def classify_native(demangled: str) -> str:
    text = demangled.lower()
    if any(term in text for term in ("gx", "gd", "psmtx", "psvec", "osmutex", "wpad", "kpad")):
        return "RVL/platform"
    if any(term in text for term in ("hitsensor", "sensorkeeper", "actorsensor")):
        return "HitSensor"
    if any(term in text for term in ("binder", "collision", "triangle", "kcl", "polygon", "floorcode")):
        return "Binder/KCL"
    if any(term in text for term in ("effect", "emitter", "particle", "sound", "aud", "bgm", "rumble")):
        return "effects/audio"
    if any(term in text for term in ("resource", "archive", "jkrheap", "dvd")):
        return "resources"
    if any(term in text for term in (
        "j3d", "jgeometry", "jmath", "xanime", "animation", "animator", "model", "joint", "matrix",
    )):
        return "J3D/model/animation"
    if any(term in text for term in ("liveactor", "nameobj", "nerve", "spine")):
        return "LiveActor/NameObj/Nerve ABI"
    if any(term in text for term in (
        "std::", "__cxa", "operator new", "operator delete", "memcpy", "memset", "strcmp", "strlen",
        "__gxx", "unwind", "pthread", "sqrt", "sin", "cos", "atan", "fmod",
    )):
        return "native compiler/system runtime"
    return "other Game/runtime provider"


def main() -> None:
    with (NOTE_DIR / "compile-tu-ledger.tsv").open() as stream:
        passed_sources = [
            row["source"] for row in csv.DictReader(stream, delimiter="\t") if row["status"] == "pass"
        ]
    built: list[pathlib.Path] = []
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="pc-player-native-") as temp_dir:
        object_dir = pathlib.Path(temp_dir)
        for source in passed_sources:
            output = object_dir / f"{pathlib.Path(source).stem}.o"
            command = [arg for arg in compile_probe.BASE_COMMAND if arg != "-fsyntax-only"]
            command.extend(["-c", source, "-o", str(output)])
            result = subprocess.run(command, cwd=REPO_ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            if result.returncode == 0:
                built.append(output)
            else:
                failures.append(source)
                (NOTE_DIR / f"native-compile-{pathlib.Path(source).stem}.log").write_text(result.stdout)
        object_symbols = [symbols(path) for path in built]
    definitions = set().union(*(entry[0] for entry in object_symbols))
    unresolved = set().union(*(entry[1] for entry in object_symbols)) - definitions

    archives = sorted(ARCHIVE_DIR.glob("*.a"))
    archive_command = ["llvm-nm", "-g", "-P", "--print-file-name", *(str(path) for path in archives)]
    archive_output = subprocess.check_output(archive_command, text=True)
    providers: dict[str, set[str]] = {}
    for line in archive_output.splitlines():
        if ": " not in line:
            continue
        path_text, fields_text = line.split(": ", 1)
        fields = fields_text.split()
        if len(fields) < 2 or fields[1] == "U" or fields[0] not in unresolved:
            continue
        providers.setdefault(fields[0], set()).add(pathlib.Path(path_text).name)

    raw_symbols = sorted(unresolved)
    demangled_output = subprocess.check_output(["c++filt"], input="\n".join(raw_symbols) + "\n", text=True)
    demangled = demangled_output.splitlines()
    rows: list[dict[str, str]] = []
    for raw, readable in zip(raw_symbols, demangled, strict=True):
        rows.append({
            "symbol": raw,
            "demangled": readable,
            "category": classify_native(readable),
            "pc_archive_provider": ",".join(sorted(providers.get(raw, set()))),
            "pc_archive_status": "provided" if raw in providers else "not found in PC archives",
        })
    with (NOTE_DIR / "native-subset-link-ledger.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys(), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    summary = {
        "syntax_pass_tus_requested": len(passed_sources),
        "native_objects_built": len(built),
        "native_compile_failures": failures,
        "native_subset_defined_symbols": len(definitions),
        "native_subset_external_symbols": len(unresolved),
        "symbols_found_in_current_pc_archives": sum(row["pc_archive_status"] == "provided" for row in rows),
        "symbols_not_found_in_current_pc_archives": sum(
            row["pc_archive_status"] != "provided" for row in rows
        ),
        "category_counts": dict(Counter(row["category"] for row in rows)),
    }
    (NOTE_DIR / "native-subset-link-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
