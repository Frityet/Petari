#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,struct
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-creator-20260903'
spec=importlib.util.spec_from_file_location('data',R/'pc-port/notes/original-planet-map-data-20260903/verify-original.py');d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
TABLE='sUniquePlanetCreateFuncTable__30@unnamed@PlanetMapCreator_cpp@'
class Elf(d.Elf):
 def references(self,n):
  rows=super().references(n)
  for r in rows:
   if r['symbol']==TABLE:r.pop('value_hex',None)
   if n=='getFarClipDistance__21PlanetMapFarClippableCFv':
    _,at,_,sec=next(s for s in self.symbols if s[0]==r['symbol']);r['value_hex']=self.section_data(sec)[at:at+4].hex()
  return rows

def compare_pointer_object(a,b,name,dol):
 aa=sorted(a.references(name),key=lambda r:int(r['offset'],16));bb=sorted(b.references(name),key=lambda r:int(r['offset'],16));assert len(aa)==len(bb)
 sa=next(s for s in a.symbols if s[0]==name);sb=next(s for s in b.symbols if s[0]==name);assert sa[2]==sb[2]
 result=[]
 for x,y in zip(aa,bb):
  assert x['offset']==y['offset'] and x['kind']==y['kind']==1
  address=struct.unpack('>I',d.o.reader.dol_bytes(dol,d.o.addresses[name]+int(x['offset'],16),4))[0]
  if 'value_hex' in x:
   assert x['value_hex']==y['value_hex'];raw=bytes.fromhex(y['value_hex']);assert d.o.reader.dol_bytes(dol,address,len(raw))==raw;value=raw[:-1].decode()
  else:
   assert x['symbol']==y['symbol'] and x['addend']==y['addend'];assert address==d.o.addresses[y['symbol']]+y['addend'];value=y['symbol']
  result.append(dict(offset=x['offset'],retail_pointer=hex(address),value=value))
 # All unrelocated vtable slots/padding must also be identical.
 ca=a.code(name);cb=b.code(name)
 for x in aa:ca[int(x['offset'],16):int(x['offset'],16)+4]=b'\0'*4
 for y in bb:cb[int(y['offset'],16):int(y['offset'],16)+4]=b'\0'*4
 assert ca==cb
 return dict(symbol=name,bytes=sa[2],all_relocated_pointers_and_unrelocated_bytes_equal=True,entries=result)

def main():
 B.mkdir(parents=True,exist_ok=True);src=R/'src/Game/Map/PlanetMapCreator.cpp';ret=R/'build/original-collision-owner-20260903/retail/obj/Game/Map/PlanetMapCreator.o';obj=B/'PlanetMapCreator.o';cmd=d.o.c.compiler('cflags_game')+['-c',str(src),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True);subprocess.run([R/'build/tools/objdiff-cli','diff','-1',ret,'-2',obj,'-o',B/'diff.json','--format','json-pretty'],check=True,stdout=subprocess.DEVNULL)
 a,b=Elf(ret),Elf(obj);j=json.loads((B/'diff.json').read_text());dol=d.o.c.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578';rows=[]
 for left in j['left']['symbols']:
  name=left['name']
  if not name.startswith(('createNameObj<','getCreateFunc__16','getPlanetMapCreator__24','__dt__21PlanetMapFarClippable','getFarClipDistance__21PlanetMapFarClippable')):continue
  assert len(a.code(name))==len(b.code(name));refs=d.o.relocate(b,a,name,d.o.addresses[name],dol)
  rows.append(dict(symbol=name,retail_address=hex(d.o.addresses[name]),bytes=len(a.code(name)),match_percent=left['match_percent'],all_instructions_identical_after_verified_relocations=True,verified_relocations=refs))
 assert len(rows)==27
 objects=[compare_pointer_object(a,b,n,dol) for n in (TABLE,'__vt__21PlanetMapFarClippable')]
 assert len(objects[0]['entries'])==78
 evidence=dict(source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),header_sha256=hashlib.sha256((R/'include/Game/Map/PlanetMap.hpp').read_bytes()).hexdigest(),compiler_command=cmd,functions=rows,pointer_objects=objects,no_runtime_creator_table_initialization=not any(n.startswith('__sinit') and size for n,at,size,sec in a.symbols))
 (N/'compiler-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n');print(len(rows),'functions',sum(r['bytes'] for r in rows),'bytes exact;',len(objects[0]['entries']),'table pointers exact; vtable exact')
if __name__=='__main__':main()
