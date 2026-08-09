#!/usr/bin/env python3
"""Measure the diagnostic-only legacy GX declaration tranche."""

from __future__ import annotations

import concurrent.futures
import csv
import json
import pathlib
import subprocess

import compile_probe


NOTE_DIR = pathlib.Path(__file__).resolve().parent
REPO_ROOT = NOTE_DIR.parents[2]
OVERLAY = "pc-port/notes/pc-player-compile-probe-20260809T035937Z/rvl_compile_overlay.hpp"


def compile_one(source: str) -> tuple[str, bool, str]:
    command = [*compile_probe.BASE_COMMAND, "-include", OVERLAY, source]
    result = subprocess.run(
        command,
        cwd=REPO_ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    first_error = next((line for line in result.stdout.splitlines() if "error:" in line), "")
    return source, result.returncode == 0, first_error


def main() -> None:
    with (NOTE_DIR / "compile-tu-ledger.tsv").open() as stream:
        failed = [row["source"] for row in csv.DictReader(stream, delimiter="\t") if row["status"] == "fail"]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        results = list(executor.map(compile_one, failed))
    unlocked = sorted(source for source, passed, _ in results if passed)
    still_failed = sorted(source for source, passed, _ in results if not passed)
    report = {
        "baseline_failed_tus": len(failed),
        "tus_unlocked_by_legacy_gx_declaration_tranche": len(unlocked),
        "unlocked": unlocked,
        "still_failed": len(still_failed),
        "tranche": [
            "preserve retail enum tags _GXAttr and _GXTlutSize in Aurora",
            "accept retail GXSetArray(GXAttr, const void*, u8) while retaining the sized PC upload path",
        ],
    }
    (NOTE_DIR / "rvl-tranche-measurement.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
