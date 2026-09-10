"""Check observed movement, key release, and two jumps in the fixed demo replay.

Requires: w180-210, a270-300, s360-390, d450-480,
space600-605 and780-785, at least960ticks. This is synthetic SDL input.
"""
import argparse
import json
import math
import pathlib
import re

parser = argparse.ArgumentParser()
parser.add_argument("log", type=pathlib.Path)
args = parser.parse_args()
samples = {}
for line in args.log.read_text(errors="replace").splitlines():
    if not line.startswith(("[smgpc:control]", "[smgpc:gameplay]", "[smgpc:timing]")):
        continue
    fields = dict(re.findall(r"(\w+)=(\([^)]*\)|[^\s]+)", line))
    sample = samples.setdefault(int(fields["tick"]), {})
    for name, value in fields.items():
        if value.startswith("("):
            sample[name] = tuple(float(part) for part in value[1:-1].split(","))
        else:
            try:
                sample[name] = float(value)
            except ValueError:
                sample[name] = value

checks = []
def check(name, passed, **evidence):
    checks.append(dict(name=name, passed=bool(passed), **evidence))

records = [(tick, sample) for tick, sample in sorted(samples.items()) if "consumed_stick" in sample]
check("completed replay", bool(records) and records[-1][0] >= 960,
      last_tick=records[-1][0] if records else None)
nonfinite = []
for tick, sample in records:
    for name in ("walk_speed", "position", "mario_velocity", "eye", "watch", "up"):
        values = sample.get(name, ())
        if not isinstance(values, tuple):
            values = (values,)
        if any(not math.isfinite(value) for value in values):
            nonfinite.append((tick, name))
check("finite movement and camera", not nonfinite, invalid_samples=nonfinite[:10])

idle = [(tick, s) for tick, s in records if 100 <= tick <= 150]
idle_displacement = max((math.dist(idle[0][1]["position"], s["position"]) for _, s in idle), default=math.inf)
check("idle remains still on ground", len(idle) >= 20 and idle_displacement < 0.1 and
      all(s.get("mario_grounded") == 1 and s.get("jumping") == 0 and
          s.get("input_disable") == 0 and s.get("wasd") == (0, 0, 0, 0) for _, s in idle),
      observed_samples=len(idle), displacement=idle_displacement)

for first, last, name, keys, stick in (
    (180, 210, "W", (1, 0, 0, 0), (0, 1)),
    (270, 300, "A", (0, 1, 0, 0), (-1, 0)),
    (360, 390, "S", (0, 0, 1, 0), (0, -1)),
    (450, 480, "D", (0, 0, 0, 1), (1, 0)),
):
    active = [(tick, s) for tick, s in records if first <= tick <= last]
    displacement = math.dist(active[0][1]["position"], active[-1][1]["position"]) if active else 0
    # mWalkSpeed is the original normalized walk blend, not world units/tick.
    moving_on_ground = sum(s.get("mario_grounded") == 1 and
                           math.dist(s.get("mario_velocity", (0, 0, 0)), (0, 0, 0)) > 0.5
                           for _, s in active)
    check(name + " is accepted and moves Mario", len(active) >= 10 and displacement > 10 and
          moving_on_ground >= len(active) / 2 and
          all(s.get("wasd") == keys and s.get("consumed_stick") == stick and
              s.get("input_disable") == 0 for _, s in active),
          samples=len(active), moving_on_ground_samples=moving_on_ground, displacement=displacement)
    released = [s for tick, s in records if last + 30 <= tick <= last + 45]
    check(name + " releases", len(released) >= 5 and
          all(s.get("consumed_stick") == (0, 0) and s.get("wasd") == (0, 0, 0, 0) for s in released),
          samples=len(released))

for tick in (600, 780):
    takeoff = [(t, s) for t, s in records if tick <= t <= tick + 5 and s.get("jumping") == 1]
    landing = [(t, s) for t, s in records if tick + 10 <= t <= tick + 150 and
               s.get("jumping") == 0 and s.get("mario_grounded") == 1]
    check("jump at " + str(tick) + " takes off and lands", bool(takeoff) and bool(landing),
          takeoff_tick=takeoff[0][0] if takeoff else None,
          landing_tick=landing[0][0] if landing else None)

passed = all(item["passed"] for item in checks)
print(json.dumps(dict(passed=passed, synthetic_sdl_input=True, checks=checks), indent=2))
raise SystemExit(0 if passed else 1)
