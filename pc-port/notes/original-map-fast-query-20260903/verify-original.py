#!/usr/bin/env python3
from pathlib import Path
import hashlib,importlib.util,json,subprocess,struct,re
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-map-fast-query-20260903'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
class Elf(reader.Elf):
 def section_data(self,index):
  s=self.sections[index];return bytes(s[5]) if s[1]==8 else super().section_data(index)
 def references(self,name):
  refs=super().references(name)
  for ref in refs:
   if ref["symbol"] in ("mSortBuffer","mSortCount"):ref.pop("value_hex",None)
  return refs
 def code(self,name):
  _,start,size,index=next(s for s in self.symbols if s[0]==name);return bytearray(self.section_data(index)[start:start+size])
def normalize(elf,name):
 code=elf.code(name);refs=[]
 for r in elf.references(name):
  at=int(r['offset'],16);kind=r['kind']
  if kind==10:
   word=struct.unpack_from('>I',code,at)[0];struct.pack_into('>I',code,at,word&0xfc000003)
  elif kind in (4,5,6):code[at:at+2]=b'\0\0'
  elif kind==109:
   word=struct.unpack_from('>I',code,at)[0];struct.pack_into('>I',code,at,word&0xffe00000)
  else:raise AssertionError(r)
  refs.append({'offset':r['offset'],'kind':kind,'target':r.get('value_hex',r['symbol']),'addend':r['addend']})
 return code,refs
retail=ROOT/'build/original-collision-owner-20260903/retail/obj/Game/Util/MapUtil.o';a=Elf(retail)
units=[]
for name,start,size,index in a.symbols:
 if name.startswith(('getNearPolyOnLineSort__','getCameraPolyFast__','getFirstPolyOnLineBFast__')):
  units.append(('Game/Util/MapUtil',name,0x803E19D0+start,size))
proof=module('proof','pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
dol=compiler.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
def relocate(elf,target,name,address,size,dol):
 # Constants use the verified retail pool; mutable storage keeps symbol identity.
 code,records=proof.relocated(elf,target,name,address,size,dol)
 code=bytearray(code);retail_code=reader.dol_bytes(dol,address,size)
 for ref in records:
  if ref['symbol'] != 'mSortCount':continue
  assert ref['kind']==109 and int(ref['effective_retail_target'],16)==0x806B7008
  offset=int(ref['offset'],16);word=struct.unpack_from('>I',code,offset)[0]
  target_word=struct.unpack_from('>I',retail_code,offset)[0]
  assert (target_word>>16)&31==13
  assert 0x806B9620+struct.unpack_from('>h',retail_code,offset+2)[0]==0x806B7008
  word=(word&~0x1fffff)|(13<<16)|((0x806B7008-0x806B9620)&65535)
  struct.pack_into('>I',code,offset,word)
  ref['small_data_base_register']=13;ref['small_data_base_address']='0x806b9620'
 for ref in records:
  offset=int(ref["offset"],16);width=2 if ref["kind"] in (4,6) else 4
  assert code[offset:offset+width]==retail_code[offset:offset+width],ref
  ref["relocated_operand_matches_dol"]=True
 return bytes(code),records
rows=[]
for unit,name,address,size in units:
 obj=BUILD/(Path(unit).name+'-original.o');cmd=compiler.compiler('cflags_game')+['-c',str(ROOT/'src'/(unit+'.cpp')),'-o',str(obj)]
 if not rows:
  with (BUILD/(obj.stem+'.log')).open('w') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 out=BUILD/(Path(unit).name+'-original.diff.json')
 if not rows:subprocess.run([ROOT/'build/tools/objdiff-cli','diff','-1',retail,'-2',obj,'-o',out,'--format','json-pretty'],stdout=subprocess.DEVNULL,check=True)
 d=json.loads(out.read_text());left=next(s for s in d['left']['symbols'] if s['name']==name);right=next(s for s in d['right']['symbols'] if s['name']==name);b=Elf(obj);ac,ar=normalize(a,name);bc,br=normalize(b,name)
 assert ar==br and len(ac)==len(bc)==size
 for side in (left,right):
  for ins in side['instructions']:
   if 'instruction' in ins:ins['instruction'].setdefault('address','0')
 canonical=[proof.normalize(side,elf.references(name),'map-fast',bool(i)) for i,(side,elf) in enumerate(((left,a),(right,b)))]
 changes=[]
 if name.startswith('getNearPolyOnLineSort__'):
  mapping={25:21,28:25}
  canonical[0]=[re.sub(r'\br(\d+)\b',lambda m:'r'+str(mapping.get(int(m[1]),int(m[1]))),line) if 97<=i<119 else line for i,line in enumerate(canonical[0])]
  changes=['Instructions [97,119): retail source/destination pointer registers r25/r28 -> compiled r21/r25.']
 if name.startswith('getFirstPolyOnLineBFast__'):
  mapping={0x14:0x38,0x38:0x2c,0x8:0x20,0x2c:0x14,0x20:0x8}
  canonical[0]=[re.sub(r'(?<=r1, )0x[0-9a-f]+',lambda m:hex(mapping.get(int(m[0],16),int(m[0],16))),line) if 39<=i<=107 else line for i,line in enumerate(canonical[0])]
  changes=['Instructions [39,108): five distinct TVec3f stack slots 0x14/0x38/0x08/0x2c/0x20 -> 0x38/0x2c/0x20/0x14/0x08; 12-byte extents remain disjoint.']
 assert canonical[0]==canonical[1],(name,[(i,x,y) for i,(x,y) in enumerate(zip(*canonical)) if x!=y])
 relocated,relocations=relocate(b,a,name,address,size,dol)
 equal=relocated==reader.dol_bytes(dol,address,size)
 assert changes or equal
 row={'all_canonical_instructions_equal':True,'normalization':changes,'relocated_bytes_equal':equal,'verified_relocations':relocations,'canonical_instructions':canonical[0],'symbol':name,'retail_address':hex(address),'retail_bytes':size,'compiled_bytes':len(bc),'match_percent':left['match_percent'],'normalized_instructions_identical':ac==bc,'references_identical':ar==br,'references':{'retail':ar,'compiled':br},'compiler_command':cmd,'differences':{side:[ins for ins in next(s for s in d[side]['symbols'] if s['name']==name)['instructions'] if 'diff_kind' in ins] for side in ('left','right')}};rows.append(row);print(name,left['match_percent'],len(bc),ac==bc,ar==br)
(NOTES/'compiler-evidence.json').write_text(json.dumps({'source_sha256':{unit:hashlib.sha256((ROOT/'src'/(unit+'.cpp')).read_bytes()).hexdigest() for unit,*_ in units},'functions':rows},indent=2)+'\n')
