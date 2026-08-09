#!/usr/bin/env python3
"""Measure exact NameObj/LiveActor headers alone and with the GX tranche."""

from __future__ import annotations

import concurrent.futures
import csv
import json
import pathlib
import subprocess

import compile_probe


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
ACTOR_OVERLAY = "-Ipc-port/notes/pc-player-compile-probe-20260809T035937Z/exact-actor-overlay"
RVL_OVERLAY = "pc-port/notes/pc-player-compile-probe-20260809T035937Z/rvl_compile_overlay.hpp"


def command_for(source: str, with_rvl: bool) -> list[str]:
    command = list(compile_probe.BASE_COMMAND)
    command.insert(command.index("-Ipc-port/src/render"), ACTOR_OVERLAY)
    if with_rvl:
        command.extend(["-include", RVL_OVERLAY])
    command.append(source)
    return command


def compile_one(item: tuple[str, bool]) -> tuple[str, bool, bool, str]:
    source, with_rvl = item
    result = subprocess.run(
        command_for(source, with_rvl),
        cwd=REPO_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    first_error = next((line for line in result.stdout.splitlines() if "error:" in line), "")
    return source, with_rvl, result.returncode == 0, first_error


def main() -> None:
    with (NOTE_DIR / "compile-tu-ledger.tsv").open() as stream:
        baseline_rows = list(csv.DictReader(stream, delimiter="\t"))
    sources = [row["source"] for row in baseline_rows]
    baseline_passed = {row["source"] for row in baseline_rows if row["status"] == "pass"}
    work = [(source, variant) for variant in (False, True) for source in sources]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        results = list(executor.map(compile_one, work))

    report: dict[str, object] = {"baseline_passed": len(baseline_passed)}
    for with_rvl, label in ((False, "exact_actor_abi"), (True, "exact_actor_abi_plus_legacy_gx")):
        passed = {source for source, variant, success, _ in results if variant == with_rvl and success}
        report[label] = {
            "passed_tus": len(passed),
            "newly_unlocked": sorted(passed - baseline_passed),
            "baseline_regressions": sorted(baseline_passed - passed),
        }
    (NOTE_DIR / "actor-abi-tranche-measurement.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
