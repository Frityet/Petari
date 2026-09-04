#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,re,struct
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-particle-resource-owner-20260903'
s=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
cases=[('OriginalParticleResourceLookup',N/'OriginalParticleResourceLookup-root.cpp','Util/SystemUtil'),('OriginalParticleResourceQueries',N/'OriginalParticleResourceQueries-root.cpp','Effect/EffectSystemUtil'),('ParticleResourceHolder',R/'src/Game/Effect/ParticleResourceHolder.cpp','Effect/ParticleResourceHolder')]
results=[];evidence=[];symbols=(R/'config/RMGK01/symbols.txt').read_text();dol=(R/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
for name,src,retail_name in cases:
 out=B/(name+'-root.o');cmd=h.compiler('cflags_game')+['-c',str(src),'-o',str(out)]
 p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/(name+'-root-compile.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 retail=R/'build/mario-update-restoration-20260903/retail/obj/Game'/(retail_name+'.o')
 diff=B/(name+'-root.json');subprocess.run([str(R/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(out),'-o',str(diff),'--format','json-pretty'],check=True,capture_output=True)
 rows=[{k:r[k] for k in ['name','size','match_percent']} for r in json.loads(diff.read_text())['left']['symbols'] if r.get('match_percent') is not None and not r['name'].startswith('[.')]
 results.append({'source':str(src.relative_to(R)),'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'command':cmd,'methods':rows});print(name,rows)
 matched={r['name'] for r in rows};elf=h.Elf(retail)
 for symbol,start,size,index in elf.symbols:
  if symbol not in matched:continue
  m=re.search(r'^'+re.escape(symbol)+r' = \.text:0x([0-9A-Fa-f]+);.*size:0x([0-9A-Fa-f]+)',symbols,re.M)
  if not m or not size:continue
  addr,n=[int(v,16) for v in m.groups()];assert n==size
  a=bytearray(elf.section_data(index)[start:start+size]);b=bytearray(h.dol_bytes(dol,addr,size))
  for sec in elf.sections:
   if sec[1]!=4 or sec[7]!=index:continue
   for off in range(sec[4],sec[4]+sec[5],sec[9]):
    at,info,addend=struct.unpack_from('>IIi',elf.data,off)
    if not start<=at<start+size:continue
    at-=start;kind=info&255
    for code in (a,b):
     if kind in (10,109):
      mask=0xfc000003 if kind==10 else 0xffe00000;word=struct.unpack_from('>I',code,at)[0];struct.pack_into('>I',code,at,word&mask)
     elif kind in (4,5,6):code[at:at+2]=b'\0\0'
     elif kind==1:code[at:at+4]=b'\0\0\0\0'
     else:raise AssertionError((symbol,kind))
  assert a==b,(symbol,'DOL mismatch');evidence.append({'symbol':symbol,'address':hex(addr),'size':size,'normalized_retail_dol_equal':True})
def normalized(elf, symbol):
 name,start,size,index=next(row for row in elf.symbols if row[0]==symbol)
 code=bytearray(elf.section_data(index)[start:start+size])
 for sec in elf.sections:
  if sec[1]!=4 or sec[7]!=index:continue
  for off in range(sec[4],sec[4]+sec[5],sec[9]):
   at,info,addend=struct.unpack_from('>IIi',elf.data,off)
   if not start<=at<start+size:continue
   at-=start;kind=info&255
   if kind in (10,109):
    mask=0xfc000003 if kind==10 else 0xffe00000;struct.pack_into('>I',code,at,struct.unpack_from('>I',code,at)[0]&mask)
   elif kind in (4,5,6):code[at:at+2]=b'\0\0'
   elif kind==1:code[at:at+4]=b'\0\0\0\0'
   else:raise AssertionError((symbol,kind))
 return code
for result,(name,src,retail_name) in zip(results,cases):
 elf=h.Elf(B/(name+'-root.o'));retail=h.Elf(R/'build/mario-update-restoration-20260903/retail/obj/Game'/(retail_name+'.o'))
 for method in result['methods']:
  method['normalized_compiled_retail_equal']=normalized(elf,method['name'])==normalized(retail,method['name'])
  if name!='ParticleResourceHolder':assert method['normalized_compiled_retail_equal']
(N/'root-evidence.json').write_text(json.dumps(results,indent=2)+'\n')
(N/'dol-evidence.json').write_text(json.dumps({'dol_sha1':hashlib.sha1(dol).hexdigest(),'methods':evidence},indent=2)+'\n')
