#!/usr/bin/env python3
"""Attribute the retail Player closure's external symbols to provider objects."""

from __future__ import annotations

import csv
import json
import pathlib
import re
import subprocess
from collections import Counter


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
OBJ_ROOT = REPO_ROOT / "build/RMGK02/obj"
GAME_XMAKE = REPO_ROOT / "pc-port/src/Game/xmake.lua"
EXCLUDED_GAME_SOURCES = set(re.findall(r'remove_files\("([^"]+)"\)', GAME_XMAKE.read_text()))


def classify(provider: str, symbol: str) -> str:
    lower_provider = provider.lower()
    lower_symbol = symbol.lower()
    if lower_provider.startswith("rvl_sdk/"):
        return "RVL/platform"
    if lower_provider.startswith(("runtime/", "msl_c/", "msl_c++/")):
        return "compiler/MSL runtime"
    if any(term in lower_symbol for term in (
        "hitsensor", "sensorkeeper", "actorsensor", "sensorhit",
    )) or any(term in lower_provider for term in (
        "hitsensor", "actorsensorutil", "sensorhitchecker",
    )):
        return "HitSensor"
    if any(term in lower_symbol for term in (
        "binder", "collision", "kcl", "polygon", "triangle", "floorcode", "wallcode",
        "binded",
    )) or any(term in lower_provider for term in (
        "/binder", "/collision", "/hitinfo", "maputil", "geometrybind",
    )):
        return "Binder/KCL"
    if any(term in lower_symbol for term in (
        "effect", "emitter", "particle", "sound", "aud", "bgm", "rumble",
    )) or any(term in lower_provider for term in (
        "game/effect", "game/audiolib", "game/rhythmlib", "effectutil", "soundutil",
        "jparticle", "jaudio",
    )):
        return "effects/audio"
    if any(term in lower_symbol for term in (
        "resource", "archive", "jkrheap", "jkrarchive", "dvd", "fileloader",
    )) or any(term in lower_provider for term in (
        "resourceholder", "fileutil", "jsystem/jkernel", "rarc",
    )):
        return "resources"
    if any(term in lower_symbol for term in (
        "j3d", "jgeometry", "jmath", "xanime", "animation", "animator", "model", "joint",
        "drawmtx", "calcanm",
    )) or any(term in lower_provider for term in (
        "jsystem/j3d", "game/animation", "modelmanager", "partsmodel", "modelutil",
        "jointutil", "mtxutil", "directdraw",
    )):
        return "J3D/model/animation"
    if any(term in lower_symbol for term in (
        "liveactor", "nameobj", "nerve", "spine",
    )) or any(term in lower_provider for term in (
        "game/nameobj", "liveactor/liveactor", "liveactor/spine", "nerveutil",
    )):
        return "LiveActor/NameObj/Nerve ABI"
    return "other Game/runtime provider"


def pc_status(provider: str) -> str:
    if provider.startswith("Game/"):
        relative = pathlib.Path(provider).with_suffix(".cpp")
        root_source = REPO_ROOT / "src" / relative
        pc_source = REPO_ROOT / "pc-port/src" / relative
        if not pc_source.exists():
            return "PC Game source absent"
        exactness = "byte-identical" if root_source.exists() and root_source.read_bytes() == pc_source.read_bytes() else "divergent"
        game_relative = str(relative.relative_to("Game"))
        linkage = "excluded" if game_relative in EXCLUDED_GAME_SOURCES else "included"
        return f"PC Game source {exactness}, {linkage}"
    if provider.startswith("JSystem/"):
        return "PC JSystem/Aurora provider requires API audit"
    if provider.startswith("RVL_SDK/"):
        return "Aurora subsystem present; retail API compatibility incomplete"
    if provider.startswith(("Runtime/", "MSL_C/", "MSL_C++/")):
        return "native compiler/runtime mapping"
    return "provider status unknown"


def main() -> None:
    unresolved = set((NOTE_DIR / "closure-unresolved-symbols.txt").read_text().splitlines())
    objects = sorted(OBJ_ROOT.rglob("*.o"))
    command = ["llvm-nm", "-g", "-P", "--print-file-name", *(str(path) for path in objects)]
    output = subprocess.check_output(command, text=True)
    candidates: dict[str, list[tuple[str, str]]] = {}
    prefix = f"{OBJ_ROOT}/"
    for line in output.splitlines():
        if ": " not in line:
            continue
        path_text, fields_text = line.split(": ", 1)
        fields = fields_text.split()
        if len(fields) < 2:
            continue
        symbol, symbol_type = fields[0], fields[1]
        if symbol not in unresolved or symbol_type == "U":
            continue
        provider = path_text.removeprefix(prefix).removesuffix(".o")
        candidates.setdefault(symbol, []).append((provider, symbol_type))

    rows: list[dict[str, object]] = []
    missing_providers: list[str] = []
    for symbol in sorted(unresolved):
        symbol_candidates = candidates.get(symbol, [])
        if not symbol_candidates:
            missing_providers.append(symbol)
            continue
        strong = [entry for entry in symbol_candidates if entry[1].upper() not in {"W", "V"}]
        selected = sorted(strong or symbol_candidates)[0][0]
        rows.append({
            "symbol": symbol,
            "category": classify(selected, symbol),
            "provider_object": f"{selected}.o",
            "provider_candidate_count": len(symbol_candidates),
            "pc_status": pc_status(selected),
        })

    with (NOTE_DIR / "provider-ledger.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys(), delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    category_counts = Counter(str(row["category"]) for row in rows)
    category_objects: dict[str, set[str]] = {}
    status_counts = Counter(str(row["pc_status"]) for row in rows)
    status_objects: dict[str, set[str]] = {}
    for row in rows:
        category_objects.setdefault(str(row["category"]), set()).add(str(row["provider_object"]))
        status_objects.setdefault(str(row["pc_status"]), set()).add(str(row["provider_object"]))
    summary = {
        "external_unresolved_symbols": len(unresolved),
        "symbols_with_retail_provider": len(rows),
        "symbols_without_retail_provider": len(missing_providers),
        "selected_provider_objects": len({str(row["provider_object"]) for row in rows}),
        "category_symbol_counts": dict(category_counts),
        "category_provider_object_counts": {
            category: len(objects) for category, objects in category_objects.items()
        },
        "pc_status_symbol_counts": dict(status_counts),
        "pc_status_provider_object_counts": {
            status: len(objects) for status, objects in status_objects.items()
        },
    }
    (NOTE_DIR / "provider-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    if missing_providers:
        print("missing retail providers:", ", ".join(missing_providers))


if __name__ == "__main__":
    main()
