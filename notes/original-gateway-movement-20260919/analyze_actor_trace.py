"""Summarize recorded observations; does not treat frame count as story completion."""
import argparse
import json
import math
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('trace', type=Path)
parser.add_argument('output', type=Path)
options = parser.parse_args()
rows = [json.loads(line) for line in options.trace.read_text().splitlines()]
actors = {}
controls = []
for row in rows:
    frame = row['frame_index']
    control = row.get('controller')
    if control:
        signature = (control.get('hold'), tuple(control.get('stick', [])))
        if not controls or controls[-1]['signature'] != signature:
            controls.append({'first_observed_frame': frame, 'last_observed_frame': frame,
                             'signature': signature, 'controller': control})
        else:
            controls[-1]['last_observed_frame'] = frame
    for actor in row['actors']:
        state = actors.setdefault(actor['id'], {'id': actor['id'], 'type': actor['type'],
            'name': actor.get('name'), 'samples': 0, 'transitions': [],
            'first_frame': frame, 'first_position': actor['position'],
            'sampled_path_distance': 0.0, 'nonfinite_position_samples': 0,
            'stick_segments': []})
        state['samples'] += 1
        if not all(math.isfinite(v) for v in actor['position']):
            state['nonfinite_position_samples'] += 1
        if 'last_position' in state:
            state['sampled_path_distance'] += math.dist(state['last_position'], actor['position'])
        state['last_frame'] = frame
        state['last_position'] = actor['position']
        signature = (actor['dead'], actor['hidden'], actor['clipped'],
                     (actor.get('nerve') or {}).get('type'))
        if not state['transitions'] or state['transitions'][-1]['signature'] != signature:
            state['transitions'].append({'frame': frame, 'signature': signature,
                                         'nerve': actor.get('nerve'), 'position': actor['position']})
        if player := actor.get('player'):
            stick = (control or {}).get('stick', [0, 0])
            if stick != [0, 0]:
                segments = state['stick_segments']
                if not segments or segments[-1]['controller_span'] != len(controls) - 1:
                    segments.append({'stick': stick, 'first_frame': frame,
                        'controller_span': len(controls) - 1,
                        'first_position': actor['position'], 'first_player_stick': player['stick_position']})
                segment = segments[-1]
                segment['last_frame'] = frame
                segment['last_position'] = actor['position']
                segment['last_player_stick'] = player['stick_position']
                segment['last_world_pad_direction'] = player['world_pad_direction']
                segment['net_distance'] = math.dist(segment['first_position'], segment['last_position'])
                segment['last_status'] = player['status']
result = {'source': str(options.trace), 'observed_frames': [rows[0]['frame_index'], rows[-1]['frame_index']],
          'records': len(rows), 'note': 'Sampled observations only; no completion or input between samples is inferred.',
          'controller_spans': controls, 'actors': list(actors.values())}
options.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n')
print(f"{len(rows)} records; {len(actors)} original actors; wrote {options.output}")
