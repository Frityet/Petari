#!/usr/bin/env python3
"""Re-run the frozen 93-source/96-TU Player frontier against the live PC headers."""

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
LOG_DIR = NOTE_DIR / "compile-logs"


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
    if result.stdout:
        (LOG_DIR / f"{pathlib.Path(source).stem}.log").write_text(result.stdout)
    return source, result.returncode == 0, first_error


def main() -> None:
    baseline_module = load_baseline_module()
    command = list(baseline_module.BASE_COMMAND)
    with (BASELINE_DIR / "compile-tu-ledger.tsv").open() as stream:
        baseline_rows = list(csv.DictReader(stream, delimiter="\t"))
    sources = [row["source"] for row in baseline_rows]
    baseline_passed = {row["source"] for row in baseline_rows if row["status"] == "pass"}

    LOG_DIR.mkdir(exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        results = list(executor.map(lambda source: compile_one(command, source), sources))

    passed = {source for source, success, _ in results if success}
    newly_unlocked = sorted(passed - baseline_passed)
    regressions = sorted(baseline_passed - passed)
    expected = {
        "src/Game/Player/MarioCollision.cpp",
        "src/Game/Player/MarioEnforce.cpp",
        "src/Game/Player/MarioSpecial.cpp",
        "src/Game/Player/MarioWalk.cpp",
    }
    report = {
        "frozen_closure_tus": 96,
        "attempted_source_tus": len(sources),
        "frozen_baseline_passed_tus": len(baseline_passed),
        "live_header_passed_tus": len(passed),
        "newly_unlocked": newly_unlocked,
        "expected_actor_abi_unlocks_present": sorted(expected & passed),
        "expected_actor_abi_unlocks_missing": sorted(expected - passed),
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


if __name__ == "__main__":
    main()
