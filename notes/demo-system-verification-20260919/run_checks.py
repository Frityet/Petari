"""Serialize builds and probes, retaining every exit code and artifact hash."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

root = Path(__file__).resolve().parents[2]
notes = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("label")
parser.add_argument("targets", nargs="+")
parser.add_argument("--build-only", action="store_true")
parser.add_argument("--without-disc", action="store_true", help="Run portable checks; optional real-disc cases may explicitly skip")
args = parser.parse_args()
output = notes / (args.label + ".json")
if output.exists():
    parser.error("Use a fresh evidence label")
env = {k: v for k, v in os.environ.items() if not k.startswith("SMGPC_")}
env.update({"SMGPC_REAL_DISC": str(root / "Super Mario Wii - Galaxy Adventure (Korea).rvz"),
            "AURORA_BACKEND": "metal"})
if args.without_disc:
    env.pop("SMGPC_REAL_DISC")
results = []
for target in args.targets:
    record = {"target": target}
    for phase in (["build"] if args.build_only else ["build", "test"]):
        binary = root / "build/macosx/arm64/debug" / target
        command = ["xmake", "build", "-j", "8", target] if phase == "build" else [str(binary)]
        if phase == "test" and target.startswith("smg-pc-original-process-"):
            command += ["-ApplePersistenceIgnoreState", "YES"]
        if phase == "test" and target == "smg-pc-original-sensor-matrix-tests":
            command += ["--original-sensors"]
        if phase == "test":
            record["binary_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
        started = time.monotonic()
        with (notes / (args.label + "-" + target + "-" + phase + ".log")).open("w") as log:
            process = subprocess.Popen(command, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                code = process.wait(timeout=240)
            except subprocess.TimeoutExpired:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
                code = process.returncode
                record[phase + "_timed_out"] = True
        record[phase] = {"command": command, "exit_code": code, "elapsed_seconds": time.monotonic() - started}
        print(json.dumps({"target": target, "phase": phase, "exit_code": code}), flush=True)
        if code:
            break
    results.append(record)
    output.write_text(json.dumps(results, indent=2) + "\n")
raise SystemExit(any(r.get("build", {}).get("exit_code") != 0 or
                    (not args.build_only and r.get("test", {}).get("exit_code") != 0) for r in results))
