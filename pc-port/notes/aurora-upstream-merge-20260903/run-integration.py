#!/usr/bin/env python3
"""Run already-built integration executables serially and retain their evidence."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
PORT = HERE.parents[1]
BIN = PORT / "build/macosx/arm64/debug"
TARGETS = [
    "smg-pc-binder-kcl-mario-walk-tests",
    "smg-pc-aurora-native-tests",
    "smg-pc-original-camera-runtime-tests",
    "smg-pc-only-camera-tests",
    "smg-pc-camera-view-interpolator-tests",
    "smg-pc-camera-view-service-tests",
    "smg-pc-stage-start-camera-tests",
    "smg-pc-actor-event-camera-tests",
    "smg-pc-stage-collision-registration-tests",
    "smg-pc-collision-triangle-filter-tests",
    "smg-pc-original-xanime-core-tests",
    "smg-pc-original-j3d-joint-traversal-tests",
    "smg-pc-gateway-spin-checkpoint-tests",
    "smg-pc-mario-gateway-walk-tests",
    "smg-pc-showcase",
]


def main():
    targets = sys.argv[1:] or TARGETS
    unknown = set(targets) - set(TARGETS)
    if unknown:
        raise SystemExit(f"Unknown targets: {sorted(unknown)}")
    env = dict(os.environ)
    disc = env.setdefault("SMGPC_REAL_DISC", "../Super Mario Wii - Galaxy Adventure (Korea).rvz")
    results_path = HERE / "integration-results.json"
    results = json.loads(results_path.read_text()) if results_path.exists() else []
    failed = False
    for target in targets:
        executable = BIN / target
        command = [str(executable)]
        if target == "smg-pc-showcase":
            command += ["gateway", "--disc", disc, "--smoke", "--max-frames", "360",
                        "--screenshot", str(HERE / "showcase.png"), "--screenshot-frame", "20"]
        digest = hashlib.sha256(executable.read_bytes()).hexdigest()
        run_number = 1 + sum(item["target"] == target for item in results)
        log = HERE / f"{target}-{run_number}.log"
        started = time.monotonic()
        with log.open("wb") as output:
            try:
                completed = subprocess.run(command, cwd=PORT, env=env, stdout=output,
                                           stderr=subprocess.STDOUT, timeout=180)
                code = completed.returncode
            except subprocess.TimeoutExpired:
                code = "timeout"
        result = dict(target=target, command=command, exit_code=code,
                      seconds=round(time.monotonic() - started, 2),
                      real_disc=disc, tested_binary_sha256=digest,
                      log=str(log.relative_to(PORT)))
        results.append(result)
        results_path.write_text(json.dumps(results, indent=2) + "\n")
        print(f"{target}: {code} ({result['seconds']}s)", flush=True)
        failed |= code != 0
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
