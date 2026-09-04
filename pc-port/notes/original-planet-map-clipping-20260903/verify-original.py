#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,struct,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-clipping-20260903'
spec=importlib.util.spec_from_file_location('creator',R/'pc-port/notes/original-planet-map-creator-20260903/verify-original.py');c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
SPECIAL=('sClippingInfo__','cFollowJointName__')
class Elf(c.Elf):
 def references(self,n):
  rows=super().references(n)
  for r in rows:
   _,at,size,sec=next(s for s in self.symbols if s[0]==r['symbol'])
   if r['symbol'].startswith(SPECIAL):r.pop('value_hex',None)
   elif r['kind']==109:
    assert sec and size==4,(n,r,size);r['value_hex']=self.section_data(sec)[at:at+4].hex()
  return rows

def relocate(elf,ret,name,address,dol):
 # Standard PPC relocation proof, extended for an SDA-relative pointer object.
 original=c.d.o.reader.dol_bytes(dol,address,len(elf.code(name)));refs=ret.references(name);constants={};bases={}
 for r in refs:
  at=int(r['offset'],16)
  if r['kind']==109:
   word=struct.unpack_from('>I',original,at)[0];base=(word>>16)&31;assert base in (2,13);bases[at]=base;effective={2:0x806bfc20,13:0x806b9620}[base]+struct.unpack_from('>h',original,at+2)[0]
  elif 'value_hex' in r:
   pair={q['kind']:int(q['offset'],16) for q in refs if q['symbol']==r['symbol'] and q['addend']==r['addend']};effective=((struct.unpack_from('>H',original,pair[6])[0]<<16)+struct.unpack_from('>h',original,pair[4])[0])&0xffffffff
  else:continue
  if 'value_hex' in r:
   expected=bytes.fromhex(r['value_hex']);assert c.d.o.reader.dol_bytes(dol,effective,len(expected))==expected;constants[r['value_hex']]=effective
  else:assert effective==c.d.o.addresses[r['symbol']]+r['addend']
 code=elf.code(name);records=[]
 for r in elf.references(name):
  at=int(r['offset'],16);kind=r['kind'];target=constants[r['value_hex']] if 'value_hex' in r else c.d.o.addresses[r['symbol']]+r['addend']
  if kind in (4,6):struct.pack_into('>H',code,at,(target if kind==4 else (target+0x8000)>>16)&65535)
  else:
   word=struct.unpack_from('>I',code,at)[0]
   if kind==10:word=(word&0xfc000003)|((target-address-at)&0x3fffffc)
   elif kind==109:
    base=bases[at];word=(word&~0x1fffff)|(base<<16)|((target-{2:0x806bfc20,13:0x806b9620}[base])&65535)
   else:raise AssertionError(r)
   struct.pack_into('>I',code,at,word)
  records.append({**r,'effective_retail_target':hex(target)})
 if name.startswith('initClipping'):
  # Two independent 12-byte stack temporaries have exchanged stack locations:
  # local offset vector [0x18,0x24) <-> expression result [0x0c,0x18).
  # Every direct access and the result pointer are checked before remapping.
  changes={0x6c:(0x3881000c,0x38810018),0x88:(0xd0210020,0xd0210014),
           0x90:(0xd0410014,0xd0410020),0x94:(0xe0618020,0xe0618014),
           0xa4:(0xf0010018,0xf001000c)}
  for at,(expected,replacement) in changes.items():
   assert struct.unpack_from('>I',code,at)[0]==expected,(hex(at),hex(struct.unpack_from('>I',code,at)[0]))
   assert struct.unpack_from('>I',original,at)[0]==replacement
   struct.pack_into('>I',code,at,replacement)
 assert code==original,name
 return records

def main():
 src=R/'src/Game/Map/PlanetMap.cpp';ret=R/'build/original-collision-owner-20260903/retail/obj/Game/Map/PlanetMap.o';obj=B/'PlanetMap.o';cmd=c.d.o.c.compiler('cflags_game')+['-c',str(src),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True);subprocess.run([R/'build/tools/objdiff-cli','diff','-1',ret,'-2',obj,'-o',B/'diff.json','--format','json-pretty'],check=True,stdout=subprocess.DEVNULL)
 a,b=Elf(ret),Elf(obj);j=json.loads((B/'diff.json').read_text());dol=c.d.o.c.DOL.read_bytes();rows=[]
 for name in ('init__9PlanetMapFRC12JMapInfoIter','initClipping__9PlanetMapFRC12JMapInfoIter'):
  left=next(s for s in j['left']['symbols'] if s['name']==name);assert len(a.code(name))==len(b.code(name));refs=relocate(b,a,name,c.d.o.addresses[name],dol);rows.append(dict(symbol=name,retail_address=hex(c.d.o.addresses[name]),bytes=len(a.code(name)),match_percent=left['match_percent'],all_instructions_identical_after_verified_relocations_and_stack_slot_mapping=True,relocated_byte_exact=name.startswith('init__'),verified_relocations=refs));print(name,left['match_percent'],len(a.code(name)),'bytes verified')
 tables=[]
 for prefix in SPECIAL:
  name=next(s[0] for s in a.symbols if s[0].startswith(prefix));tables.append(c.compare_pointer_object(a,b,name,dol))
 table=tables[0];assert table['bytes']==20
 raw=c.d.o.reader.dol_bytes(dol,c.d.o.addresses[table['symbol']]+4,16);assert struct.unpack('>4f',raw)==(3000.0,800.0,1300.0,0.0)
 # The original argument block reads Arg0/Arg2 and performs exactly one unused
 # ordered comparison with zero. Assert its exact source-independent words.
 name=rows[0]['symbol'];raw=c.d.o.reader.dol_bytes(dol,c.d.o.addresses[name],rows[0]['bytes']);assert struct.unpack_from('>I',raw,0x64)[0]==0xfc010040
 (N/'compiler-evidence.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),header_sha256=hashlib.sha256((R/'include/Game/Map/PlanetMap.hpp').read_bytes()).hexdigest(),compiler_command=cmd,functions=rows,pointer_objects=tables,clipping_record_floats=[3000,800,1300,0],argument_comparison_offset='0x64',argument_comparison_word='fc010040',argument_results_not_written_to_actor=True,clipping_normalization={'kind':'Two independent 12-byte stack-temporary locations exchanged','source_offset_vector':'[0x18,0x24)','retail_offset_vector':'[0x0c,0x18)','source_sum_result':'[0x0c,0x18)','retail_sum_result':'[0x18,0x24)','checked_instruction_offsets':['0x6c','0x88','0x90','0x94','0xa4'],'no_instruction_reordering_or_arithmetic_change':True}),indent=2)+'\n')
if __name__=='__main__':main()
