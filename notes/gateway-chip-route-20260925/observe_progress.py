"""Read-only milestone capture for the live original-game run; never sends input."""
from pathlib import Path
import gzip,json,time,sys
p=Path(__file__).resolve().parent;limit=float(sys.argv[1]) if len(sys.argv)>1 else 40
statefile=p/'observer-state.json';state=json.loads(statefile.read_text()) if statefile.exists() else {'offset':0,'seen':[]}
seen=set(state['seen']);end=time.monotonic()+limit;last=None
wanted={'ChipBase':['NrvGotE'],'SuperSpinDriver':['NrvShootStartE','NrvShootE','NrvCoolDownE'],'SpinDriver':['NrvShootStartE','NrvShootE','NrvCoolDownE'],'PowerStar':['NrvWeakToWaitE','NrvWaitE','NrvStageClearDemoE'],'FlipPanel':['NrvBackLandE','NrvBackE','NrvEndE'],'FlipPanelObserver':['NrvCompleteE','NrvDemoWaitE'],'CapsuleCage':['NrvOpenE','NrvOpenWaitE'],'CrystalCage':['NrvBreakE','NrvBreakAfterE'],'KeySwitch':['NrvAppearE','NrvGotE'],'MarioActor':['NrvGameOverBlackHoleE','NrvGameOverE']}
with (p/'route-actors.jsonl').open('rb') as source,(p/'observed-milestones.jsonl').open('a') as output,gzip.open(p/'observed-snapshots.jsonl.gz','ab') as snapshots:
 source.seek(state['offset'])
 while time.monotonic()<end:
  start=source.tell();line=source.readline()
  if not line.endswith(b'\n'):
   source.seek(start)
   if (p/'route.json').exists():break
   time.sleep(.2);continue
  s=json.loads(line);last=s;state['offset']=source.tell();events=[]
  stage=s['stage'];scene=s.get('scene_nerve',{}).get('type','')
  key='scene:'+stage+':'+scene
  if key not in seen:seen.add(key);events.append({'scene':stage,'nerve':scene})
  for a in s['actors']:
   nerve=(a.get('nerve')or{}).get('type','')
   if not any(kind in nerve and any(suffix in nerve for suffix in suffixes) for kind,suffixes in wanted.items()):continue
   key=f"{stage}:{a['id']}:{nerve}:{a['dead']}"
   if key in seen:continue
   seen.add(key);events.append({k:a.get(k) for k in ('id','type','position','dead','nerve')})
  if events:
   row={'frame':s['frame_index'],'events':events}
   output.write(json.dumps(row)+'\n');output.flush();snapshots.write(line);snapshots.flush();print(json.dumps(row),flush=True)
state['seen']=sorted(seen);statefile.write_text(json.dumps(state,indent=2)+'\n')
if last:print(json.dumps({'last_frame':last['frame_index'],'stage':last['stage'],'terminal':(p/'route.json').exists()}),flush=True)
