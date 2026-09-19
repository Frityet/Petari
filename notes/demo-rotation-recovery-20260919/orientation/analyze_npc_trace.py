#!/usr/bin/env python3
"""Read-only basis evidence from the original-process actor trace."""
import argparse
import hashlib
import json
import math
from collections import Counter, defaultdict
from pathlib import Path


def cosine(a, b):
    scale = math.sqrt(sum(x * x for x in a) * sum(x * x for x in b))
    return sum(x * y for x, y in zip(a, b)) / scale if scale else None


def bounds(values):
    values = [value for value in values if value is not None]
    return [min(values), max(values)] if values else []


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--start", type=int, default=2000)
    parser.add_argument("--end", type=int, default=2**63 - 1)
    args = parser.parse_args()
    by_actor = defaultdict(list)
    captured = args.trace.read_bytes()
    # A live writer can append after this read. Record the precise complete-line
    # prefix analyzed so it can be verified against the final trace later.
    captured = captured[:captured.rfind(b"\n") + 1]
    for line in captured.splitlines():
        frame = json.loads(line)
        if not args.start <= frame["frame_index"] <= args.end:
            continue
        for actor in frame["actors"]:
            if actor.get("type") != "10DemoRabbit" or actor["dead"] or actor["clipped"] or actor["hidden"]:
                continue
            matrix = actor["model"]["base_matrix"]
            by_actor[actor["id"]].append({
                "frame": frame["frame_index"],
                "nerve": actor["nerve"]["type"],
                "front_alignment": cosine([row[2] for row in matrix], actor["demo_rabbit"]["front"]),
                "gravity_up_alignment": cosine([row[1] for row in matrix], [-x for x in actor["gravity"]]),
                "quaternion": actor["npc_pose"]["quaternion_a0"],
                "rotation": actor["rotation"],
            })
    result = {
        "trace": str(args.trace),
        "trace_prefix_bytes": len(captured),
        "trace_prefix_sha256": hashlib.sha256(captured).hexdigest(),
        "requested_frame_range": [args.start, args.end],
        "scope": "Observed live, visible, unclipped DemoRabbit bases; alignment can lag changing targets by original smoothing.",
        "actors": {},
    }
    for identity, rows in by_actor.items():
        result["actors"][identity] = {
            "sample_count": len(rows),
            "frame_range": [rows[0]["frame"], rows[-1]["frame"]],
            "unique_quaternions": len({tuple(row["quaternion"]) for row in rows}),
            "unique_euler_rotations": len({tuple(row["rotation"]) for row in rows}),
            "front_alignment_range": bounds(row["front_alignment"] for row in rows),
            "gravity_up_alignment_range": bounds(row["gravity_up_alignment"] for row in rows),
            "nerve_samples": dict(Counter(row["nerve"] for row in rows)),
            "frame2300": next((row for row in rows if row["frame"] == 2300), None),
        }
    args.output.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
