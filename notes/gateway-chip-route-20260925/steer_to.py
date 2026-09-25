"""Brief supervised ordinary-stick target; no writes except the controller override."""
from pathlib import Path
import json,sys,time
from follow_actor import controls
p=Path(__file__).resolve().parent;target={'position':list(map(float,sys.argv[1:4]))};duration=int(sys.argv[4]);start=None
while True:
 with (p/'route-actors.jsonl').open('rb') as f:
  f.seek(0,2);f.seek(max(0,f.tell()-600000));raw=f.read();lines=raw.splitlines()
  if not raw.endswith(b'\n'):lines.pop()
  s=json.loads(lines[-1])
 frame=s['frame_index'];player=next(a for a in s['actors'] if 'player'in a)
 if start is None:start=frame
 x,y,d=controls(player,target,90)
 if frame-start>=duration or d<=90 or (p/'route.json').exists():
  (p/'manual-input.json').write_text(json.dumps({'until_frame':0,'stick':[0,0]})+'\n');break
 value={'until_frame':frame+45,'stick':[x,y],'target':target['position']}
 tmp=p/'manual-input.next';tmp.write_text(json.dumps(value)+'\n');tmp.replace(p/'manual-input.json')
 print(json.dumps({'frame':frame,'position':player['position'],'distance':d,'command':value}),flush=True)
 time.sleep(.1)
