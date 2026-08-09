#!/usr/bin/env python3
"""Reproduce the MarioActor-seeded Player object closure from RMGK02 objects."""

from __future__ import annotations

import json
import pathlib
import subprocess
import csv


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
PLAYER_OBJ_DIR = REPO_ROOT / "build/RMGK02/obj/Game/Player"
PLAYER_SRC_DIR = REPO_ROOT / "src/Game/Player"


def read_symbols(path: pathlib.Path) -> tuple[set[str], set[str]]:
    output = subprocess.check_output(["llvm-nm", "-g", "-P", str(path)], text=True)
    definitions: set[str] = set()
    undefined: set[str] = set()
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue
        name, symbol_type = fields[0], fields[1]
        if symbol_type == "U":
            undefined.add(name)
        else:
            definitions.add(name)
    return definitions, undefined


def main() -> None:
    objects = sorted(PLAYER_OBJ_DIR.glob("*.o"))
    symbols = {path.stem: read_symbols(path) for path in objects}
    providers: dict[str, list[str]] = {}
    for object_name, (definitions, _) in symbols.items():
        for symbol in definitions:
            providers.setdefault(symbol, []).append(object_name)

    # Strong definitions are unique for the symbols that expand this closure.
    # When the retail compiler emitted a duplicate inline/template definition,
    # prefer an already-reached provider, otherwise use the lexicographically
    # first Player provider for deterministic attribution.
    reached = {"MarioActor"}
    changed = True
    while changed:
        changed = False
        unresolved = set().union(*(symbols[name][1] for name in reached))
        unresolved -= set().union(*(symbols[name][0] for name in reached))
        for symbol in sorted(unresolved):
            candidates = providers.get(symbol, [])
            if not candidates or any(name in reached for name in candidates):
                continue
            provider = candidates[0]
            if provider not in reached:
                reached.add(provider)
                changed = True

    definitions = set().union(*(symbols[name][0] for name in reached))
    unresolved = set().union(*(symbols[name][1] for name in reached)) - definitions
    source_names = {path.stem for path in PLAYER_SRC_DIR.glob("*.cpp")}
    with_source = sorted(reached & source_names)
    without_source = sorted(reached - source_names)
    outside = sorted(set(symbols) - reached)

    report = {
        "declared_player_objects": len(objects),
        "closure_objects": len(reached),
        "closure_with_source": len(with_source),
        "closure_without_source": len(without_source),
        "outside_closure": len(outside),
        "closure_defined_symbols": len(definitions),
        "external_unresolved_symbols": len(unresolved),
        "with_source": with_source,
        "without_source": without_source,
        "outside": outside,
    }
    (NOTE_DIR / "closure.json").write_text(json.dumps(report, indent=2) + "\n")
    (NOTE_DIR / "closure-sources.txt").write_text(
        "".join(f"src/Game/Player/{name}.cpp\n" for name in with_source)
    )
    (NOTE_DIR / "closure-unresolved-symbols.txt").write_text(
        "".join(f"{symbol}\n" for symbol in sorted(unresolved))
    )
    compile_status: dict[str, str] = {}
    compile_ledger = NOTE_DIR / "compile-tu-ledger.tsv"
    if compile_ledger.exists():
        with compile_ledger.open() as stream:
            compile_status = {
                pathlib.Path(row["source"]).stem: row["status"]
                for row in csv.DictReader(stream, delimiter="\t")
            }
    with (NOTE_DIR / "frontier-matrix.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(["player_tu", "source_status", "native_syntax_status", "frontier_gate"])
        for name in sorted(reached):
            source_status = "present" if name in source_names else "absent"
            syntax_status = compile_status.get(name, "not attempted")
            gate = (
                "native-parseable"
                if syntax_status == "pass"
                else "syntax-blocked"
                if syntax_status == "fail"
                else "source-absent"
            )
            writer.writerow([name, source_status, syntax_status, gate])
    print(json.dumps({key: value for key, value in report.items() if not isinstance(value, list)}, indent=2))
    if without_source:
        print("source absent:", ", ".join(without_source))


if __name__ == "__main__":
    main()
