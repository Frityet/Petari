"""Summarize observed native Mario control samples without asserting gameplay success."""
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
    tick = int(fields["tick"])
    sample = samples.setdefault(tick, {})
    for name, value in fields.items():
        if value.startswith("("):
            sample[name] = [float(part) if math.isfinite(float(part)) else part
                            for part in value[1:-1].split(",")]
        else:
            try:
                number = float(value)
                sample[name] = number if math.isfinite(number) else value
            except ValueError:
                sample[name] = value

groups = []
for tick, sample in sorted(samples.items()):
    state = tuple(sample.get("wasd", ()))
    if not groups or groups[-1]["wasd"] != list(state):
        groups.append({"wasd": list(state), "samples": []})
    groups[-1]["samples"].append(sample)

result = []
for group in groups:
    observations = group.pop("samples")
    first, last = observations[0], observations[-1]
    group.update({
        "first_tick": first["tick"], "last_tick": last["tick"],
        "sample_count": len(observations),
        "input_disabled_samples": sum(s.get("input_disable") == 1 for s in observations),
        "accepted_stick_samples": sum(any(s.get("consumed_stick", ())) for s in observations),
        "jumping_samples": sum(s.get("jumping") == 1 for s in observations),
        "mario_grounded_samples": sum(s.get("mario_grounded") == 1 for s in observations),
        "nonfinite_walk_speed_samples": sum(isinstance(s.get("walk_speed"), str) for s in observations),
        "first": first, "last": last,
    })
    if "position" in first and "position" in last and all(
        isinstance(value, float) for value in first["position"] + last["position"]
    ):
        group["position_displacement"] = math.dist(first["position"], last["position"])
    result.append(group)
print(json.dumps({"log": str(args.log), "groups": result}, indent=2, ensure_ascii=False, allow_nan=False))
