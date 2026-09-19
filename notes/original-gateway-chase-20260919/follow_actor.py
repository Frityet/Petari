"""External controller-only diagnostic driver using read-only original actor traces.

This is a test operator, not a runtime provider. It does not write game memory.
The caller explicitly selects one actor and the nervous states to follow.
Every accepted file revision is independently logged by the game process.
"""
import argparse
import json
import math
from pathlib import Path
import time


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def scale(a, s):
    return [x * s for x in a]


def normalized(a):
    length = math.sqrt(dot(a, a))
    return scale(a, 1 / length) if length > 1e-5 else [0, 0, 0]


def controls(player, target, stop_distance=130):
    # Reconstruct the two tangent input directions from captured fields. This
    # does not invoke calcMoveDir, whose gravity smoothing mutates Mario state.
    state = player["player"]
    up = state["movement_up"]
    cx, cy, cz = state["camera_x"], state["camera_y"], scale(state["camera_z"], -1)
    yd, zd = dot(cy, up), dot(cz, up)
    if abs(zd) > abs(yd):
        base = scale(cy, -1 if zd < 0 else 1)
        side = cross(base, up)
    else:
        base = scale(cz, 1 if yd < 0 else -1)
        side = cross(base, up)
    front = normalized(cross(cx, up))
    side = normalized(side)
    if dot(front, front) == 0:
        front = base
    if dot(side, side) == 0:
        side = cx
    forward = scale(front, -1)
    difference = [b - a for a, b in zip(player["position"], target["position"])]
    # The camera-derived basis need not be perpendicular. Solve its Gram system.
    aa, ab, bb = dot(side, side), dot(side, forward), dot(forward, forward)
    determinant = aa * bb - ab * ab
    if abs(determinant) < 1e-6:
        raise RuntimeError("Degenerate captured camera input basis")
    da, db = dot(difference, side), dot(difference, forward)
    x, y = (da * bb - db * ab) / determinant, (db * aa - da * ab) / determinant
    length = math.hypot(x, y)
    distance = math.sqrt(dot(difference, difference))
    magnitude = min(1, max(0.35, distance / 350)) if distance > stop_distance else 0
    return (x * magnitude / length, y * magnitude / length, distance) if length else (0, 0, distance)


def publish(path, x, y, frame):
    command = {"buttons": "", "pointer": "", "stick": f"{frame + 1}-{frame + 90}:{x:.6f}:{y:.6f}"}
    temporary = path.with_suffix(".next")
    temporary.write_text(json.dumps(command) + "\n")
    temporary.replace(path)
    return command


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("input", type=Path)
    parser.add_argument("actor", type=int, nargs="?", help="Runtime actor identity from this trace")
    parser.add_argument("--position", type=float, nargs=3, metavar=("X", "Y", "Z"),
                        help="Navigate toward a world position using controller input only")
    parser.add_argument("--start", type=int, default=2250)
    parser.add_argument("--end", type=int, default=11000)
    parser.add_argument("--states", default="Wait,Guide,Goal")
    parser.add_argument("--timeout", type=float, default=1400)
    args = parser.parse_args()
    if (args.actor is None) == (args.position is None):
        parser.error("select exactly one runtime actor or --position target")
    if args.position is not None and not all(math.isfinite(v) for v in args.position):
        parser.error("position components must be finite")
    deadline = time.monotonic() + args.timeout
    while not args.trace.exists() and time.monotonic() < deadline:
        time.sleep(0.1)
    allowed = args.states.split(",")
    with args.trace.open() as source:
        while time.monotonic() < deadline:
            offset = source.tell()
            line = source.readline()
            if not line.endswith("\n"):
                source.seek(offset)
                time.sleep(0.1)
                continue
            snapshot = json.loads(line)
            frame = snapshot["frame_index"]
            if frame < args.start:
                continue
            actors = snapshot["actors"]
            player = next(a for a in actors if "player" in a)
            target = next(a for a in actors if a["id"] == args.actor) if args.actor is not None else {
                "position": args.position, "dead": False
            }
            nerve = (target.get("nerve") or {}).get("type", "")
            x, y, distance = controls(player, target)
            complete = frame >= args.end or target["dead"] or (
                not any("Nrv" + n + "E" in nerve for n in allowed) if args.actor is not None else distance <= 130
            )
            if complete:
                x = y = 0
            command = publish(args.input, x, y, frame)
            print(json.dumps({"frame": frame, "player": player["position"], "target": target["position"],
                              "nerve": nerve, "distance": distance, "command": command, "finished": complete}), flush=True)
            if complete:
                break


if __name__ == "__main__":
    main()
