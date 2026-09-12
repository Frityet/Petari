import hashlib
import json
import os
from pathlib import Path
import re
import selectors
import subprocess
import time

root = Path(__file__).resolve().parents[2]
output = Path(__file__).resolve().parent
binary = root / "build/macosx/arm64/debug/smg-pc-showcase"
disc = root / "Super Mario Wii - Galaxy Adventure (Korea).rvz"
command = [str(binary), "gateway", "--disc", str(disc), "--max-frames", "240",
           "--width", "1280", "--height", "720", "--screenshot",
           str(output / "gateway.png"), "--screenshot-frame", "210"]
environment = dict(os.environ, SMGPC_DEBUG_SIMULATION_TIMING="1",
                   SMGPC_DEBUG_SDL_KEY_SCRIPT="60-100:w;120-125:space")
record = dict(command=command, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
              timeout=False, sdl_key_script=environment["SMGPC_DEBUG_SDL_KEY_SCRIPT"], frames=[])
start = time.monotonic()
process = subprocess.Popen(command, cwd=root, env=environment, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
selector = selectors.DefaultSelector()
selector.register(process.stdout, selectors.EVENT_READ)
pending = b""
with (output / "smoke.log").open("w") as log:
    while selector.get_map():
        if time.monotonic() - start > 45 and process.poll() is None:
            record["timeout"] = True
            process.kill()
        for key, _ in selector.select(0.2):
            chunk = os.read(key.fileobj.fileno(), 65536)
            if not chunk:
                selector.unregister(key.fileobj)
                break
            pending += chunk
            while b"\n" in pending:
                raw, pending = pending.split(b"\n", 1)
                line = raw.decode(errors="replace")
                log.write(line + "\n")
                match = re.search(r"\[smgpc:timing\] tick=(\d+) present=(\d+)", line)
                if match:
                    record["frames"].append(dict(wall_seconds=time.monotonic() - start,
                                                  tick=int(match[1]), present=int(match[2])))
                if "mode=supported_subset_for_development" in line:
                    record["placement_summary_line"] = line.strip()
    log.write(pending.decode(errors="replace"))
record["exit_code"] = process.wait(timeout=5)
record["wall_seconds"] = time.monotonic() - start
frames = record["frames"]
if len(frames) > 1:
    duration = frames[-1]["wall_seconds"] - frames[0]["wall_seconds"]
    record["present_fps"] = (frames[-1]["present"] - frames[0]["present"]) / duration
    record["simulation_ticks_per_second"] = (frames[-1]["tick"] - frames[0]["tick"]) / duration
record["screenshot_exists"] = (output / "gateway.png").is_file()
(output / "smoke.json").write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps({key: value for key, value in record.items() if key != "frames"}, indent=2))
raise SystemExit(record["exit_code"] or int(record["timeout"]))
