import struct,json,math,pathlib,sys,hashlib
p=pathlib.Path('notes/demo-system-verification-20260919');data=(p/'physics/HeavensDoorAppearStepAAfter.arc').read_bytes()
if data[:4]==b'Yaz0':
 expected=struct.unpack_from('>I',data,4)[0];out=bytearray();pos=16
 while len(out)<expected:
  flags=data[pos];pos+=1
  for bit in range(7,-1,-1):
   if len(out)==expected:break
   if flags&(1<<bit):out.append(data[pos]);pos+=1
   else:
    first,second=data[pos:pos+2];pos+=2;count=first>>4
    if count==0:count=data[pos]+0x12;pos+=1
    else:count+=2
    distance=((first&15)<<8|second)+1
    for _ in range(count):out.append(out[-distance])
 data=bytes(out)
source_yaz0_archive_sha256=hashlib.sha256((p/'physics/HeavensDoorAppearStepAAfter.arc').read_bytes()).hexdigest()
assert data[:4]==b'RARC' 
u32=lambda off:struct.unpack_from('>I',data,off)[0]
u16=lambda off:struct.unpack_from('>H',data,off)[0]
nodes,entries,strings,payload=[0x20+u32(o) for o in (0x24,0x2c,0x34,0x0c)]
files={}
def walk(node,path):
 off=nodes+node*16
 for i in range(u16(off+10)):
  entry=entries+(u32(off+12)+i)*20;meta=u32(entry+4);at=strings+(meta&0xffffff);name=data[at:data.index(0,at)].decode('cp932')
  if name in('.','..'):continue
  current=path+'/'+name
  if meta>>24&2:walk(u32(entry+8),current)
  else:at=payload+u32(entry+8);files[current]=data[at:at+u32(entry+12)]
walk(0,'');print('files',list(files))
kcl=next(blob for name,blob in files.items()if name.endswith('.kcl'));v,n,pr,oc=struct.unpack_from('>4I',kcl)
print('kcl',len(kcl),v,n,pr,oc)
vec=lambda off:struct.unpack_from('>3f',kcl,off)
verts=[vec(i)for i in range(v,n,12)];norms=[vec(i)for i in range(n,pr+16,12)]
# RMG KCL stores prism offset pointing to dummy entry; each real prism is +16.
cross=lambda a,b:[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
dot=lambda a,b:sum(x*y for x,y in zip(a,b))
placements=json.loads((p/'final2-route-placements.json').read_text());print('placementkeys',placements.keys())
rows=placements['entries'];mat=next(x['zone_placement_matrix']for x in rows if x['object']=='HeavensDoorAppearStepAAfter')
world=lambda a:[sum(mat[i*4+j]*a[j]for j in range(3))+mat[i*4+3]for i in range(3)]
triangles=[]
for at in range(pr+16,oc,16):
 h,vi,ni,a,b,c,attr=struct.unpack_from('>f6H',kcl,at);point=verts[vi];normal=norms[ni];vtx=[list(point)]
 for direction in(cross(normal,norms[b]),cross(norms[a],normal)):
  scale=h/dot(direction,norms[c]);vtx.append([x+scale*y for x,y in zip(point,direction)])
 centre=[sum(x[i]for x in vtx)/3 for i in range(3)]
 triangles.append({'index':(at-pr)//16-1,'normal':normal,'vertices':vtx,'centre':centre,'world_centre':world(centre),'attr':attr})
flat=[t for t in triangles if t['normal'][1]>.95]
print('triangles',len(triangles),'flat',len(flat));print('flatcentres',json.dumps([{'index':t['index'],'centre':t['centre'],'world':t['world_centre']}for t in sorted(flat,key=lambda t:t['centre'][1])],indent=2))
(p/'physics/steps-geometry.json').write_text(json.dumps({'matrix':mat,'triangles':triangles,'decompressed_rarc_sha256':hashlib.sha256(data).hexdigest(),'source_yaz0_archive_sha256':source_yaz0_archive_sha256,'kcl_sha256':hashlib.sha256(kcl).hexdigest()},indent=2)+'\n')

report=json.loads((p/'physics/steps-geometry.json').read_text());checks=[]
for line in (p/'final2-route-actors.jsonl').read_text().splitlines():
 try:r=json.loads(line)
 except json.JSONDecodeError:continue
 if r['frame_index']not in(14440,15040,16040,17290):continue
 m=next(a for a in r['actors']if'player'in a)['player'];t=m['ground_triangle'];index=t['prism_index'];decoded=triangles[index];vs=[world(v)for v in decoded['vertices']];errors=[min(math.dist(v,w)for w in vs)for v in t['vertices']]
 checks.append({'frame':r['frame_index'],'prism_index':index,'actual_ground_flag':bool(m['movement_low_word']&0x40000000),'world_vertex_max_residual':max(errors)})
report['world_transform_note']='Authored placement matrix; cross-check against actual traced world vertices bounds numerical differences.'
report['trace_vertex_checks']=checks
report['suggested_tread_centres']=[{'prism_index':i,'local':triangles[i]['centre'],'world':triangles[i]['world_centre']}for i in(238,244,248,254,124,118,224,230,270)]
(p/'physics/steps-geometry.json').write_text(json.dumps(report,indent=2)+'\n')
