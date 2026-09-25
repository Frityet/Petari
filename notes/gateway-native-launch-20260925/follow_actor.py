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


def raw_stick_for_direction(x, y, magnitude):
    """Invert the ordinary Mario stick shaping before the camera basis.

    The two authored MarioConst tables use the same margins. This external
    operator handles ordinary 3D walking; it does not bypass input-disable,
    retained direction, 2D modes or collision. JMath table rounding and the
    frame delay are left to the actual game.
    """
    if magnitude <= 0 or (x == 0 and y == 0):
        return 0.0, 0.0
    half_pi = math.pi / 2
    target = math.atan2(abs(y), abs(x))

    def world_stick_angle(angle):
        # MarioModule::calcWorldPadDir, after Mario::inputStick has already
        # produced a vector of the requested magnitude.
        px, py = magnitude * math.cos(angle), magnitude * math.sin(angle)
        if py > 0.5:
            px = max(0, (px - 0.25) / 0.75)
        elif px > 0.5:
            py = max(0, (py - 0.2) / 0.8)
        return math.atan2(py, px)

    low, high = 0.0, half_pi
    for _ in range(48):
        mid = (low + high) / 2
        if world_stick_angle(mid) < target:
            low = mid
        else:
            high = mid
    # The original threshold branches can leave a small unreachable angular
    # interval. Pick its closer endpoint instead of claiming an exact inverse.
    # Stay two original table bins inside either branch. A value exactly at a
    # threshold can cross it after JMath angle quantization or six-place script
    # serialization and select the other side of the discontinuity.
    bin_width = half_pi / 4096
    candidates = (max(0, low - 2 * bin_width), min(half_pi, high + 2 * bin_width))
    processed_angle = min(candidates, key=lambda angle: abs(world_stick_angle(angle) - target))
    if target == 0:
        input_angle = 0.0
    elif target == half_pi:
        input_angle = half_pi
    else:
        input_angle = 0.1 + processed_angle * (half_pi - 0.2) / half_pi
    # Keep the pre-clamp vector within the original circular magnitude. Sending
    # a unit raw vector would clip axes independently after the 1.5 multiplier.
    radius = magnitude / 1.5
    return math.copysign(radius * math.cos(input_angle), x), math.copysign(radius * math.sin(input_angle), y)


def controls(player, target, stop_distance=130, *, full_speed=False):
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
    magnitude = (1 if full_speed else min(1, max(0.35, distance / 350))) if distance > stop_distance else 0
    raw_x, raw_y = raw_stick_for_direction(x, y, magnitude) if length else (0, 0)
    return raw_x, raw_y, distance


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
