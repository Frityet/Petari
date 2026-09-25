"""Read-only KCL/trace analysis; emits navigation notes, never controller input."""
from pathlib import Path
import json,struct,math,hashlib,heapq,itertools
base=Path(__file__).resolve().parent
cache=Path('build/gateway-native-launch-terrain-20260925');cache.mkdir(parents=True,exist_ok=True)
archive=cache/'HeavensDoorBlackHolePlanet.arc';raw=archive.read_bytes();data=raw
if data[:4]==b'Yaz0':
 expected=struct.unpack_from('>I',data,4)[0];out=bytearray();pos=16
 while len(out)<expected:
  flags=data[pos];pos+=1
  for bit in range(7,-1,-1):
   if len(out)==expected:break
   if flags&(1<<bit):out.append(data[pos]);pos+=1
   else:
    first,second=data[pos:pos+2];pos+=2;count=first>>4
    if not count:count=data[pos]+0x12;pos+=1
    else:count+=2
    distance=((first&15)<<8|second)+1
    for _ in range(count):out.append(out[-distance])
 data=bytes(out)
assert data[:4]==b'RARC'
u32=lambda o:struct.unpack_from('>I',data,o)[0];u16=lambda o:struct.unpack_from('>H',data,o)[0]
nodes,entries,strings,payload=[0x20+u32(o) for o in (0x24,0x2c,0x34,0x0c)];files={}
def walk(node,path):
 off=nodes+node*16
 for i in range(u16(off+10)):
  e=entries+(u32(off+12)+i)*20;m=u32(e+4);at=strings+(m&0xffffff);name=data[at:data.index(0,at)].decode('cp932')
  if name in ('.','..'):continue
  current=path+'/'+name
  if m>>24&2:walk(u32(e+8),current)
  else:at=payload+u32(e+8);files[current]=data[at:at+u32(e+12)]
walk(0,'');kname=next(n for n in files if n.endswith('.kcl'));kcl=files[kname]
vo,no,po,oo=struct.unpack_from('>4I',kcl);vec=lambda off:struct.unpack_from('>3f',kcl,off)
verts=[vec(i) for i in range(vo,no,12)];norms=[vec(i) for i in range(no,po+16,12)]
add=lambda a,b:tuple(x+y for x,y in zip(a,b));sub=lambda a,b:tuple(x-y for x,y in zip(a,b));mul=lambda a,k:tuple(x*k for x in a)
dot=lambda a,b:sum(x*y for x,y in zip(a,b));cross=lambda a,b:(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);norm=lambda a:math.sqrt(dot(a,a));unit=lambda a:mul(a,1/norm(a))
center=(13917.0625,-1896.3616943359375,-4606.8095703125)
# Quantized native zone rotation observed on the five chip actors.
x,y,z=map(math.radians,(132.5390625,20.91796875,19.27001953125))
def rotate(p):
 a,b,c=p;b,c=math.cos(x)*b-math.sin(x)*c,math.sin(x)*b+math.cos(x)*c
 a,c=math.cos(y)*a+math.sin(y)*c,-math.sin(y)*a+math.cos(y)*c
 return (math.cos(z)*a-math.sin(z)*b,math.sin(z)*a+math.cos(z)*b,c)
world=lambda p:add(center,rotate(p));triangles=[]
for at in range(po+16,oo,16):
 h,vi,ni,a,b,c,attr=struct.unpack_from('>f6H',kcl,at);point=verts[vi];normal=norms[ni];vtx=[point]
 for direction in (cross(normal,norms[b]),cross(norms[a],normal)):
  vtx.append(add(point,mul(direction,h/dot(direction,norms[c]))))
 mid=tuple(sum(v[j] for v in vtx)/3 for j in range(3));rad=norm(mid)
 triangles.append({'index':(at-po)//16-1,'normal':rotate(normal),'vertices':[world(v) for v in vtx],'mid':world(mid),'radius':rad,'slope_dot':dot(normal,unit(mid)),'attr':attr})
trace=base.parent/'route-actors.jsonl';frames={}
with trace.open('rb') as f:
 f.seek(0,2);size=f.tell();f.seek(max(0,size-25000000));lines=f.read().splitlines()
for line in lines:
 try:s=json.loads(line)
 except ValueError:continue
 if 25320<=s['frame_index']<=26810:frames[s['frame_index']]=s
checks=[]
for frame in (25360,25870,26480,26520):
 s=frames[frame];p=next(a for a in s['actors'] if 'player'in a)['player'];g=p['ground_triangle'];t=triangles[g['prism_index']]
 checks.append({'frame':frame,'prism':g['prism_index'],'max_vertex_residual':max(min(math.dist(v,w) for w in t['vertices']) for v in g['vertices'])})
print('files',kname,'triangles',len(triangles),'checks',checks)
print('radius percentiles',[sorted(t['radius'] for t in triangles)[int((len(triangles)-1)*p)] for p in (0,.1,.25,.5,.75,.9,1)])
print('walkable count',sum(t['slope_dot']>.65 for t in triangles))
report={'archive_sha256':hashlib.sha256(raw).hexdigest(),'kcl_file':kname,'kcl_sha256':hashlib.sha256(kcl).hexdigest(),'triangle_count':len(triangles),'native_rotation_degrees':[math.degrees(x),math.degrees(y),math.degrees(z)],'trace_checks':checks}
(cache/'terrain-geometry.json').write_text(json.dumps({'provenance':report,'triangles':triangles},indent=2)+'\n')
# Connect actual KCL triangles through welded shared edges; no straight-world shortcut.
vertex_bins={};welded=[]
def weld(v):
 cell=tuple(math.floor(x/0.1) for x in v)
 for delta in itertools.product((-1,0,1),repeat=3):
  for i in vertex_bins.get(tuple(cell[j]+delta[j] for j in range(3)),()):
   if math.dist(v,welded[i])<.08:return i
 i=len(welded);welded.append(v);vertex_bins.setdefault(cell,[]).append(i);return i
edges={}
for t in triangles:
 ids=[weld(v) for v in t['vertices']];t['vertex_ids']=ids
 for j in range(3):edges.setdefault(tuple(sorted((ids[j],ids[(j+1)%3]))),[]).append(t['index'])
valid={t['index'] for t in triangles if t['slope_dot']>=.75 and t['radius']>=1040}
nav={i:[] for i in valid};bounds=[]
for e,owners in edges.items():
 active=[i for i in owners if i in valid]
 if len(active)==2:
  i,j=active;nav[i].append(j);nav[j].append(i)
 elif len(active)==1:bounds.append(e)
def point_segment_distance(p,a,b):
 d=sub(b,a);t=max(0,min(1,dot(sub(p,a),d)/dot(d,d)));return math.dist(p,add(a,mul(d,t)))
clear={i:min(point_segment_distance(triangles[i]['mid'],welded[e[0]],welded[e[1]]) for e in bounds) for i in valid}
print('valid',len(valid),'boundary_edges',len(bounds),'clearance max',max(clear.values()))
def nearest_valid(p):return min(valid,key=lambda i:math.dist(p,triangles[i]['mid']))
def path_between(p,q):
 start=nearest_valid(p);end=nearest_valid(q);cost={start:0};parent={};heap=[(0,start)]
 while heap:
  d,i=heapq.heappop(heap)
  if d!=cost[i]:continue
  if i==end:break
  for j in nav[i]:
   length=math.dist(triangles[i]['mid'],triangles[j]['mid']);margin=min(clear[i],clear[j]);nd=d+length*(1+(120/max(margin,10))**2)
   if nd<cost.get(j,float('inf')):cost[j]=nd;parent[j]=i;heapq.heappush(heap,(nd,j))
 if end not in cost:return {'error':'disconnected','start_prism':start,'end_prism':end}
 ids=[end]
 while ids[-1]!=start:ids.append(parent[ids[-1]])
 ids.reverse();steps=[{'prism':i,'position':triangles[i]['mid'],'edge_clearance':clear[i],'radius':triangles[i]['radius'],'radial_normal_dot':triangles[i]['slope_dot']} for i in ids]
 return {'start_prism':start,'end_prism':end,'start_projection_distance':math.dist(p,triangles[start]['mid']),'end_projection_distance':math.dist(q,triangles[end]['mid']),'length':sum(math.dist(a['position'],b['position']) for a,b in zip(steps,steps[1:])),'minimum_center_edge_clearance':min(clear[i] for i in ids),'steps':steps}
A={a['id']:a for a in frames[25360]['actors']};start=next(a for a in frames[25360]['actors'] if 'player'in a)['position']
routes={};curr=start
for i in (806,803,793,809,871):
 routes[str(i)]=path_between(curr,A[i]['position']);curr=A[i]['position']
 print('route',i,{k:v for k,v in routes[str(i)].items() if k!='steps'})
# Also a short first detour through the optional actual cage-side placement.
routes['via1004_first']=path_between(start,A[1004]['position'])
routes['via1004_second']=path_between(A[1004]['position'],A[806]['position'])
for name in ('via1004_first','via1004_second'):print('route',name,{k:v for k,v in routes[name].items() if k!='steps'})
(base/'terrain-routes.json').write_text(json.dumps({'provenance':report,'navigation_filter':{'minimum_radial_normal_dot':.75,'minimum_radius':1040,'boundary_clearance':'Euclidean distance from triangle centroid to all outer-navigation boundary edges; excludes steep/inner-crater triangles','cost':'length*(1+(120/max(clearance,10))^2)'},'routes':routes},indent=2)+'\n')
