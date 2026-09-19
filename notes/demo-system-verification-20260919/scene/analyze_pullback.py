"""Read-only original PullBackCylinder projection of a frozen trace prefix.

Writes evidence only. No game process, controller, source, or build access.
"""
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BASE = Path(__file__).resolve().parent
helper = ROOT / "notes/original-rabbit-tower-chain-20260919/compute_waypoints.py"
namespace = {"__file__": str(helper)}
# Reuse only numerical function definitions; exclude the old helper's output writes.
exec(helper.read_text().split("rawgalaxy=", 1)[0], namespace)
f, rotate, mul, euler, point = [namespace[n] for n in ("f", "rotate", "mul", "euler", "point")]
metadata_path = ROOT / "notes/original-rabbit-tower-chain-20260919/zone-metadata.json"
report_path = ROOT / "notes/demo-system-verification-20260919/final2-route-placements.json"
trace_path = ROOT / "notes/demo-system-verification-20260919/final2-route-actors.jsonl"
metadata = json.loads(metadata_path.read_text())
rows = [(path, row) for path, entries in metadata["tables"].items()
        if "/common/" in path or "/layera/" in path for row in entries
        if row.get("name") in ("PullBackCube", "PullBackCylinder")]
assert len(rows) == 1 and rows[0][1]["name"] == "PullBackCylinder"
table_path, row = rows[0]
placement = next(x for x in json.loads(report_path.read_text())["entries"]
                 if x["zone"] == 5 and x["object"] == "PullBackCylinder")
assert placement["table_path"] == table_path and placement["row"] == row["row"]
m = placement["zone_placement_matrix"]
zone, translation = [m[0:3], m[4:7], m[8:11]], m[3::4]
origin = point(zone, [row["pos_"+v] for v in "xyz"], translation)
local, indices = rotate([row["dir_"+v] for v in "xyz"], True)
world_euler = euler(mul(zone, local))
# Original tmpMtxRot*Deg uses inverse axis signs, unlike tmpMtxRot*Rad.
# Degree trig quantizes absolute angle with JMath's 16384-entry table.
values, degree_indices = [], []
for degrees in world_euler:
    index = int(f(abs(degrees) * f(45.511112))) & 16383
    angle = index * 6.2831854820251465 / 16384
    sine, cosine = f(math.sin(angle)), f(math.cos(angle))
    values.append((-sine if degrees < 0 else sine, cosine))
    degree_indices.append(index)
(sx, cx), (sy, cy), (sz, cz) = values
up = [f(f(cz*f(sy*sx))+f(sz*cx)), f(f(-sz*f(sy*sx))+f(cz*cx)), f(-cy*sx)]
length = math.sqrt(sum(v*v for v in up))
up = [f(v/length) for v in up]
radius, height = f(f(row["scale_x"])*500), f(f(row["scale_y"])*500)
def projection(position):
    delta = [position[i]-origin[i] for i in range(3)]
    axial = sum(delta[i]*up[i] for i in range(3))
    radial = math.sqrt(sum((delta[i]-axial*up[i])**2 for i in range(3)))
    return {"position": position, "axial": axial, "radial": radial,
            "inside": 0 <= axial <= height and radial < radius}
transitions, selected, previous = [], [], None
prefix = bytearray()
with trace_path.open("rb") as trace:
    for line in trace:
        try:
            frame = json.loads(line)
        except ValueError:
            continue
        if frame["frame_index"] > 11000:
            break
        prefix.extend(line)
        actor = next((a for a in frame.get("actors", []) if "player" in a), None)
        if actor is None:
            continue
        player = actor["player"]
        current = {"frame": frame["frame_index"], "actor_id": actor["id"],
                   "status": player["status"], **projection(player["position"])}
        if previous and 5000 <= current["frame"] and current["status"] == 19 and previous["status"] != 19:
            transitions.append({"before": previous, "first_status19": current})
        if 9840 <= current["frame"] <= 10290:
            selected.append(current)
        previous = current
archive = ROOT / "build/demo-system-verification-20260919/HeavensDoorMysteriousZone.arc"
assert hashlib.sha256(archive.read_bytes()).hexdigest() == metadata["archive_sha256"]
source_paths = [metadata_path, report_path, helper, archive]
result = {
    "scope": "Original source/data reconstruction and sampled runtime correlation, not a live AreaForm field capture or retail-console differential run.",
    "raw_area_row": row, "actual_placement_report_entry": placement,
    "world_origin": origin, "world_euler_degrees": world_euler,
    "local_short_indices": indices, "degree_table_indices": degree_indices,
    "world_up": up, "radius": radius, "height": height,
    "containment": "0 <= dot(position-origin,up) <= height; length(delta-up*dot(delta,up)) < radius",
    "numeric_limit": "Float32 intermediate model with double dot sums/libm table reconstruction; not bit-exact. No threshold changed. Closest first-status19 axial margin is about0.133units.",
    "reported_crater": projection([13739,-10139,7740]),
    "reported_rabbit_ground": projection([12820,-10770,7545]),
    "trace_path": str(trace_path.relative_to(ROOT)), "trace_prefix_last_frame": previous["frame"],
    "trace_prefix_byte_count": len(prefix), "trace_prefix_sha256": hashlib.sha256(prefix).hexdigest(),
    "source_sha256": {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths},
    "status19_transition_frame_range": [5000,11000],
    "transition_count": len(transitions),
    "all_first_status19_samples_inside": all(t["first_status19"]["inside"] for t in transitions),
    "all_preceding_samples_outside": all(not t["before"]["inside"] for t in transitions),
    "transitions": transitions, "sidestep_samples": selected,
    "limits": ["Status19 is shared by Recovery and Warp; trace does not identify concrete Mario state.",
               "Sampling every10frames brackets activation but does not record the exact query/call frame.",
               "No debug query, actor manipulation, controller input or gameplay state write performed."]}
(BASE / "pullback-final2.json").write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n")
print(json.dumps({k:result[k] for k in ("world_origin","world_up","transition_count","all_first_status19_samples_inside","all_preceding_samples_outside")}, indent=2))
