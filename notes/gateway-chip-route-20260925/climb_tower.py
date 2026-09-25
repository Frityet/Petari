"""Ordinary controller navigation over authored stair waypoints, followed by A presses.

This notes-only operator reads traces and writes the existing controller input file.
It cannot change game state, actor transforms, story flags or demo timing.
"""
import fcntl
import json
import math
from pathlib import Path
import time

from follow_actor import controls
from operate_first_catch import available, publish, StallJump

notes = Path(__file__).resolve().parent
deadline = time.monotonic() + 600
index = 0
jump = StallJump()
current_press = ""
next_press = 0
phase = "stairs"
plan = json.loads((notes / "stair-route.json").read_text())
points = plan["points"]
with (notes / "input.json.operator-lock").open("a") as lock:
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    try:
        with (notes / "route-actors.jsonl").open() as source:
            while time.monotonic() < deadline:
                snapshot = None
                for snapshot in available(source):
                    pass
                if snapshot is None:
                    time.sleep(0.08)
                    continue
                frame = snapshot["frame_index"]
                if frame >= 98000:
                    break
                player = next(a for a in snapshot["actors"] if "player" in a)
                info = player["player"]
                rosetta = next((a for a in snapshot["actors"] if a["type"] == "7Rosetta"), None)
                x = y = 0
                distance = None
                event = None
                buttons = ""
                if phase == "stairs":
                    target = {"position": points[index]}
                    x, y, distance = controls(player, target, 0)
                    if distance < plan.get("reach_distance", 130):
                        index += 1
                        if index == len(points):
                            phase = "tutorial"
                            x = y = 0
                        else:
                            x, y, distance = controls(player, {"position": points[index]}, 0)
                    if rosetta and math.dist(player["position"], rosetta["position"]) < 440:
                        phase = "tutorial"
                        x = y = 0
                    accepted = info.get("stick_position", [0, 0, 0])[2] > 0.05
                    buttons, event = jump.update(frame, player["position"], index,
                        info.get("status") == 0 and accepted, bool(info.get("movement_low_word",0) & 0x40000000))
                if phase == "tutorial":
                    if rosetta and rosetta["dead"] and frame > 16000:
                        print(json.dumps({"frame": frame, "event": "tutorial-finished-handoff"}), flush=True)
                        break
                    if frame >= next_press:
                        current_press = f"{frame+1}-{frame+6}:A"
                        next_press = frame + 100
                    buttons = current_press
                command = {"buttons": buttons, "pointer": "", "stick": f"{frame+1}-{frame+90}:{x:.6f}:{y:.6f}"}
                override_path = notes / "manual-input.json"
                if override_path.exists():
                    override = json.loads(override_path.read_text())
                    if frame < override.get("until_frame", 0):
                        ox, oy = override.get("stick", [x, y])
                        if not all(math.isfinite(v) and -1 <= v <= 1 for v in (ox, oy)):
                            raise ValueError("Manual input must be normalized")
                        command["stick"] = f"{frame+1}-{min(frame+90,override['until_frame'])}:{ox}:{oy}"
                        command["buttons"] = override.get("buttons", buttons)
                publish(notes / "input.json", command)
                print(json.dumps({"frame": frame, "phase": phase, "waypoint": index,
                    "player": player["position"], "target": points[min(index,len(points)-1)],
                    "distance": distance, "jump": event, "status": info.get("status"),
                    "rosetta": None if rosetta is None else {k:rosetta[k] for k in ("id","position","dead","hidden","nerve")},
                    "command": command}), flush=True)
    finally:
        publish(notes / "input.json", {"buttons": "", "pointer": "", "stick": ""})
