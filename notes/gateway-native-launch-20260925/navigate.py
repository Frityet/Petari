"""Notes-only controller operator. Targets come from read-only actor observations.

This process writes only the existing controller JSON and its own log. It cannot
change actors, collision, story flags, nerves or velocities. Keyboard X is sent
separately through normal app controls by the human-facing computer-use tool.
"""
import fcntl
import json
import math
from pathlib import Path
import time
from follow_actor import controls
from operate_first_catch import available, publish, StallJump

notes = Path(__file__).resolve().parent
last_press = -999
press = ''
last_config = None
jump = StallJump()
with (notes/'input.json.operator-lock').open('a') as lock:
    fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    try:
        with (notes/'route-actors.jsonl').open() as source:
            while True:
                if (notes/'route.json').exists():
                    break
                snapshot = None
                for snapshot in available(source):
                    pass
                if snapshot is None:
                    time.sleep(.08)
                    continue
                frame = snapshot['frame_index']
                config = json.loads((notes/'navigate.json').read_text())
                if config.get('stop') or frame >= 99500:
                    break
                player = next(a for a in snapshot['actors'] if 'player' in a)
                target = next((a for a in snapshot['actors'] if a['id'] == config.get('actor')), None)
                if 'position' in config:
                    target = {'position': config['position'], 'dead': False}
                if config != last_config:
                    print(json.dumps({'frame':frame, 'configuration':config}), flush=True)
                    last_config = config
                    jump = StallJump()
                    last_press = -999
                x=y=0
                distance = None
                event = None
                if target and not target.get('dead') and not config.get('neutral'):
                    x,y,distance = controls(player,target,config.get('stop_distance',100))
                if player['player'].get('bound_actor_934') and config.get('neutral_while_bound', True):
                    x=y=0
                status = player['player'].get('state_type','')
                buttons = ''
                interval = config.get('jump_interval',0)
                if config.get('advance_talk',True) and 'MarioTalk' in status:
                    interval = 90
                    x=y=0
                if interval:
                    if frame >= last_press + interval:
                        press=f'{frame+1}-{frame+8}:A'
                        last_press=frame
                    buttons=press
                elif config.get('stall_jump') and distance and distance>config.get('stop_distance',100):
                    buttons,event=jump.update(frame,player['position'],0,
                        player['player'].get('status')==0 and player['player'].get('stick_position',[0,0,0])[2]>.05,
                        bool(player['player'].get('movement_low_word', 0) & 0x40000000))
                command={'buttons':buttons,'pointer':'','stick':f'{frame+1}-{frame+90}:{x:.6f}:{y:.6f}'}
                publish(notes/'input.json',command)
                print(json.dumps({'frame':frame,'player':player['position'],'state':status,'target':None if target is None else {k:target.get(k) for k in ('id','position','nerve','dead')},'distance':distance,'jump':event,'command':command}),flush=True)
    finally:
        publish(notes/'input.json',{'buttons':'','pointer':'','stick':''})
