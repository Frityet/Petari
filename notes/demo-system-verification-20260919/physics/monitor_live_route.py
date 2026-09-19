"""Read-only append trace observer; writes its own evidence log, never input."""
import json
import bisect
import math
from pathlib import Path
import sys
import time

root = Path(__file__).resolve().parent.parent
prefix = sys.argv[1]
trace_path = root / (prefix + '-route-actors.jsonl')
operator_path = root / (prefix + '-operator.jsonl')
result_path = root / (prefix + '-route.json')
output_path = root / 'physics' / (prefix + '-live-events.jsonl')
last = {}
positions = []
next_report = 0
operator_rows = []
operator_frames = []
seen_jumps = set()

def lines(source):
    while True:
        offset = source.tell()
        line = source.readline()
        if not line.endswith('\n'):
            source.seek(offset)
            return
        yield json.loads(line)

def emit(value):
    line = json.dumps(value, ensure_ascii=False)
    output.write(line + '\n')
    output.flush()
    print(line, flush=True)

with trace_path.open() as trace, operator_path.open() as commands, output_path.open('w') as output:
    while True:
        for command in lines(commands):
            operator_rows.append(command)
            operator_frames.append(command["frame"])
            jump = command.get('automatic_jump')
            if jump and jump['first_frame'] not in seen_jumps:
                seen_jumps.add(jump['first_frame'])
                emit({'event': 'driver_jump', 'frame': command['frame'], 'jump': jump})
        for sample in lines(trace):
            frame = sample['frame_index']
            mario = next((a for a in sample['actors'] if 'player' in a), None)
            if mario:
                gravity = mario['player'].get('gravity_info', {}).get('field_actor_id')
                if last.get('gravity') != gravity:
                    emit({'event': 'selected_gravity', 'frame': frame, 'owner': gravity})
                    last['gravity'] = gravity
                positions.append((frame, mario['position']))
                positions = [(f, p) for f, p in positions if f >= frame - 1000]
            for actor in sample['actors']:
                kind = actor['type']
                if kind == '16GlobalGravityObj' and actor.get('gravity_field', {}).get('type') == '15ParallelGravity':
                    current = (actor['dead'], actor['gravity_field']['appeared'], actor['gravity_field']['activated'])
                elif kind in ('13RunawayRabbit', '11RunawayTico', '7Rosetta', '20RunawayRabbitCollect',
                              '11EarthenPipe', '18HeavensDoorDemoObj'):
                    current = ((actor.get('nerve') or {}).get('type'), actor['dead'], actor.get('hidden'))
                else:
                    continue
                if last.get(actor['id']) != current:
                    emit({'event': 'actor_transition', 'frame': frame, 'id': actor['id'], 'type': kind,
                          'state': current})
                    last[actor['id']] = current
            if mario and frame >= next_report:
                next_report = frame + 1000
                operator_index = bisect.bisect_right(operator_frames, frame) - 1
                operator = operator_rows[operator_index] if operator_index >= 0 else None
                extent = [max(p[i] for _, p in positions) - min(p[i] for _, p in positions) for i in range(3)]
                emit({'event': 'progress', 'frame': frame, 'position': mario['position'],
                      'window_first_frame': positions[0][0], 'bounds_diagonal': math.sqrt(sum(x*x for x in extent)),
                      'status': mario['player']['status'], 'processed_magnitude': mario['player']['stick_position'][2],
                      'operator': None if operator is None else {k: operator.get(k) for k in
                       ('frame', 'phase', 'route_index', 'completed_catches', 'distance', 'finished_reason')}})
        if result_path.exists():
            result = json.loads(result_path.read_text())
            if 'exit_code' in result:
                emit({'event': 'process_complete', 'result': result})
                break
        time.sleep(0.25)
