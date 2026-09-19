#!/usr/bin/env python3
"""Bound and record the real original-process player diagnostic."""
from pathlib import Path
import hashlib
import json
import os
import signal
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
folder = Path(__file__).resolve().parent
label = sys.argv[1]
binary = root / "build/macosx/arm64/debug/smg-pc-original-process-player-owner-tests"
environment = os.environ.copy()
environment.update(SMGPC_REAL_DISC=str(root / "Super Mario Wii - Galaxy Adventure (Korea).rvz"), AURORA_BACKEND="metal")
record = {"command": [str(binary)], "cwd": str(root),
          "environment": {key: environment[key] for key in ("SMGPC_REAL_DISC", "AURORA_BACKEND")},
          "sha256": hashlib.sha256(binary.read_bytes()).hexdigest(), "timeout_seconds": 180}
start = time.monotonic()
with (folder / f"{label}-run.log").open("w") as log:
    process = subprocess.Popen([str(binary)], cwd=root, env=environment, stdout=log,
                               stderr=subprocess.STDOUT, start_new_session=True)
    record["pid"] = process.pid
    print(json.dumps({"pid": process.pid, "sha256": record["sha256"]}), flush=True)
    try:
        record["exit_code"] = process.wait(timeout=180)
    except subprocess.TimeoutExpired:
        record["timeout"] = True
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
        record["exit_code"] = process.returncode
record["elapsed_seconds"] = time.monotonic() - start
record["reaped"] = True
(folder / f"{label}-result.json").write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps(record), flush=True)
print((folder / f"{label}-run.log").read_text()[-6500:])
sys.exit(0 if record["exit_code"] == 0 else 1)
