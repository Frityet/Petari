"""Read-only comparison of captured player input directions with original math."""
import importlib.util
import hashlib
import json
import math
from pathlib import Path
import re

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
spec=importlib.util.spec_from_file_location('operator_controls',ROOT/'notes/original-gateway-chase-20260919/follow_actor.py')
operator=importlib.util.module_from_spec(spec);spec.loader.exec_module(operator)
dot,cross,scale,normalized=operator.dot,operator.cross,operator.scale,operator.normalized

def subtract(a,b): return [x-y for x,y in zip(a,b)]
def length(a):return math.sqrt(dot(a,a))
def cosine(a,b):return dot(normalized(a),normalized(b)) if length(a)>1e-5 and length(b)>1e-5 else None

def basis(p):
 s=p['player'];u=s['movement_up'];cx,cy,cz=s['camera_x'],s['camera_y'],scale(s['camera_z'],-1)
 yd,zd=dot(cy,u),dot(cz,u)
 if abs(zd)>abs(yd):
  base=scale(cy,-1 if zd<0 else 1);side=cross(base,u)
 else:
  base=scale(cz,1 if yd<0 else -1);side=cross(base,u)
 front=cross(cx,u)
 raw_lengths=[length(side),length(front)]
 side=normalized(side);front=normalized(front)
 if dot(side,side)==0:side=cx
 if dot(front,front)==0:front=base
 return side,scale(front,-1),raw_lengths

def adjusted(x,y):
 if abs(y)>.5:
  x=0 if abs(x)<.25 else (x-math.copysign(.25,x))/.75
 elif abs(x)>.5:
  y=0 if abs(y)<.2 else (y-math.copysign(.2,y))/.8
 return x,y

def prediction(p,x,y):
 a,b,_=basis(p)
 return [x*v+y*w for v,w in zip(a,b)]

snapshots=[]
trace=ROOT/'notes/gateway-compat-20260919/content-world-first-catch-18000-actors.jsonl'
with trace.open() as source:
 for line in source:
  if not line.endswith('\n'):break
  s=json.loads(line)
  if 11000<=s['frame_index']<=12500:snapshots.append(s)
records=[]
target={'position':[13565.785,-10103.431,7954.287]}
for s in snapshots:
 p=next(a for a in s['actors'] if 'player' in a);state=p['player']
 x,y=state['stick_position'][:2];ax,ay=adjusted(x,y)
 velocity=state['velocity'];pad=state['world_pad_direction']
 up=normalized(state['movement_up']);difference=subtract(target['position'],p['position'])
 tangent=subtract(difference,scale(up,dot(difference,up)))
 fresh=operator.controls(p,target)
 a,b,raw_lengths=basis(p)
 record={'frame':s['frame_index'],'position':p['position'],'status':state['status'],
  'controller':s['controller']['stick'],'player_stick':state['stick_position'],
  'predicted_to_pad_dot':cosine(prediction(p,ax,ay),pad),
  'unmargined_to_pad_dot':cosine(prediction(p,x,y),pad),
  'velocity_to_pad_dot':cosine(velocity,pad),'speed':length(velocity),
  'pad_to_hole_tangent_dot':cosine(pad,tangent),
  'fresh_controls_to_hole_tangent_dot':cosine(prediction(p,*fresh[:2]),tangent),
  'current_to_fresh_control_dot':cosine(s['controller']['stick'],fresh[:2]),
  'hole_distance':fresh[2], 'basis_lengths':raw_lengths,
  'basis_det':dot(a,a)*dot(b,b)-dot(a,b)**2,
  'movement_low_word':state['movement_low_word'],'movement_high_word':state['movement_high_word']}
 records.append(record)

fields=['predicted_to_pad_dot','unmargined_to_pad_dot','velocity_to_pad_dot','pad_to_hole_tangent_dot','fresh_controls_to_hole_tangent_dot','current_to_fresh_control_dot','basis_det','speed']
stats={}
for field in fields:
 data=sorted(r[field] for r in records if r[field] is not None)
 stats[field]={'min':data[0], 'median':data[len(data)//2], 'max':data[-1]}
summary={'scope':'Read-only captured frames11000..12500; fixed hole target supplied by operator logs; no gameplay mutations',
 'count':len(records),'stats':stats,'samples':[r for r in records if r['frame']%100==0]}
moving=[r for r in records if r['speed']>.01 and r['predicted_to_pad_dot'] is not None]
summary['motion_subset']={'selection':'speed >0.01 and both reconstructed/observed pad vectors nonzero; excludes stopped/bound samples and zero input',
 'count':len(moving),'statistics':{}}
for field in ('predicted_to_pad_dot','velocity_to_pad_dot','pad_to_hole_tangent_dot'):
 data=sorted(r[field] for r in moving if r[field] is not None)
 summary['motion_subset']['statistics'][field]={'min':data[0],'median':data[len(data)//2],'max':data[-1]}
summary['provenance']={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in (
 trace,ROOT/'notes/original-gateway-chase-20260919/follow_actor.py',
 ROOT/'src/Game/Player/MarioMove.cpp',ROOT/'src/Game/Player/MarioModule.cpp',
 ROOT/'src/Game/Player/Mario.cpp',ROOT/'src/Game/MapObj/EarthenPipe.cpp')}

events=[]
last={}
for snapshot in snapshots:
 for actor in snapshot['actors']:
  if actor['id'] not in (339,697,699,892,902):continue
  state=(actor.get('nerve') or {}).get('type')
  key=(state,actor['dead'])
  if last.get(actor['id'])==key:continue
  last[actor['id']]=key
  events.append({'frame':snapshot['frame_index'],'id':actor['id'],'state':state,
   'dead':actor['dead'],'position':actor['position']})
summary['position_discontinuities']=[{'before_frame':a['frame'],'after_frame':b['frame'],
 'distance':math.dist(a['position'],b['position']), 'before':a['position'],'after':b['position']}
 for a,b in zip(records,records[1:]) if math.dist(a['position'],b['position'])>1000]

def function_body(source,key):
 begin=source.index('{',source.index(key));end=begin+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return re.sub(r'\s+','',source[begin:end])
equivalence={}
for file,key in [('MarioMove.cpp','void Mario::calcMoveDir('),('MarioModule.cpp','bool MarioModule::calcWorldPadDir(')]:
 native=(ROOT/'src/Game/Player'/file).read_text()
 donor=(ROOT/'decomp/src/Game/Player'/file).read_text()
 equivalence[key]=function_body(native,key)==function_body(donor,key)
(HERE/'source-equivalence.json').write_text(json.dumps(equivalence,indent=2)+'\n')
(HERE/'state-transitions.json').write_text(json.dumps(events,indent=2)+'\n')
(HERE/'samples.json').write_text(json.dumps(records,indent=2)+'\n')
(HERE/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
