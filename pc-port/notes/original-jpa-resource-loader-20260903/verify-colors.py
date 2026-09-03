#!/usr/bin/env python3
"""Compare host guard with actual retail look-ahead over contiguous JPC bytes."""
from pathlib import Path
import struct,json,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-resource-loader-20260903';b=(B/'Effect.arc.jpc').read_bytes()
def half(o):return struct.unpack_from('>h',b,o)[0]
def word(o):return struct.unpack_from('>I',b,o)[0]
def f32(x):return struct.unpack('f',struct.pack('f',x))[0]
def calc(off,count,maxf,guarded):
 values=list(b[off+2:off+6]);step=[0.0]*4;j=0;out=[];beyond=[]
 for i in range(maxf+1):
  if not guarded and j>=count:beyond.append({'frame':i,'index':half(off+j*6)})
  if (not guarded or j<count) and i==half(off+j*6):
   values=list(b[off+j*6+2:off+j*6+6]);out.append(bytes(values));j+=1
   if j<count:
    inv=f32(1.0/(half(off+j*6)-half(off+(j-1)*6)))
    step=[f32(inv*f32(b[off+j*6+2+c]-values[c])) for c in range(4)]
   else:step=[0.0]*4
  else:
   values=[f32(values[c]+step[c]) for c in range(4)];out.append(bytes(int(v)&255 for v in values))
 return b''.join(out),beyond
binary=bytearray();records=[];diffs=[];off=16;total=0;out_hash=hashlib.sha256()
for i in range(struct.unpack_from('>H',b,8)[0]):
 uid,count=struct.unpack_from('>HH',b,off);off+=8
 for j in range(count):
  size=word(off+4)
  if b[off:off+4]==b'BSP1':
   maxf=half(off+0x24)
   for flag,pos,npos in [(2,0xc,0x22),(8,0xe,0x23)]:
    if b[off+0x21]&flag:
     start=off+half(off+pos);n=b[off+npos];total+=1
     retail,reads=calc(start,n,maxf,False);guard,_=calc(start,n,maxf,True);out_hash.update(retail);binary.extend(retail)
     if reads:records.append({'id':uid,'channel':'prm' if flag==2 else 'env','keys':n,'max_frame':maxf,'last_key':half(start+(n-1)*6),'lookahead_reads':len(reads),'adjacent_indices':sorted(set(r['index'] for r in reads))})
     if retail!=guard:diffs.append({'id':uid,'channel':flag,'different_frames':[x for x in range(maxf+1) if retail[x*4:x*4+4]!=guard[x*4:x*4+4]]})
  off+=size
result={'jpc_sha256':hashlib.sha256(b).hexdigest(),'tables':total,'tables_with_lookahead':len(records),'lookahead_reads':sum(r['lookahead_reads'] for r in records),'different_tables':diffs,'retail_output_sha256':out_hash.hexdigest(),'lookahead_tables':records}
(B/'retail-colors.bin').write_bytes(binary)
(N/'color-equivalence.json').write_text(json.dumps(result,indent=2)+'\n');print({k:v for k,v in result.items() if k!='lookahead_tables'});assert not diffs
