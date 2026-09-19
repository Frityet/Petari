#!/usr/bin/env python3
"""Read-only post-frame matrix correlations, not a rendered-pixel verifier.

Mtx34 rows are stored verbatim. Their columns are local side/up/front in world
space. Joint 0 is the already calculated animation matrix, not an upload log.
An incomplete last JSONL record is excluded during live sampling; malformed
complete records fail. Output records retain state/ground flags for correlation.
"""
import argparse
import gzip
import hashlib
import json
import math
from pathlib import Path
import statistics


def valid(v, size):
    return (isinstance(v, (list, tuple)) and len(v) == size
            and all(isinstance(x, (float, int)) and math.isfinite(x) for x in v))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def norm(v):
    return math.sqrt(dot(v, v))


def unit(v):
    if not valid(v, 3) or norm(v) < 1e-12:
        return None
    return [x / norm(v) for x in v]


def angle(a, b):
    a, b = unit(a), unit(b)
    if a is None or b is None:
        return None
    return math.degrees(math.acos(max(-1., min(1., dot(a, b)))))


def negative(v):
    return [-x for x in v] if valid(v, 3) else None


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]]


def matrix_stats(m):
    if m is None:
        return {"present": False}
    finite = isinstance(m, list) and len(m) == 3 and all(valid(row, 4) for row in m)
    if not finite:
        return {"present": True, "finite": False}
    axes = [[row[c] for row in m] for c in range(3)]
    lengths = [norm(v) for v in axes]
    units = [unit(v) for v in axes]
    determinant = dot(axes[0], cross(axes[1], axes[2]))
    usable = all(u is not None for u in units)
    return {"present": True, "finite": True, "axis_lengths": lengths,
            "determinant": determinant,
            "normalized_determinant": determinant / math.prod(lengths) if usable else None,
            "max_normalized_axis_dot": max(abs(dot(units[i], units[j]))
                for i, j in ((0, 1), (0, 2), (1, 2))) if usable else None,
            "side": axes[0], "up": axes[1], "front": axes[2],
            "translation": [row[3] for row in m]}


def quaternion_matrix(q):
    if not valid(q, 4) or norm(q) < 1e-12:
        return None
    # Independent normalized quaternion reference, matching local-to-world axes.
    x, y, z, w = [v / norm(q) for v in q]
    return [[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w), 0.],
            [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w), 0.],
            [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y), 0.]]


def difference(a, b, columns=4):
    if not matrix_stats(a).get("finite") or not matrix_stats(b).get("finite"):
        return None
    return max(abs(a[r][c]-b[r][c]) for r in range(3) for c in range(columns))


def observe(frame, actor):
    player = actor.get("player")
    selected = (player.get("active_model") if player else actor.get("model")) or {}
    base = selected.get("base_matrix")
    joint = selected.get("joint0_animation_matrix")
    b, j = matrix_stats(base), matrix_stats(joint)
    result = {"frame": frame, "id": actor["id"], "type": actor["type"],
              "dead": actor["dead"], "hidden": actor["hidden"], "clipped": actor["clipped"],
              "position": actor["position"],
              "nerve": actor.get("nerve"), "base": b, "joint0": j,
              "joint0_base_rotation_max_abs_delta": difference(joint, base, 3)}
    for axis in ("up", "front", "side"):
        result[f"joint0_vs_base_{axis}_degrees"] = angle(j.get(axis), b.get(axis))
    if player:
        result.update({"status": player["status"], "state_type": player.get("state_type"),
            "on_ground": bool(player["movement_low_word"] & 0x40000000),
            "movement_low_word": player["movement_low_word"],
            "movement_high_word": player["movement_high_word"],
            "draw_word": player["draw_word"],
            "active_model_index": player["active_model_index"],
            "stick_magnitude": norm(player["stick_position"][:2]),
            "speed": norm(player["velocity"]),
            "posture_c4": matrix_stats(player.get("posture_matrix_c4")),
            "posture_f4": matrix_stats(player.get("posture_matrix_f4")),
            "model0_base_max_abs_delta": difference(base, (player.get("model0") or {}).get("base_matrix")),
            "actor_base_max_abs_delta": difference(base, player.get("actor_base_matrix")),
            "model_flags": {key: player.get(key) for key in ("bound_actor_934", "fixed_matrix_ea4",
                "fixed_matrix_ea5", "fixed_matrix_ea6", "model_update_requested_1c0", "model_update_skipped_1c1")}})
        for label, v in (("head", player.get("up")), ("direction_up", player.get("direction_up_1fc")),
                         ("movement_up", player.get("movement_up")),
                         ("inverse_gravity", negative(player.get("air_gravity"))),
                         ("ground_normal", (player.get("ground_triangle") or {}).get("normal"))):
            result[f"base_up_vs_{label}_degrees"] = angle(b.get("up"), v)
        result["base_front_vs_front_degrees"] = angle(b.get("front"), player.get("front"))
        result["base_side_vs_side_degrees"] = angle(b.get("side"), player.get("side"))
        result["ground_triangle"] = player.get("ground_triangle")
        result["head"] = player.get("up")
        result["camera_up_actor"] = player.get("camera_up_actor")
        result["camera_up_target"] = player.get("camera_up_target_300")
        result["camera_up_timer"] = player.get("camera_up_timer_330")
        result["camera_up_vs_head_degrees"] = angle(player.get("camera_up_actor"), player.get("up"))
        result["camera_up_vs_target_degrees"] = angle(player.get("camera_up_actor"), player.get("camera_up_target_300"))
        result["camera_target_vs_head_degrees"] = angle(player.get("camera_up_target_300"), player.get("up"))
        for field in ("safety", "warp", "recovery"):
            result[field] = player.get(field)
        warp, safety = player.get("warp") or {}, player.get("safety") or {}
        if warp.get("mode_45") == 3 and valid(warp.get("destination_14"), 3):
            residuals = {}
            for label, point_key, tri_key in (("latest", "latest_position_7d4", "latest_triangle_7e0"),
                                             ("previous", "previous_position_814", "previous_triangle_820")):
                point, normal = safety.get(point_key), (safety.get(tri_key) or {}).get("normal")
                if valid(point, 3) and valid(normal, 3):
                    expected = [p + n * 160. for p, n in zip(point, normal)]
                    residuals[label] = norm([a-b for a, b in zip(warp["destination_14"], expected)])
            result["warp_destination_vs_cached_safety_normal160_residual"] = residuals
    elif "demo_rabbit" in actor or "runaway_rabbit" in actor:
        is_demo = "demo_rabbit" in actor
        rabbit = actor["demo_rabbit" if is_demo else "runaway_rabbit"]
        q = ((actor.get("npc_pose") or {}).get("quaternion_a0") if is_demo
             else rabbit.get("pose_quaternion_a4"))
        qmatrix = quaternion_matrix(q)
        result.update({"quaternion": q, "quaternion_norm": norm(q) if valid(q, 4) else None,
            "quaternion_base_rotation_max_abs_delta": difference(qmatrix, base, 3),
            "actor_base_max_abs_delta": difference(base, rabbit.get("actor_base_matrix")),
            "base_up_vs_inverse_gravity_degrees": angle(b.get("up"), negative(actor.get("gravity"))),
            "base_front_vs_front_degrees": angle(b.get("front"), rabbit.get("front" if is_demo else "front_b4")),
            "authored_euler_equals_cached": actor.get("rotation") == (actor.get("npc_pose") or {}).get("cached_euler_cc")})
    return result


def statistic(rows, get):
    pairs = [(row, get(row)) for row in rows]
    pairs = [(r, v) for r, v in pairs if v is not None and math.isfinite(v)]
    if not pairs:
        return {"count": 0}
    worst, maximum = max(pairs, key=lambda pair: pair[1])
    return {"count": len(pairs), "min": min(v for _, v in pairs),
            "median": statistics.median(v for _, v in pairs), "max": maximum,
            "max_frame": worst["frame"], "max_actor_id": worst["id"]}


def summarize(rows):
    angle_keys = sorted({key for r in rows for key in r if key.endswith("_degrees")})
    return {"samples": len(rows), "angles": {key: statistic(rows, lambda r: r.get(key)) for key in angle_keys},
        "base": {key: statistic(rows, lambda r: r["base"].get(key)) for key in
                 ("determinant", "normalized_determinant", "max_normalized_axis_dot")},
        "joint0": {key: statistic(rows, lambda r: r["joint0"].get(key)) for key in
                   ("determinant", "normalized_determinant", "max_normalized_axis_dot")},
        "invalid_base_frames": [r["frame"] for r in rows if not r["base"].get("finite")],
        "invalid_joint0_frames": [r["frame"] for r in rows if not r["joint0"].get("finite")],
        "reflected_base_frames": [r["frame"] for r in rows if (r["base"].get("determinant") or 0) < 0],
        "reflected_joint0_frames": [r["frame"] for r in rows if (r["joint0"].get("determinant") or 0) < 0]}


def same_vector(a, b):
    return valid(a, 3) and valid(b, 3) and all(abs(x-y) <= 32 * 2**-23 for x, y in zip(a, b))


def camera_stable_intervals(players):
    # Sample equality only. No inference that an unobserved intervening frame
    # retained the same head, or that an update was scheduled in every frame.
    intervals = []
    previous = {}
    for row in players:
        prev = previous.get(row["id"])
        previous[row["id"]] = row
        if prev is None or row.get("camera_up_timer") is None or prev.get("camera_up_timer") is None:
            continue
        if same_vector(prev["head"], row["head"]):
            intervals.append({"from_frame": prev["frame"], "to_frame": row["frame"],
                "timer_before": prev["camera_up_timer"], "timer_after": row["camera_up_timer"],
                "status": row["status"], "state_type": row["state_type"],
                "camera_up_vs_head_before_degrees": prev["camera_up_vs_head_degrees"],
                "camera_up_vs_head_after_degrees": row["camera_up_vs_head_degrees"]})
    return intervals


def analyze(path, start_frame=0, end_frame=None):
    raw = path.read_bytes()
    data = gzip.decompress(raw) if path.suffix == ".gz" else raw
    end = data.rfind(b"\n") + 1
    records = [json.loads(line) for line in data[:end].splitlines()]
    selected_records = [r for r in records if r["frame_index"] >= start_frame
                        and (end_frame is None or r["frame_index"] <= end_frame)]
    rows = [observe(record["frame_index"], actor) for record in selected_records for actor in record["actors"]
            if "player" in actor or "demo_rabbit" in actor or "runaway_rabbit" in actor]
    players = [r for r in rows if "status" in r and not r["dead"]]
    moving = [r for r in players if r["status"] == 0 and r["on_ground"] and r["stick_magnitude"] > .1]
    unbound = [r for r in moving if not r["model_flags"]["bound_actor_934"]]
    state_groups = {}
    for row in players:
        key = f'{row["status"]}:{row["state_type"] or "no_active_state"}'
        state_groups.setdefault(key, []).append(row)
    rabbits = {}
    for actor_id in sorted({r["id"] for r in rows if "quaternion" in r}):
        selected = [r for r in rows if r["id"] == actor_id and not r["dead"] and not r["clipped"]]
        if not selected:
            continue
        rabbits[str(actor_id)] = summarize(selected)
        rabbits[str(actor_id)].update({"type": selected[0]["type"],
            "first_frame": selected[0]["frame"], "last_frame": selected[-1]["frame"],
            "unique_quaternions": len({tuple(r["quaternion"]) for r in selected if r["quaternion"]}),
            "unique_base_rotations": len({tuple(sum([r["base"].get(k, []) for k in ("side", "up", "front")], [])) for r in selected}),
            "quaternion_base_rotation_max_abs_delta": statistic(selected, lambda r: r.get("quaternion_base_rotation_max_abs_delta"))})
    summary = {"source": str(path), "source_file_sha256": hashlib.sha256(raw).hexdigest(),
        "complete_prefix_sha256": hashlib.sha256(data[:end]).hexdigest(),
        "ignored_incomplete_tail_bytes": len(data)-end,
        "last_frame": records[-1]["frame_index"] if records else None,
        "selection": {"start_frame": start_frame, "end_frame": end_frame,
            "first_selected_frame": selected_records[0]["frame_index"] if selected_records else None,
            "last_selected_frame": selected_records[-1]["frame_index"] if selected_records else None},
        "record_count": len(records), "player_all_alive": summarize(players),
        "player_grounded_moving_status0": summarize(moving),
        "player_grounded_unbound_moving_status0": summarize(unbound),
        "player_by_status_and_active_state": {key: summarize(group) for key, group in state_groups.items()},
        "player_status19_recovery_or_warp": summarize([r for r in players if r["status"] == 19]),
        "stable_head_snapshot_intervals": camera_stable_intervals(players),
        "alive_unclipped_rabbits": rabbits,
        "limitations": ["Post-frame stored matrices; no GX upload or pixel verification.",
            "Ground contact may be cached; movement_low_word on-ground flag retained separately.",
            "Joint0/base differences can be intentional animation; not automatically errors.",
            "Basis scale and orthogonality are reported separately; authored scale is not a defect."]}
    return summary, rows


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--samples", type=Path)
    parser.add_argument("--start-frame", type=int, default=0)
    parser.add_argument("--end-frame", type=int)
    args = parser.parse_args()
    summary, samples = analyze(args.trace, args.start_frame, args.end_frame)
    args.output.write_text(json.dumps(summary, indent=2, allow_nan=False) + "\n")
    if args.samples:
        payload = (json.dumps(samples, indent=2, allow_nan=False) + "\n").encode()
        args.samples.write_bytes(gzip.compress(payload, mtime=0) if args.samples.suffix == ".gz" else payload)
    print(json.dumps({"last_frame": summary["last_frame"], "records": summary["record_count"],
        "moving_samples": summary["player_grounded_moving_status0"]["samples"], "output": str(args.output)}))
