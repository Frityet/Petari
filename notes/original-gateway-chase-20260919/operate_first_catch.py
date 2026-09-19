"""Supervised, controller-only original Gateway diagnostic operator.

The waypoint is supplied by the caller from the actual authored map transform.
This script writes only the opt-in controller file. It cannot change game memory,
switches, actors, story state, or positions. The separate runner owns game exit.
"""
import argparse
import fcntl
import json
import os
from pathlib import Path
import time

from follow_actor import controls


def state(actor, name):
    return (actor is not None and
            "Nrv" + name + "E" in (actor.get("nerve") or {}).get("type", ""))


def newest(source):
    result = None
    while True:
        offset = source.tell()
        line = source.readline()
        if not line.endswith("\n"):
            source.seek(offset)
            return result
        result = json.loads(line)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("input", type=Path)
    parser.add_argument("--waypoint", type=float, nargs=3, required=True)
    parser.add_argument("--rabbit-id", type=int, help="Select one current runtime rabbit for a later catch")
    parser.add_argument("--start", type=int, default=1400)
    parser.add_argument("--end", type=int, default=16000)
    parser.add_argument("--timeout", type=float, default=600)
    args = parser.parse_args()
    owner = args.input.with_suffix(".operator-lock").open("a")
    fcntl.flock(owner, fcntl.LOCK_EX | fcntl.LOCK_NB)
    deadline = time.monotonic() + args.timeout
    while not args.trace.exists() and time.monotonic() < deadline:
        time.sleep(0.1)
    guide_id = None
    rabbit_id = None
    phase = "opening"
    caught_frame = None
    next_press = args.start
    current_press = ""
    with args.trace.open() as source:
        while time.monotonic() < deadline:
            snapshot = newest(source)
            if snapshot is None:
                time.sleep(0.08)
                continue
            frame = snapshot["frame_index"]
            actors = snapshot["actors"]
            player = next((a for a in actors if "player" in a), None)
            if player is None or frame < args.start:
                continue
            by_id = {a["id"]: a for a in actors}
            if guide_id is None:
                guide = next((a for a in actors if a["type"] == "10DemoRabbit" and
                              any(state(a, n) for n in ("Talk0", "Guide", "Wait", "Goal", "Talk1"))), None)
                if guide is not None:
                    guide_id = guide["id"]
            guide = by_id.get(guide_id)
            collector = next((a for a in actors if a["type"] == "20RunawayRabbitCollect"), None)
            if phase in ("opening", "guide", "dialogue"):
                if state(collector, "Active"):
                    phase = "reveal"
                    current_press = ""
                elif state(guide, "Talk1"):
                    phase = "dialogue"
                elif any(state(guide, n) for n in ("Wait", "Guide", "Goal")):
                    phase = "guide"
            if phase == "reveal":
                rabbit = next((a for a in actors if a["type"] == "13RunawayRabbit" and
                               (args.rabbit_id is None or a["id"] == args.rabbit_id) and
                               any(state(a, n) for n in ("Appear", "Runaway", "Stop", "BlowDamage"))), None)
                if rabbit is not None:
                    rabbit_id = rabbit["id"]
                    phase = "chase"
            rabbit = by_id.get(rabbit_id)
            if phase == "chase" and any(state(rabbit, n) for n in (
                    "TryCaughtDemo", "Caught", "CaughtTalk", "CaughtEnd")):
                phase = "caught"
                caught_frame = frame
                next_press = frame + 120
            x = y = 0
            distance = None
            target = None
            if phase == "guide":
                target = guide
            elif phase == "reveal":
                target = {"position": args.waypoint}
            elif phase == "chase":
                target = rabbit
            if target is not None:
                x, y, distance = controls(player, target, 0 if phase == "chase" else 130)
            tico_talk = any(a["type"] == "11RunawayTico" and not a["dead"] and
                            state(a, "Talk") for a in actors)
            if tico_talk:
                x = y = 0
            if (phase in ("opening", "dialogue", "caught") or tico_talk) and frame >= next_press:
                current_press = f"{frame + 1}-{frame + 10}:A"
                next_press = frame + 120
            if phase in ("guide", "reveal", "chase") and not tico_talk:
                current_press = ""
            finished = frame >= args.end or (phase == "caught" and
                rabbit is not None and rabbit["dead"] and not tico_talk and
                frame >= caught_frame + 240)
            if finished:
                x = y = 0
                current_press = ""
            command = {"buttons": current_press, "pointer": "",
                       "stick": f"{frame + 1}-{frame + 90}:{x:.6f}:{y:.6f}"}
            temporary = args.input.with_suffix(f".{os.getpid()}.next")
            temporary.write_text(json.dumps(command) + "\n")
            temporary.replace(args.input)
            print(json.dumps({"frame": frame, "phase": phase, "guide_id": guide_id,
                              "rabbit_id": rabbit_id, "player": player["position"],
                              "target": None if target is None else {
                                  k: target[k] for k in ("id", "position", "nerve") if k in target
                              }, "distance": distance,
                              "command": command, "finished": finished}), flush=True)
            if finished:
                return
    raise TimeoutError("Controller operator reached its wall-clock limit")


if __name__ == "__main__":
    main()
