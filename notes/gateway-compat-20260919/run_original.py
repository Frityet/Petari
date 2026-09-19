"""Record a bounded real-disc original-process run; no game input/state injection."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
notes = Path(__file__).resolve().parent
label = sys.argv[1]
frames = sys.argv[2] if len(sys.argv) > 2 else "300"
binary = root / "build/macosx/arm64/debug/smg-pc"
disc = root / "Super Mario Wii - Galaxy Adventure (Korea).rvz"
command = [str(binary), "--original", "--disc", str(disc), "--stage",
           "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", frames]
settings = {
    "SMGPC_SAVE_DIR": str(notes / (label + "-nand")),
    "AURORA_BACKEND": "metal",
    "SMGPC_WINDOW_WIDTH": "1280", "SMGPC_WINDOW_HEIGHT": "720",
    "SMGPC_SCREENSHOT_PATH": str(notes / (label + "-frame240.png")),
    "SMGPC_SCREENSHOT_FRAME": "240",
}
record = {"command": command, "environment": settings,
          "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}
start = time.monotonic()
with (notes / (label + ".log")).open("w") as output:
    process = subprocess.Popen(command, cwd=root, env=os.environ | settings,
                               stdout=output, stderr=subprocess.STDOUT)
    try:
        record["exit_code"] = process.wait(timeout=180)
        record["timeout_terminated"] = False
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        record["exit_code"] = process.returncode
        record["timeout_terminated"] = True
record["elapsed_seconds"] = time.monotonic() - start
(notes / (label + ".json")).write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps(record, indent=2))
