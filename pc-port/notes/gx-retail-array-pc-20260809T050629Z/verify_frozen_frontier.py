#!/usr/bin/env python3
"""Re-run the frozen 93-source/96-TU Player frontier against live PC headers."""

from __future__ import annotations

import concurrent.futures
import csv
import importlib.util
import json
import pathlib
import subprocess


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
BASELINE_DIR = REPO_ROOT / "pc-port/notes/pc-player-compile-probe-20260809T035937Z"


def load_baseline_module():
    spec = importlib.util.spec_from_file_location("frozen_compile_probe", BASELINE_DIR / "compile_probe.py")
    if spec is None or spec.loader is None:
        raise RuntimeError("Unable to load frozen compile probe")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compile_one(command: list[str], source: str) -> tuple[str, bool, str]:
    result = subprocess.run(
        [*command, source],
        cwd=REPO_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    first_error = next((line for line in result.stdout.splitlines() if "error:" in line), "")
    return source, result.returncode == 0, first_error


def main() -> None:
    baseline_module = load_baseline_module()
    command = list(baseline_module.BASE_COMMAND)
    with (BASELINE_DIR / "compile-tu-ledger.tsv").open() as stream:
        baseline_rows = list(csv.DictReader(stream, delimiter="\t"))
    sources = [row["source"] for row in baseline_rows]
    baseline_passed = {row["source"] for row in baseline_rows if row["status"] == "pass"}

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        results = list(executor.map(lambda source: compile_one(command, source), sources))

    passed = {source for source, success, _ in results if success}
    newly_unlocked = sorted(passed - baseline_passed)
    regressions = sorted(baseline_passed - passed)
    actor_unlocks = {
        "src/Game/Player/MarioCollision.cpp",
        "src/Game/Player/MarioEnforce.cpp",
        "src/Game/Player/MarioSpecial.cpp",
        "src/Game/Player/MarioWalk.cpp",
    }
    gx_unlocks = {
        "src/Game/Player/JetTurtleShadow.cpp",
        "src/Game/Player/MarioActorHand.cpp",
        "src/Game/Player/MarioActorShadow.cpp",
        "src/Game/Player/MarioModule.cpp",
        "src/Game/Player/ModelHolder.cpp",
    }
    expected_unlocks = actor_unlocks | gx_unlocks
    report = {
        "frozen_closure_tus": 96,
        "attempted_source_tus": len(sources),
        "frozen_baseline_passed_tus": len(baseline_passed),
        "live_combined_passed_tus": len(passed),
        "live_combined_closure_ratio": f"{len(passed)}/96",
        "newly_unlocked": newly_unlocked,
        "actor_abi_unlocks": sorted(actor_unlocks & passed),
        "gx_surface_unlocks": sorted(gx_unlocks & passed),
        "expected_unlocks_missing": sorted(expected_unlocks - passed),
        "unexpected_unlocks": sorted((passed - baseline_passed) - expected_unlocks),
        "baseline_regressions": regressions,
        "command": command,
    }
    (NOTE_DIR / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    with (NOTE_DIR / "live-frontier.tsv").open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(["source", "status", "first_error"])
        for source, success, first_error in results:
            writer.writerow([source, "pass" if success else "fail", first_error])
    print(json.dumps(report, indent=2))

    if len(passed) != 61 or regressions or (passed - baseline_passed) != expected_unlocks:
        raise SystemExit("Frozen Player frontier did not match the expected 61/96 combined result")


if __name__ == "__main__":
    main()
