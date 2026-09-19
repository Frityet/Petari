#!/usr/bin/env python3
"""Read-only analysis of accepted controller receipts and actual actor traces."""
import bisect
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import sys

HERE = Path(__file__).resolve().parent
PARENT = HERE.parent
sys.path.insert(0, str(PARENT))
from follow_actor import cross, dot, normalized, scale


def basis(state):
    up, cx, cy = state["movement_up"], state["camera_x"], state["camera_y"]
    cz = scale(state["camera_z"], -1)
    yd, zd = dot(cy, up), dot(cz, up)
    base = (scale(cy, -1 if zd < 0 else 1) if abs(zd) > abs(yd)
            else scale(cz, 1 if yd < 0 else -1))
    side, front = normalized(cross(base, up)), normalized(cross(cx, up))
    if dot(side, side) == 0:
        side = cx
    if dot(front, front) == 0:
        front = base
    return side, scale(front, -1)


def direction(axes, x, y):
    return normalized([a * x + b * y for a, b in zip(*axes)])


def module_margins(x, y):
    if abs(y) > 0.5:
        x = 0 if abs(x) < 0.25 else math.copysign((abs(x) - 0.25) / 0.75, x)
    elif abs(x) > 0.5:
        y = 0 if abs(y) < 0.2 else math.copysign((abs(y) - 0.2) / 0.8, y)
    return x, y


def input_stick(x, y):
    # Continuous double model; errors retain original JMath lookup rounding.
    x, y = max(-1, min(1, 1.5 * x)), max(-1, min(1, 1.5 * y))
    radius = min(1, math.hypot(x, y))
    angle = math.atan2(y, x) % (2 * math.pi)
    quadrant, angle = divmod(angle, math.pi / 2)
    angle = (0 if angle <= 0.1 else math.pi / 2 if angle >= math.pi / 2 - 0.1
             else (angle - 0.1) * (math.pi / 2) / (math.pi / 2 - 0.2))
    angle += quadrant * math.pi / 2
    return radius * math.cos(angle), radius * math.sin(angle)


def angle(a, b):
    return math.degrees(math.acos(max(-1, min(1, dot(normalized(a), normalized(b))))))


def summary(rows):
    return {key: {"min": min(row[key] for row in rows),
                  "median": statistics.median(row[key] for row in rows),
                  "max": max(row[key] for row in rows)}
            for key in rows[0] if key != "frame"}


receipts = []
log = PARENT / "baseline2-route.log"
trace = PARENT / "baseline2-route-actors.jsonl"
for line in log.open():
    match = re.search(r"\[debug-input-file\] frame=(\d+) revision=\d+ scripts=(.*)", line)
    if match:
        receipts.append((int(match[1]), json.loads(match[2])))
frames = [frame for frame, _ in receipts]
rows, excluded = [], {"missing_or_inactive_span": 0, "zero_raw_or_processed_stick": 0, "zero_pad": 0}
for line in trace.open():
    snapshot = json.loads(line)
    frame = snapshot["frame_index"]
    if not 19000 <= frame <= 29500:
        continue
    player = next(actor for actor in snapshot["actors"] if "player" in actor)
    target = next(actor for actor in snapshot["actors"] if actor["id"] == 912)
    state = player["player"]
    index = bisect.bisect_right(frames, frame) - 1
    if index < 0 or not receipts[index][1]["stick"]:
        excluded["missing_or_inactive_span"] += 1
        continue
    raw = receipts[index][1]["stick"].split(":")
    start, end = map(int, raw[0].split("-"))
    x, y = map(float, raw[1:])
    if not start <= frame <= end:
        excluded["missing_or_inactive_span"] += 1
        continue
    if math.hypot(x, y) < 0.1 or math.hypot(*state["stick_position"][:2]) < 0.1:
        excluded["zero_raw_or_processed_stick"] += 1
        continue
    world = state["world_pad_direction"]
    if dot(world, world) < 0.01:
        excluded["zero_pad"] += 1
        continue
    axes = basis(state)
    difference = [a - b for a, b in zip(target["position"], player["position"])]
    up = normalized(state["movement_up"])
    tangent = [a - dot(difference, up) * b for a, b in zip(difference, up)]
    processed = input_stick(x, y)
    rows.append(dict(frame=frame,
        raw_to_stick_deg=angle([*processed, 0], [*state["stick_position"][:2], 0]),
        basis_from_stick_to_pad_deg=angle(direction(axes, *module_margins(*state["stick_position"][:2])), world),
        linear_raw_to_pad_deg=angle(direction(axes, x, y), world),
        shaped_raw_to_pad_deg=angle(direction(axes, *module_margins(*processed)), world),
        distance=math.sqrt(dot(difference, difference)),
        radial_cos=dot(normalized(difference), up),
        tangent_distance=math.sqrt(dot(tangent, tangent)),
        velocity_pad_dot=dot(normalized(state["velocity"]), normalized(world)),
        target_tangent_pad_dot=dot(normalized(tangent), normalized(world))))

result = dict(
    scope="actual accepted input receipts; ordinary 3D forward-model comparison; no game writes",
    input_sha256={str(path.relative_to(PARENT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in (log, trace)},
    frame_range=[19000, 29500], target_id=912, count=len(rows), excluded=excluded,
    stats=summary(rows), late_stats=summary([row for row in rows if row["frame"] >= 24000]),
    samples=[row for row in rows if row["frame"] in (20000, 24000, 26000, 28000)],
)
(HERE / "controller-mapping-baseline.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
