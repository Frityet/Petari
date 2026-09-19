#!/usr/bin/env python3
"""Run the bounded original-owner zone placement test and retain provenance."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
binary = ROOT / "build/macosx/arm64/debug/smg-pc-original-process-placement-transform-tests"
base = HERE / (sys.argv[1] if len(sys.argv) > 1 else "placement-process")
if base.with_suffix(".json").exists() or base.with_suffix(".log").exists():
    raise SystemExit("Preserve the previous run: choose a new output basename")
environment = os.environ.copy()
environment["SMGPC_REAL_DISC"] = str(ROOT / "Super Mario Wii - Galaxy Adventure (Korea).rvz")
sha = hashlib.sha256(binary.read_bytes()).hexdigest()
start = time.monotonic()
timed_out = False
with base.with_suffix(".log").open("w") as log:
    process = subprocess.Popen([str(binary)], cwd=ROOT, env=environment, stdout=log, stderr=subprocess.STDOUT)
    try:
        result = process.wait(timeout=60)
    except subprocess.TimeoutExpired:
        timed_out = True
        process.kill()
        result = process.wait()
log = base.with_suffix(".log").read_text(errors="replace")
record = {
    "command": [str(binary)], "binary_sha256": sha, "pid": process.pid,
    "exit_code": result, "wall_seconds": time.monotonic() - start,
    "timed_out": timed_out, "pid_reaped": process.poll() is not None,
    "completed_120_frames": "Original GameSystem stopped after 120 completed frames" in log,
    "placement_assertions_passed": "PASS original-process zone transforms:" in log,
    "scope": "Read-only actual-owner nonidentity placement, rotation, rail, actor and area checks",
}
base.with_suffix(".json").write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps(record, indent=2))
print("\n".join(log.splitlines()[-12:]))
raise SystemExit(0 if result == 0 and not timed_out else 1)
