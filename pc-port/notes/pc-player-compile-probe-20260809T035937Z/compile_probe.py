#!/usr/bin/env python3
"""Compile the exact root Player closure against the current PC/Aurora surface."""

from __future__ import annotations

import concurrent.futures
import csv
import json
import os
import pathlib
import re
import subprocess
from collections import Counter


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
LOG_DIR = NOTE_DIR / "compile-logs"

BASE_COMMAND = [
    "g++",
    "-fsyntax-only",
    "-fmax-errors=200",
    "-Wno-multichar",
    "-m64",
    "-g",
    "-O0",
    "-std=c++23",
    "-DSMGPC_DEBUG_BUILD",
    "-DAURORA_ENABLE_GX",
    "-DAURORA",
    "-DTARGET_PC",
    "-include",
    "pc-port/src/compat/MetrowerksStdCompat.hpp",
    "-Ipc-port/src/render",
    "-Ipc-port/src",
    "-Ipc-port/src/common",
    "-Ipc-port/notes/pc-player-compile-probe-20260809T035937Z/revolution-overlay",
    "-Ipc-port/aurora/include",
    "-Iinclude",
    "-Ilibs/JSystem/include",
    "-Ilibs/nw4r/include",
    "-Ilibs/RVLFaceLib/include",
]

ERROR_RE = re.compile(r"^(.*?):\d+(?::\d+)?: (?:fatal )?error: (.*)$")


def classify_error(path: str, message: str) -> str:
    text = f"{path} {message}".lower()
    # Put low-level SDK incompatibilities ahead of the J3D header that exposes
    # them: these require Aurora providers, not Game/J3D source edits.
    if any(term in text for term in (
        "_gxtlutsize", "_gxattr", "gxset", "gxbegin", "gxend", "gd", "revolution/",
        "dolphin/", "psmtx", "psvec", "wpad", "kpad", "osmutex", "osinterrupt",
    )):
        return "RVL/platform"
    if any(term in text for term in (
        "binder", "collisionparts", "collision parts", "kcl", "hitinfo", "triangle",
        "mplanenum", "floorcode", "wallcode",
    )):
        return "Binder/KCL"
    if "hitsensor" in text or "sensorkeeper" in text or "sensor keeper" in text:
        return "HitSensor"
    if any(term in text for term in (
        "effectkeeper", "effect keeper", "multiemitter", "particle", "aud", "sound",
        "bgm", "jaisound", "rumble",
    )):
        return "effects/audio"
    if any(term in text for term in (
        "resourceholder", "resource holder", "jkrarchive", "jkrheap", "archive", "rarc",
        "resourceinfo", "resource info",
    )):
        return "resources"
    if any(term in text for term in (
        "j3d", "jgeometry", "tvec", "tmatrix", "trotation", "tposition", "jmath",
        "xanime", "animation", "animator", "modelmanager", "model manager", "modeldata",
        "model data", "joint",
    )):
        return "J3D/model/animation"
    if any(term in text for term in (
        "liveactor", "nameobj", "nerve", "spine", "getbasemtx", "gettakingmtx",
        "initnerve", "mspine", "mflag", "override",
    )):
        return "LiveActor/NameObj/Nerve ABI"
    if any(term in text for term in (
        "jump to case label", "crosses initialization", "invalid conversion", "placement new",
        "loses precision", "incomplete type", "narrowing", "reinterpret_cast", "pointer",
        "private within this context", "dependent", "template",
    )):
        return "modern C++/64-bit compiler"
    return "other exact Game/API surface"


def normalize_signature(message: str) -> str:
    message = re.sub(r"‘[^’]*’", "<symbol>", message)
    message = re.sub(r"`[^`]*`", "<symbol>", message)
    message = re.sub(r"\b\d+\b", "<n>", message)
    return message


def compile_one(source: str) -> dict[str, object]:
    command = [*BASE_COMMAND, source]
    result = subprocess.run(
        command,
        cwd=REPO_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    errors: list[dict[str, str]] = []
    for line in result.stdout.splitlines():
        match = ERROR_RE.match(line)
        if not match:
            continue
        path, message = match.groups()
        errors.append({
            "path": path,
            "message": message,
            "category": classify_error(path, message),
            "signature": normalize_signature(message),
        })
    return {
        "source": source,
        "returncode": result.returncode,
        "errors": errors,
        "output": result.stdout,
    }


def main() -> None:
    sources = [
        line.strip()
        for line in (NOTE_DIR / "closure-sources.txt").read_text().splitlines()
        if line.strip()
    ]
    LOG_DIR.mkdir(exist_ok=True)
    workers = min(4, os.cpu_count() or 1)
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        results = list(executor.map(compile_one, sources))

    category_errors: Counter[str] = Counter()
    category_tus: Counter[str] = Counter()
    signatures: Counter[tuple[str, str]] = Counter()
    rows: list[dict[str, object]] = []
    for result in results:
        source = str(result["source"])
        name = pathlib.Path(source).stem
        log_path = LOG_DIR / f"{name}.log"
        if result["output"]:
            log_path.write_text(str(result["output"]))
        elif log_path.exists():
            log_path.unlink()
        errors = list(result["errors"])
        categories = sorted({str(error["category"]) for error in errors})
        for category in categories:
            category_tus[category] += 1
        for error in errors:
            category = str(error["category"])
            category_errors[category] += 1
            signatures[(category, str(error["signature"]))] += 1
        rows.append({
            "source": source,
            "status": "pass" if int(result["returncode"]) == 0 else "fail",
            "error_count": len(errors),
            "categories": ",".join(categories),
            "first_error_category": str(errors[0]["category"]) if errors else "",
            "first_error": str(errors[0]["message"]) if errors else "",
        })

    with (NOTE_DIR / "compile-tu-ledger.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys(), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    with (NOTE_DIR / "first-errors.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(["source", "category", "first_error"])
        for row in rows:
            if row["status"] == "fail":
                writer.writerow([row["source"], row["first_error_category"], row["first_error"]])
    with (NOTE_DIR / "compile-error-signatures.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(["category", "count", "normalized_signature"])
        for (category, signature), count in sorted(signatures.items(), key=lambda item: (-item[1], item[0])):
            writer.writerow([category, count, signature])

    passed = sum(row["status"] == "pass" for row in rows)
    failed = len(rows) - passed
    first_error_categories = Counter(
        str(row["first_error_category"]) for row in rows if row["status"] == "fail"
    )
    summary = {
        "attempted_source_tus": len(rows),
        "passed_tus": passed,
        "failed_tus": failed,
        "total_diagnostics_classified": sum(category_errors.values()),
        "category_error_counts": dict(category_errors),
        "category_tu_incidence": dict(category_tus),
        "first_error_category_counts": dict(first_error_categories),
        "command": BASE_COMMAND,
    }
    (NOTE_DIR / "compile-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
