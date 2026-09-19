"""Record a bounded real-disc run, optionally replaying explicit debug controller input."""
import hashlib
import argparse
from datetime import datetime, timezone
import json
import os
import re
from pathlib import Path
import subprocess
import time

root = Path(__file__).resolve().parents[2]
notes = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("label")
parser.add_argument("frames", type=int, nargs="?", default=300)
parser.add_argument("--bundle", action="store_true", help="Run the native app bundle for live keyboard checks")
parser.add_argument("--screenshot-frame", type=int, default=240)
parser.add_argument("--timeout", type=float, default=180)
parser.add_argument("--button-script", help="Debug controller input spans; records scripted input explicitly")
parser.add_argument("--pointer-script", help="Debug pointer input spans; records scripted input explicitly")
parser.add_argument("--stick-script", help="Debug normalized stick input spans; records scripted input explicitly")
options = parser.parse_args()
label = options.label
frames = str(options.frames)
if Path(label).name != label or label in (".", ".."):
    parser.error("label must be one filename component")
if (notes / (label + ".json")).exists() or (notes / (label + "-nand")).exists():
    parser.error("use a new label to preserve earlier evidence and start with fresh save data")
binary = root / "build/macosx/arm64/debug/smg-pc"
if options.bundle:
    binary = binary.parent / "smg-pc.app/Contents/MacOS/smg-pc"
disc = root / "Super Mario Wii - Galaxy Adventure (Korea).rvz"
command = [str(binary), "--original", "--disc", str(disc), "--stage",
           "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", frames]
settings = {
    "SMGPC_SAVE_DIR": str(notes / (label + "-nand")),
    "AURORA_BACKEND": "metal",
    "SMGPC_WINDOW_WIDTH": "1280", "SMGPC_WINDOW_HEIGHT": "720",
    "SMGPC_SCREENSHOT_PATH": str(notes / (label + f"-frame{options.screenshot_frame}.png")),
    "SMGPC_SCREENSHOT_FRAME": str(options.screenshot_frame),
}
for name, value in [("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", options.button_script),
                    ("SMGPC_DEBUG_WPAD_POINTER_SCRIPT", options.pointer_script),
                    ("SMGPC_DEBUG_WPAD_STICK_SCRIPT", options.stick_script)]:
    # Empty values also prevent an unrecorded inherited script from affecting
    # what is intended to be a neutral-input run.
    settings[name] = value or ""
record = {"command": command, "environment": settings,
          "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
          "started_at": datetime.now(timezone.utc).isoformat()}
start = time.monotonic()
with (notes / (label + ".log")).open("w") as output:
    process = subprocess.Popen(command, cwd=root, env=os.environ | settings,
                               stdout=output, stderr=subprocess.STDOUT)
    record["pid"] = process.pid
    (notes / (label + "-launch.json")).write_text(json.dumps(record, indent=2) + "\n")
    try:
        record["exit_code"] = process.wait(timeout=options.timeout)
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
try:
    os.kill(process.pid, 0)
    record["process_still_exists_after_wait"] = True
except ProcessLookupError:
    record["process_still_exists_after_wait"] = False
completed = re.findall(r"Original GameSystem stopped after (\d+) completed frames",
                       (notes / (label + ".log")).read_text(errors="replace"))
record["completed_frames"] = int(completed[-1]) if completed else None
if record["process_still_exists_after_wait"]:
    # macOS debugger attachment can reparent the inferior. Python's wait then
    # reports zero for ECHILD, which is not the game's eventual exit status.
    record["reported_wait_exit_code"] = record["exit_code"]
    record["exit_code"] = None
    record["exit_status_note"] = "Process still exists; wait status cannot establish completion (possible debugger reparenting)."
record["verified_bounded_completion"] = (
    record["exit_code"] == 0 and not record["timeout_terminated"]
    and not record["process_still_exists_after_wait"]
    and record["completed_frames"] == options.frames)
(notes / (label + ".json")).write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps(record, indent=2))
