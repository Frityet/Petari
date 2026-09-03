from pathlib import Path
import importlib.util,struct,json
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-particle-draw-executor-20260903'
s=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
a=h.Elf(R/'build/mario-update-restoration-20260903/retail/obj/Game/Effect/ParticleDrawExecutor.o');b=h.Elf(B/'ParticleDrawExecutor.o')
def normalized(e,sym):
 _,start,size,index=sym;out=bytearray(e.section_data(index)[start:start+size])
 for sec in e.sections:
  if sec[1]!=4 or sec[7]!=index:continue
  for off in range(sec[4],sec[4]+sec[5],sec[9]):
   at,info,addend=struct.unpack_from('>IIi',e.data,off)
   if not start<=at<start+size:continue
   at-=start;kind=info&255
   if kind in (10,109):struct.pack_into('>I',out,at,struct.unpack_from('>I',out,at)[0]&(0xfc000003 if kind==10 else 0xffe00000))
   elif kind in (4,5,6):out[at:at+2]=b'\0\0'
   elif kind==1:out[at:at+4]=b'\0'*4
   else:raise AssertionError(kind)
 return out
rows=[]
names={row['symbol'] for row in json.loads((N/'dol-evidence.json').read_text())['methods']}
for sym in a.symbols:
 if sym[0] not in names:continue
 other=next(s for s in b.symbols if s[0]==sym[0]);x,y=normalized(a,sym),normalized(b,other)
 row={'symbol':sym[0],'same_size':len(x)==len(y),'normalized_instructions_equal':x==y}
 if x!=y:row['different_offsets']=[i for i in range(0,min(len(x),len(y)),4) if x[i:i+4]!=y[i:i+4]]
 rows.append(row)
(N/'ParticleDrawExecutor-normalized-source.json').write_text(json.dumps(rows,indent=2)+'\n')
print(sum(r['normalized_instructions_equal'] for r in rows),'of',len(rows),'source/retail functions instruction-identical after relocation normalization')
