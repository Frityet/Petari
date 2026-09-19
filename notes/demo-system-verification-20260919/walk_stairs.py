"""Bounded ordinary-input navigation along observed authored stair treads; no game state writes."""
import json, time, fcntl
from pathlib import Path
from operate_first_catch import available, publish, StallJump
from follow_actor import controls
p=Path(__file__).resolve().parent
points=[[15080.7,-9198.4,8276.6],[15631.4,-9192.8,8033.7],[15884.4,-9049.7,7741.5],[15960.0,-8872.6,7171.2],[15566,-8529.3,6531.5],[14939.3,-8405.7,6411.9],[14291.8,-8283.3,6783.6],[14032.3,-8242,7410.8],[14256.9,-8215.8,7562.2],[14732.918,-8280.555,7561.157]]
index=0
jump=StallJump()
with (p/'final2-input.json.operator-lock').open('a') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
 try:
  with (p/'final2-route-actors.jsonl').open() as source:
   with source:
    deadline=time.monotonic()+100
    while time.monotonic()<deadline:
     snap=None
     for snap in available(source):pass
     if snap is None:time.sleep(.08);continue
     frame=snap['frame_index']
     player=next(a for a in snap['actors'] if 'player' in a)
     info=player['player']
     x,y,d=controls(player,{'position':points[index]},0)
     if d<150:
      index+=1
      if index==len(points):break
      x,y,d=controls(player,{'position':points[index]},0)
     if frame>=29800:break
     buttons,event=jump.update(frame,player['position'],index,info['status']==0,info.get('ground_triangle') is not None)
     command={'buttons':buttons,'pointer':'','stick':f'{frame+1}-{frame+90}:{x:.6f}:{y:.6f}'}
     publish(p/'final2-input.json',command)
     print(json.dumps({'frame':frame,'waypoint':index,'position':player['position'],'target':points[index],'distance':d,'jump':event,'command':command}),flush=True)
 finally:publish(p/'final2-input.json',{'buttons':'','pointer':'','stick':''})
