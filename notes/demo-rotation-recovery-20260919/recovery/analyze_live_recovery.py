"""Read existing original-process trace; write sampled recovery evidence only."""
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BASE = Path(__file__).resolve().parent
TRACE = ROOT / 'notes/demo-rotation-recovery-20260919/after-actors.jsonl'
REPORT = ROOT / 'notes/demo-rotation-recovery-20260919/after-placements.json'
prior = ROOT / 'notes/demo-system-verification-20260919/scene/analyze_pullback.py'
namespace = {'__file__': str(prior)}
source = prior.read_text().split('transitions, selected, previous =', 1)[0]
source = source.replace('notes/demo-system-verification-20260919/final2-route-placements.json',
                        str(REPORT.relative_to(ROOT)))
exec(source, namespace)
projection = namespace['projection']

def distance(a, b):
    return math.sqrt(sum((x-y)**2 for x,y in zip(a,b)))

def sample(frame, actor):
    player = actor['player']
    result = {'frame': frame['frame_index'], 'actor_position': actor['position'],
              'player_position': player['position'], 'status': player['status'],
              'state_type': player['state_type'], 'controller': frame['controller'],
              'area': projection(player['position']), 'up': player['up'],
              'direction_up': player['direction_up_1fc'], 'gravity': player['air_gravity'],
              'bound_actor_934': player['bound_actor_934'],
              'movement_low_word': player['movement_low_word'],
              'on_ground_flag': bool(player['movement_low_word'] & 0x40000000),
              'safety': player.get('safety'), 'warp': player.get('warp'),
              'recovery': player.get('recovery'), 'ground_triangle': player.get('ground_triangle')}
    if result['warp'] and result['safety']:
        target = result['warp']['destination_14']
        candidates = {}
        for name,pos,tri in [('latest','latest_position_7d4','latest_triangle_7e0'),
                             ('previous','previous_position_814','previous_triangle_820')]:
            triangle = result['safety'].get(tri)
            if triangle:
                point = result['safety'][pos]
                normal = triangle['normal']
                computed = [p + n*160 for p,n in zip(point,normal)]
                candidates[name] = {'saved_plus_cached_normal_160': computed,
                                    'target_residual': distance(computed,target),
                                    'normal_length': distance(normal,[0,0,0]),
                                    'triangle_host': triangle['host_id'],
                                    'triangle_prism': triangle['prism_index']}
        result['target_candidates'] = candidates
    return result

prefix = hashlib.sha256()
size = 0
samples = []
with TRACE.open('rb') as trace:
    for line in trace:
        try:
            frame = json.loads(line)
        except ValueError:
            break
        prefix.update(line)
        size += len(line)
        actor = next((a for a in frame.get('actors',[]) if 'player' in a),None)
        if actor:
            samples.append(sample(frame,actor))

events = []
active = None
previous = None
for current in samples:
    warp = current['warp']
    is_recovery = bool(warp and warp['mode_45'] == 3)
    if is_recovery:
        if active is None:
            active = {'preceding_sample': previous, 'entry_sample': current,
                      'samples': [], 'minimum_sampled_target_distance': float('inf')}
            events.append(active)
        target = active['entry_sample']['warp']['destination_14']
        active['samples'].append(current)
        active['last_warp_sample'] = current
        active['minimum_sampled_target_distance'] = min(active['minimum_sampled_target_distance'],
                                                        distance(current['player_position'],target))
    elif active is not None:
        active['first_sample_after_warp'] = current
        target = active['entry_sample']['warp']['destination_14']
        active['first_after_target_distance'] = distance(current['player_position'],target)
        active['last_warp_target_distance'] = distance(active['last_warp_sample']['player_position'],target)
        active['sampled_duration'] = current['frame']-active['entry_sample']['frame']
        active = None
    previous = current

for event in events:
    after = event.get('first_sample_after_warp')
    if after:
        grounded = next((s for s in samples if s['frame'] >= after['frame']
                         and not s['bound_actor_934'] and s['on_ground_flag'] and not s['warp']),None)
        event['first_ground_flag_sample'] = grounded
        if grounded:
            event['first_grounded_target_distance'] = distance(grounded['player_position'],
                                                               event['entry_sample']['warp']['destination_14'])

result = {'scope': 'Read-only sampled mode3 MarioWarp recovery observation; no input or gameplay writes.',
          'trace': str(TRACE.relative_to(ROOT)), 'trace_prefix_bytes': size,
          'trace_prefix_sha256': prefix.hexdigest(), 'last_frame': samples[-1]['frame'] if samples else None,
          'last_sample': samples[-1] if samples else None,
          'volume': {key:namespace[key] for key in ['origin','up','radius','height','placement']},
          'events': events, 'event_count': len(events),
          'completed_count': sum('first_sample_after_warp' in e for e in events),
          'source_sha256': {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                            for p in [REPORT,prior,Path(__file__),ROOT/'src/Game/Player/MarioWarp.cpp',
                                      ROOT/'src/Game/Player/MarioRecovery.cpp',ROOT/'src/Game/Player/MarioCollision.cpp',
                                      ROOT/'src/Game/Player/MarioAccess.cpp',ROOT/'src/Game/Player/Mario.hpp',
                                      ROOT/'aurora/include/aurora/ppc_bitfield.hpp']},
          'limits': ['Every10-frame sampling does not establish exact trigger/close frame or final landing.',
                     'Cached triangle normals are observed, not recalculated by this diagnostic.',
                     'Area reconstructed from actual zone matrix and original formulas, not live AreaForm fields.',
                     'Warp close resumes original falling; distance at first post-warp sample includes intervening movement.',
                     'Grounded uses original MovementStates::_1 mask0x40000000 only when actor-bound934 is false, following MarioAccess::isOnGround; cached triangles do not establish grounding.',
                     'No retail pose capture or broad collision correctness claim.']}
(BASE/'after-recovery-observation.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'last_frame':result['last_frame'],'event_count':len(events),
                  'completed_count':result['completed_count'],
                  'last_position':samples[-1]['player_position'] if samples else None,
                  'entries':[{'frame':e['entry_sample']['frame'],'inside':e['entry_sample']['area']['inside'],
                              'targets':e['entry_sample'].get('target_candidates'),
                              'after_frame':e.get('first_sample_after_warp',{}).get('frame'),
                              'last_warp_distance':e.get('last_warp_target_distance'),
                              'after_distance':e.get('first_after_target_distance'),
                              'first_ground_flag_frame':e.get('first_ground_flag_sample',{}).get('frame')} for e in events]},indent=2))
