"""Read the newest complete actor snapshot without reading or modifying game memory."""
from pathlib import Path
import json, math, sys
notes=Path(__file__).resolve().parent
path=notes/'route-actors.jsonl'
with path.open('rb') as f:
 f.seek(0,2);size=f.tell();f.seek(max(0,size-2000000));lines=f.read().splitlines()
 for line in reversed(lines):
  try:s=json.loads(line);break
  except ValueError:continue
p=next(a for a in s['actors'] if 'player' in a)
print(json.dumps({'frame':s['frame_index'],'player':{k:p.get(k) for k in ['id','position','nerve','gravity']},'state':{k:p['player'].get(k) for k in ['state_type','status','stick_position','ground_triangle','movement_up','velocity','bound_actor_934']}},ensure_ascii=False))
limit=float(sys.argv[1]) if len(sys.argv)>1 else 2500
selected=[]
for a in s['actors']:
 if a is p:continue
 d=math.dist(a['position'],p['position'])
 if d<limit:
  selected.append({'distance':round(d,1),**{k:a.get(k) for k in ['id','type','name','position','dead','hidden','nerve']}})
print(json.dumps(sorted(selected,key=lambda a:a['distance']),ensure_ascii=False,indent=2))
