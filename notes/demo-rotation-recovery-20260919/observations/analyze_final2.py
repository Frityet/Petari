"""Read-only orientation vector audit of final2; old trace has no draw matrices."""
import json,math,statistics,pathlib,hashlib,gzip
p=pathlib.Path('notes/demo-system-verification-20260919/final2-route-actors.jsonl')
text=gzip.decompress(p.with_suffix(p.suffix+'.gz').read_bytes()).decode() if not p.exists() else p.read_text()
rows=[json.loads(l)for l in text.splitlines()]
def norm(v):return math.sqrt(sum(x*x for x in v))
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def angle(a,b):
 n=norm(a)*norm(b)
 return math.degrees(math.acos(max(-1,min(1,dot(a,b)/n))))if n>1e-9 else None
windows={'guide':(1450,2620),'bush_chase':(3060,3190),'pipe_chase':(4220,4690),'hole_chase':(5980,10480),'early_ascent':(14440,17210)}
records=[];rabbits={};status_changes=[];prev=None
for row in rows:
 f=row['frame_index'];actors=row['actors'];m=next((a for a in actors if'player'in a),None)
 if m:
  q=m['player'];ground=q.get('ground_triangle');up=[-v for v in q['air_gravity']];chosen_up=[-v for v in(q.get('gravity_info')or{}).get('vector',[0,0,0])];record={'frame':f,'status':q['status'],'ground_flag':bool(q['movement_low_word']&0x40000000),'jump_flag':bool(q['movement_low_word']&0x80000000),'stick_magnitude':q['stick_position'][2],'head_to_gravity_degrees':angle(q['up'],up),'head_to_selected_gravity_degrees':angle(q['up'],chosen_up),'head_to_ground_degrees':angle(q['up'],ground['normal'])if ground else None,'movement_up_to_ground_degrees':angle(q['movement_up'],ground['normal'])if ground else None,'front_head_dot':dot(q['front'],q['up']),'head_length':norm(q['up']),'front_length':norm(q['front']),'velocity_length':norm(q['velocity']),'position':m['position'],'head':q['up'],'air_gravity':q['air_gravity'],'movement_up':q['movement_up'],'front':q['front'],'ground':ground,'gravity_info':q.get('gravity_info'),'window':next((k for k,(a,b)in windows.items()if a<=f<=b),None)};records.append(record)
  if prev!=q['status']:
   status_changes.append({'frame':f,'status':q['status'],'position':m['position']});prev=q['status']
 for a in actors:
  if a['type']=='10DemoRabbit'and not a['dead']:
   rabbits.setdefault(a['id'],[]).append({'frame':f,'nerve':a['nerve'],'rotation':a['rotation'],'gravity':a['gravity'],'position':a['position']})
summary={}
for name in windows:
 selected=[r for r in records if r['window']==name and r['status']==0 and r['ground_flag']and r['stick_magnitude']>.1]
 metrics={}
 for key in('head_to_gravity_degrees','head_to_selected_gravity_degrees','head_to_ground_degrees','movement_up_to_ground_degrees','front_head_dot','head_length','front_length'):
  values=sorted(r[key]for r in selected if r[key]is not None)
  metrics[key]={'min':values[0],'median':statistics.median(values),'p95':values[int((len(values)-1)*.95)],'max':values[-1]}if values else None
 summary[name]={'selected_count':len(selected),'metrics':metrics,'largest_head_gravity_samples':sorted(selected,key=lambda r:r['head_to_gravity_degrees'] or 0,reverse=True)[:4]}
result={'source_trace_sha256_uncompressed':hashlib.sha256(text.encode()).hexdigest(),'selection':'window; status0; original on-ground flag _1; processed stick magnitude>0.1','windows':summary,'recovery_transitions':[r for r in status_changes if 5900<=r['frame']<=10500],'demo_rabbits':{str(k):{'alive_first':v[0]['frame'],'alive_last':v[-1]['frame'],'authored_euler_component_ranges':[max(x['rotation'][i]for x in v)-min(x['rotation'][i]for x in v)for i in range(3)],'gravity_change_degrees_first_to_last':angle(v[0]['gravity'],v[-1]['gravity']),'first':v[0],'last':v[-1]}for k,v in rabbits.items()},'limits':['No model base/joint matrices or rabbit quaternion existed in this trace.','Head vectors do not prove final model or GPU orientation.','Ground polygons can be cached; selected-ground flag reduces but does not establish perfect same-phase alignment.']}
out=pathlib.Path(__file__).with_name('final2-vector-analysis.json');out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
print(json.dumps({k:{'count':v['selected_count'],'head_gravity':v['metrics']['head_to_gravity_degrees'],'head_ground':v['metrics']['head_to_ground_degrees'],'front_head':v['metrics']['front_head_dot']}for k,v in summary.items()},indent=2))
