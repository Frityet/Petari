#!/usr/bin/env python3
"""Offline authored-volume/trace comparison; writes notes only, never game input."""
import ast
import hashlib
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
source = ROOT / 'notes/original-rabbit-tower-chain-20260919/compute_waypoints.py'
# Reuse the recorded approximate float32 placement math, without executing its
# module-level report write or reading any live process state.
f = lambda x: struct.unpack('f', struct.pack('f', x))[0]
for node in ast.parse(source.read_text()).body:
    if isinstance(node, ast.FunctionDef):
        exec(compile(ast.Module(body=[node], type_ignores=[]), str(source), 'exec'))

trace_path = ROOT / 'notes/demo-system-verification-20260919/final2-route-actors.jsonl'
selected = []
for line in trace_path.read_bytes().splitlines(keepends=True):
    try:
        row = json.loads(line)
    except ValueError:
        continue
    if 7000 <= row['frame_index'] <= 10000:
        selected.append((line, row))
rows = [r for _, r in selected]
placements_path = ROOT / 'notes/demo-system-verification-20260919/final2-route-placements.json'
placements = json.loads(placements_path.read_text())['entries']
authored_path = ROOT / 'notes/gateway-content-inventory-20260919/source-placement-inventory.json'
authored = json.loads(authored_path.read_text())['rows']
metadata_path = ROOT / 'notes/original-rabbit-tower-chain-20260919/zone-metadata.json'
metadata = json.loads(metadata_path.read_text())['tables']

def actor(row, identity):
    return next(a for a in row['actors'] if a['id'] == identity)

def world(matrix, local):
    return [sum(matrix[i*4+j]*local[j] for j in range(3))+matrix[i*4+3] for i in range(3)]

blockers = []
for a in authored:
    if a['name'] != 'CollisionBlocker':
        continue
    p = next(p for p in placements if p['object'] == a['name'] and p['zone_name'] == a['zone']
             and p['l_id'] == a['l_id'] and p['table_path'] == a['table'])
    raw = a['raw_row']
    center = world(p['zone_placement_matrix'], vec(raw, 'pos'))
    blockers.append({'zone': a['zone'], 'l_id': a['l_id'], 'world_center': center,
                     'sensor_radius': 50*raw['scale_y'], 'scale': vec(raw, 'scale'),
                     'SW_APPEAR': raw['SW_APPEAR'], 'SW_B': raw['SW_B'],
                     'closest_trace_samples': {
                         name: min((math.dist(actor(row, identity)['position'], center), row['frame_index']) for row in rows)
                         for name, identity in [('mario', 339), ('rabbit', 916)]}})

cylinder = next(r for entries in metadata.values() for r in entries if r.get('name') == 'PullBackCylinder')
p = next(p for p in placements if p['object'] == 'PullBackCylinder' and p['zone'] == 5)
m = p['zone_placement_matrix']
zm = [m[i*4:i*4+3] for i in range(3)]
origin = point(zm, vec(cylinder, 'pos'), [m[i*4+3] for i in range(3)])
local_rotation, _ = rotate(vec(cylinder, 'dir'), True)
euler_world = euler(mul(zm, local_rotation))
# AreaFormCylinder::calcDir uses tmpMtxRot{X,Y,Z}Deg, whose sine
# signs are opposite makeMtxRotate. These helpers use sinDegree/cosDegree,
# not sinShort: negative degree indexing must truncate absolute magnitude.
sc = []
for angle in euler_world:
    index = int(f(abs(angle)*f(45.511112))) & 16383
    radians = index*6.2831854820251465/16384
    sc.append((f((-1 if angle < 0 else 1)*math.sin(radians)), f(math.cos(radians))))
(sx, cx), (sy, cy), (sz, cz) = sc
up = [f(f(f(cz*sy)*sx)+f(sz*cx)), f(-f(f(sz*sy)*sx)+f(cz*cx)), f(-cy*sx)]
length = math.sqrt(sum(t*t for t in up))
up = [f(t/length) for t in up]
inside = []
for row in rows:
    a = actor(row, 339)
    displacement = [a['player']['position'][i]-origin[i] for i in range(3)]
    height = sum(displacement[i]*up[i] for i in range(3))
    radius = math.sqrt(sum((displacement[i]-height*up[i])**2 for i in range(3)))
    if 0 <= height <= 500*cylinder['scale_y'] and radius < 500*cylinder['scale_x']:
        inside.append({'frame': row['frame_index'], 'axis_height': height,
                       'axis_distance': radius, 'player_status': a['player']['status']})
result = {
    'scope': 'Read-only offline analysis of completed trace samples7000-10000; no runtime query or input write.',
    'sample_count': len(rows), 'selected_trace_bytes_sha256': hashlib.sha256(b''.join(l for l, _ in selected)).hexdigest(),
    'input_sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in [placements_path, authored_path, metadata_path, source]},
    'blockers': blockers,
    'pull_back_cylinder': {'raw_row': cylinder, 'world_origin': origin, 'world_euler_degrees': euler_world,
                           'approx_world_up': up, 'radius': 500*cylinder['scale_x'], 'height': 500*cylinder['scale_y'],
                           'inside_samples': inside},
    'limitations': ['Offline float32/trigonometric reconstruction is approximate, not a captured AreaForm matrix or branch execution.',
                    'Trace samples every10frames and records post-frame state. Containment correlates with status19 but does not capture the exact entry call.',
                    'Mario ground_triangle is the stored mGroundPolygon, not proof of current-frame contact during recovery.',
                    'Trace omits the actor record for contact host722; its exact authored identity is not asserted here.',
                    'All8 blocker scales are uniform. Current native scale.y versus canonical scale.x source drift cannot affect these placements.']}
(NOTES/'final2-crater-analysis.json').write_text(json.dumps(result, indent=2)+'\n')
print('frames', len(rows), 'inside PullBackCylinder', len(inside), 'statuses', sorted({r['player_status'] for r in inside}))
