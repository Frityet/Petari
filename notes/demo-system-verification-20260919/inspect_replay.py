"""Summarize read-only sampled original-process traces; flags are not pixel proof."""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("trace", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
events, seen, previous = [], set(), {}
features, statuses, contacts = collections.Counter(), collections.Counter(), collections.Counter()
invalid = []
gravity_transitions, field_transitions = [], []
last_gravity, last_fields = {}, {}
player_ground_contacts = collections.Counter()
samples = actors = 0
first = last = None

def event(kind, frame, actor):
    key = kind, actor["id"]
    if key in seen:
        return
    seen.add(key)
    events.append({"kind": kind, "frame": frame, "actor_id": actor["id"],
                   "actor_type": actor["type"], "name": actor.get("name"), "position": actor["position"],
                   "nerve": actor.get("nerve"), "clipped": actor["clipped"]})

def finite_vector(value, frame, aid, field):
    valid = isinstance(value, list) and len(value) == 3 and all(
        isinstance(x, (float, int)) and math.isfinite(x) for x in value)
    if not valid and len(invalid) < 30:
        invalid.append({"frame": frame, "actor_id": aid, "field": field, "value": value})

with args.trace.open() as source:
    for line in source:
        if not line.endswith("\n"):
            break
        record = json.loads(line)
        frame = record["frame_index"]
        first = frame if first is None else first
        last = frame
        samples += 1
        for actor in record["actors"]:
            actors += 1
            aid, typ = actor["id"], actor["type"]
            nerve = (actor.get("nerve") or {}).get("type", "")
            for field in ("position", "velocity", "gravity"):
                finite_vector(actor[field], frame, aid, field)
            player = actor.get("player")
            if player:
                statuses[player["status"]] += 1
                for field in ("position", "velocity", "velocity_after", "up", "air_gravity"):
                    finite_vector(player[field], frame, aid, "player." + field)
                info = player.get("gravity_info")
                if info:
                    identity = {k: v for k, v in info.items() if k != "vector"}
                    if identity != last_gravity.get(aid):
                        gravity_transitions.append({"frame": frame, "actor_id": aid, **identity})
                        last_gravity[aid] = identity
                    finite_vector(info["vector"], frame, aid, "player.gravity_info.vector")
                if triangle := player.get("ground_triangle"):
                    player_ground_contacts[triangle["host_id"]] += 1
                    finite_vector(triangle["normal"], frame, aid, "player.ground_triangle.normal")
                    for vertex in triangle["vertices"]:
                        finite_vector(vertex, frame, aid, "player.ground_triangle.vertex")
            if field := actor.get("gravity_field"):
                if field != last_fields.get(aid):
                    field_transitions.append({"frame": frame, "actor_id": aid, "position": actor["position"], **field})
                    last_fields[aid] = field
            if binder := actor.get("binder"):
                finite_vector(binder["fix_reaction"], frame, aid, "binder.fix_reaction")
                for kind in ("ground", "wall", "roof"):
                    if hit := binder[kind]:
                        contacts[kind] += 1
                        features[hit["feature"]] += 1
                        for field in ("hit_position", "unknown_70", "moving_reaction"):
                            finite_vector(hit[field], frame, aid, "binder." + kind + "." + field)
            if typ == "13RunawayRabbit" and "NrvCaughtE" in nerve:
                event("rabbit_caught", frame, actor)
            if typ == "11RunawayTico":
                for state in ("Talk", "WhiteOut", "WhiteIn"):
                    if "Nrv" + state + "E" in nerve:
                        event("tico_" + state, frame, actor)
            if "HeavensDoorDemoObj" in typ and "Appear" in nerve and not actor["dead"] and not actor["hidden"]:
                event("demo_appearance_actor_alive_not_hidden", frame, actor)
            if typ == "7Rosetta" and not actor["dead"] and not actor["hidden"]:
                event("rosetta_alive_not_hidden", frame, actor)
            if typ == "12SimpleMapObj" and actor.get("name") == "HeavensDoorStepAfterDemo" and not actor["dead"]:
                event("after_demo_steps_alive", frame, actor)
            if typ == "11EarthenPipe" and previous.get(aid) != nerve:
                for state in ("PlayerIn", "PlayerOut"):
                    if "Nrv" + state + "E" in nerve:
                        event("pipe_" + state, frame, actor)
            previous[aid] = nerve

result = {"trace": str(args.trace), "sha256": hashlib.file_digest(args.trace.open("rb"), "sha256").hexdigest(),
          "scope": "Sampled trace only. No retail parity claim; live/unhidden actor flags are not proof of rendered visibility.",
          "first_frame": first, "last_frame": last, "samples": samples, "actor_samples": actors,
          "invalid_sampled_vectors": invalid, "player_status_sample_counts": statuses,
          "binder_contact_sample_counts": contacts, "binder_feature_sample_counts": features,
          "player_cached_ground_triangle_host_sample_counts": player_ground_contacts,
          "player_gravity_transitions": gravity_transitions,
          "gravity_field_transitions": field_transitions,
          "events": events}
args.output.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
