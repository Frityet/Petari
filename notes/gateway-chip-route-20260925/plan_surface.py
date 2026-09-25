"""Plan notes-only controller waypoints on original KCL faces; no game-state writes."""
from pathlib import Path
import collections,gzip,heapq,itertools,json,math
notes=Path(__file__).resolve().parent
geometry=json.loads(Path('build/gateway-native-launch-terrain-20260925/terrain-geometry.json').read_text())
tri=geometry['triangles'];bins={};verts=[];edges={}
def weld(v):
 cell=tuple(math.floor(x/.1) for x in v)
 for delta in itertools.product((-1,0,1),repeat=3):
  for i in bins.get(tuple(cell[j]+delta[j] for j in range(3)),()):
   if math.dist(v,verts[i])<.08:return i
 i=len(verts);verts.append(v);bins.setdefault(cell,[]).append(i);return i
for t in tri:
 ids=[weld(v) for v in t['vertices']]
 for j in range(3):edges.setdefault(tuple(sorted((ids[j],ids[(j+1)%3]))),[]).append(t['index'])
valid={t['index'] for t in tri if t['slope_dot']>=.75 and t['radius']>=1040};nav={i:[] for i in valid};bounds=[]
for edge,owners in edges.items():
 active=[i for i in owners if i in valid]
 for i,j in itertools.combinations(active,2):nav[i].append(j);nav[j].append(i)
 if len(active)==1:bounds.append(edge)
comp={};groups=[]
for start in valid:
 if start in comp:continue
 cid=len(groups);group=[];queue=[start];comp[start]=cid
 while queue:
  i=queue.pop();group.append(i)
  for j in nav[i]:
   if j not in comp:comp[j]=cid;queue.append(j)
 groups.append(group)
main=max(groups,key=len)
def distance_to_edge(p,a,b):
 d=[y-x for x,y in zip(a,b)];n=sum(x*x for x in d);t=max(0,min(1,sum((x-y)*z for x,y,z in zip(p,a,d))/n))
 return math.dist(p,[x+t*y for x,y in zip(a,d)])
clear={i:min(distance_to_edge(tri[i]['mid'],verts[e[0]],verts[e[1]]) for e in bounds) for i in main}
def path(p,q):
 start=min(main,key=lambda i:math.dist(p,tri[i]['mid']));end=min(main,key=lambda i:math.dist(q,tri[i]['mid']))
 cost={start:0};parent={};heap=[(0,start)]
 while heap:
  d,i=heapq.heappop(heap)
  if d!=cost[i]:continue
  if i==end:break
  for j in nav[i]:
   margin=min(clear[i],clear[j]);nd=d+math.dist(tri[i]['mid'],tri[j]['mid'])*(1+(120/max(margin,10))**2)
   if nd<cost.get(j,float('inf')):cost[j]=nd;parent[j]=i;heapq.heappush(heap,(nd,j))
 ids=[end]
 while ids[-1]!=start:ids.append(parent[ids[-1]])
 ids.reverse()
 return {'prisms':ids,'points':[tri[i]['mid'] for i in ids],'target_position':q,'end_projection_distance':math.dist(q,tri[end]['mid']),'minimum_centroid_boundary_clearance':min(clear[i] for i in ids)}
with gzip.open(notes.parent/'gateway-native-launch-20260925/partial-flight-snapshots.jsonl.gz','rt') as f:
 snapshot=json.loads(f.read().splitlines()[-1])
actors={a['id']:a for a in snapshot['actors']};current=actors[799]['position'];routes={}
for aid in [806,803,793,809,871]:
 route=routes[str(aid)]=path(current,actors[aid]['position']);current=route['points'][-1]
 print(aid,len(route['points']),round(route['end_projection_distance'],1),round(route['minimum_centroid_boundary_clearance'],1))
(notes/'surface-routes.json').write_text(json.dumps({'provenance':geometry['provenance'],'filter':{'slope_dot':.75,'radius':1040,'largest_component_faces':len(main)},'limitations':'Grounded approach candidates only. Centroid clearance is not a continuous movement guarantee. Disconnected target platforms still require original movement/jump; no successful pickup is inferred from an approach.','routes':routes},indent=2)+'\n')
