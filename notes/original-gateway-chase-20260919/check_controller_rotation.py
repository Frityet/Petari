"""Check controller tangent algebra against a rigid change of world coordinates."""
import copy
import json
from pathlib import Path
from follow_actor import controls

here = Path(__file__).resolve().parent
root = here.parents[1]
matrix = json.loads((root / "notes/original-rabbit-tower-chain-20260919/controller-waypoints.json").read_text())["zone_matrix_3x4"]
def transform(vector, point=False):
    return [sum(matrix[i][j] * vector[j] for j in range(3)) + (matrix[i][3] if point else 0) for i in range(3)]
checks = []
trace = root / "notes/gateway-compat-20260919/content-stomp-fixed-16000-actors.jsonl"
for line in trace.open():
    snapshot = json.loads(line)
    frame = snapshot["frame_index"]
    if not 2300 <= frame <= 3000 or frame % 50:
        continue
    player = next(a for a in snapshot["actors"] if "player" in a)
    guide = next(a for a in snapshot["actors"] if a["id"] == 846)
    expected = controls(player, guide)
    rotated = copy.deepcopy(player)
    rotated["position"] = transform(player["position"], True)
    for key in ("movement_up", "camera_x", "camera_y", "camera_z"):
        rotated["player"][key] = transform(player["player"][key])
    actual = controls(rotated, {"position": transform(guide["position"], True)})
    errors = [abs(a - b) for a, b in zip(actual, expected)]
    assert max(errors[:2]) < 1e-5 and errors[2] < 0.001, (frame, expected, actual)
    checks.append({"frame": frame, "expected": expected, "rotated": actual, "absolute_errors": errors})
assert len(checks) == 15
(here / "controller-rotation-check.json").write_text(json.dumps({"scope": "Offline diagnostic-controller algebra only; no gameplay or production validation", "passed": len(checks), "checks": checks}, indent=2) + "\n")
print(f"PASS {len(checks)} captured camera/target samples under the actual zone rigid transform")
