#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,re,hashlib,struct
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-stage-data-holder-20260903'
def mod(name,path):
 s=importlib.util.spec_from_file_location(name,R/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
c=mod('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');reader=mod('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py');proof=mod('proof','pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
TABLE='cLayerDirName__29@unnamed@StageDataHolder_cpp@'
class Elf(reader.Elf):
 def section_data(self,i):
  s=self.sections[i];return bytes(s[5]) if s[1]==8 else super().section_data(i)
 def references(self,name):
  rows=super().references(name)
  for ref in rows:
   if ref['symbol']==TABLE:ref.pop('value_hex',None)
   elif ref.get('value_hex')=='0000000000000000':
    # The only such reference in these selected methods is an ordinary
    # empty C string; dtk assigned its symbol eight padding bytes.
    assert name.startswith('createLocalStageDataHolder__15') and ref['offset']=='0x24'
    ref['value_hex']='00'
  return rows
 def code(self,name):
  _,start,size,index=next(s for s in self.symbols if s[0]==name);return bytearray(self.section_data(index)[start:start+size])
addresses={n:int(v,16) for n,v in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(R/'config/RMGK01/symbols.txt').read_text(),re.M)}
def relocate(elf,target,name,address,dol):
 retail=reader.dol_bytes(dol,address,len(elf.code(name)));refs=target.references(name);constants={};bases={}
 for ref in refs:
  if 'value_hex' not in ref:continue
  offset=int(ref['offset'],16)
  if ref['kind']==109:
   word=struct.unpack_from('>I',retail,offset)[0];base=(word>>16)&31;assert base in (2,13)
   immediate=struct.unpack_from('>h',retail,offset+2)[0];effective={2:0x806bfc20,13:0x806b9620}[base]+immediate;bases[offset]=base
  else:
   pair={r['kind']:int(r['offset'],16) for r in refs if r['symbol']==ref['symbol'] and r['addend']==ref['addend']}
   effective=((struct.unpack_from('>H',retail,pair[6])[0]<<16)+struct.unpack_from('>h',retail,pair[4])[0])&0xffffffff
  expected=bytes.fromhex(ref['value_hex']);assert reader.dol_bytes(dol,effective,len(expected))==expected
  constants[ref['value_hex']]=effective
 code=elf.code(name);records=[]
 for ref in elf.references(name):
  at=int(ref['offset'],16);kind=ref['kind'];target_address=constants[ref['value_hex']] if 'value_hex' in ref else addresses[ref['symbol']]+ref['addend']
  if kind in (4,6):struct.pack_into('>H',code,at,(target_address if kind==4 else (target_address+0x8000)>>16)&65535)
  else:
   word=struct.unpack_from('>I',code,at)[0]
   if kind==10:word=(word&0xfc000003)|((target_address-address-at)&0x3fffffc)
   elif kind==109:
    base=bases[at];word=(word&~0x1fffff)|(base<<16)|((target_address-{2:0x806bfc20,13:0x806b9620}[base])&65535)
   else:raise AssertionError(ref)
   struct.pack_into('>I',code,at,word)
  records.append({**ref,'effective_retail_target':hex(target_address)})
 assert code==retail,name
 return records

def main():
 B.mkdir(parents=True,exist_ok=True);ret=R/'build/original-collision-owner-20260903/retail/obj/Game/Scene/StageDataHolder.o';obj=B/'StageDataHolder.o';cmd=c.compiler('cflags_game')+['-c',str(R/'src/Game/Scene/StageDataHolder.cpp'),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True)
 subprocess.run([R/'build/tools/objdiff-cli','diff','-1',ret,'-2',obj,'-o',B/'diff.json','--format','json-pretty'],check=True,stdout=subprocess.DEVNULL)
 a=Elf(ret);b=Elf(obj);j=json.loads((B/'diff.json').read_text());dol=c.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578';rows=[]
 table_a=sorted(a.references(TABLE),key=lambda x:int(x['offset'],16));table_b=sorted(b.references(TABLE),key=lambda x:int(x['offset'],16));assert len(table_a)==len(table_b)==17
 for x,y in zip(table_a,table_b):
  assert x['offset']==y['offset'] and x['kind']==y['kind']==1 and x['value_hex']==y['value_hex']
  address=struct.unpack('>I',reader.dol_bytes(dol,addresses[TABLE]+int(x['offset'],16),4))[0]
  data=bytes.fromhex(y['value_hex']);assert reader.dol_bytes(dol,address,len(data))==data
 for name,start,size,index in a.symbols:
  if not size or not name.startswith(('__ct__15StageDataHolder','initLayerJmpInfo__15','initPlacementInfoOrderedScenario__15','createLocalStageDataHolder__15','initPlacementInfoOrderedCommon__15')):continue
  left=next(s for s in j['left']['symbols'] if s['name']==name);right=next(s for s in j['right']['symbols'] if s['name']==name)
  for side in (left,right):
   for ins in side['instructions']:
    if 'instruction' in ins:ins['instruction'].setdefault('address','0')
  canonical=[proof.normalize(side,elf.references(name),'stage-data',bool(i)) for i,(side,elf) in enumerate(((left,a),(right,b)))]
  assert canonical[0]==canonical[1] and size==len(b.code(name)),name
  refs=relocate(b,a,name,addresses[name],dol)
  rows.append({'symbol':name,'match_percent':left['match_percent'],'retail_address':hex(addresses[name]),'bytes':size,'all_instructions_identical_after_verified_relocations':True,'verified_relocations':refs,'compiler_command':cmd});print(name,left['match_percent'],size,'relocated byte exact')
 (N/'compiler-evidence.json').write_text(json.dumps({'source_sha256':hashlib.sha256((R/'src/Game/Scene/StageDataHolder.cpp').read_bytes()).hexdigest(),'header_sha256':hashlib.sha256((R/'include/Game/Scene/StageDataHolder.hpp').read_bytes()).hexdigest(),'layer_table_all_17_relocated_pointers_and_strings_equal':True,'functions':rows},indent=2)+'\n')
if __name__=='__main__':main()
