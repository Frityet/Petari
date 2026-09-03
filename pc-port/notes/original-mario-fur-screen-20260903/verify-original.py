#!/usr/bin/env python3
from pathlib import Path
import hashlib,importlib.util,json,re,subprocess,struct
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-mario-fur-screen-20260903'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
class Elf(reader.Elf):
 def section_data(self,index):
  s=self.sections[index];return bytes(s[5]) if s[1]==8 else super().section_data(index)
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
units={'Game/Player/MarioActorSpecialDraw':['initScreenBox__10MarioActorFv','hideBeeFur__10MarioActorFv','showBeeFur__10MarioActorFv','drawSphereMask__10MarioActorCFv'],'Game/Util/FurMulti':['__ct__8FurMultiFP9LiveActorUl','offDraw__8FurMultiFUl','onDraw__8FurMultiFUl'],'Game/Util/LiveActorUtil':['initFurPlayer__2MRFP9LiveActor']}
rows=[];commands=[];syms=(ROOT/'config/RMGK01/symbols.txt').read_text();dol=compiler.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
for rel,names in units.items():
 source=ROOT/'src'/(rel+'.cpp');obj=BUILD/(Path(rel).name+'.o');retail=ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj'/(rel+'.o')
 command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(obj)];commands.append(command)
 with (BUILD/(Path(rel).name+'.compile.log')).open('w') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 diff=BUILD/(Path(rel).name+'.diff.json');subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(obj),'-o',str(diff),'--format','json-pretty'],cwd=ROOT,check=True)
 d=json.loads(diff.read_text());a=Elf(retail);b=Elf(obj)
 for name in names:
  left=next(s for s in d['left']['symbols'] if s['name']==name);right=next(s for s in d['right']['symbols'] if s['name']==name)
  ac,ar=normalize(a,name);bc,br=normalize(b,name)
  assert ar==br,(name,ar,br)
  assert ac==bc,(name,'instruction mismatch')
  address=int(re.search(r'^'+re.escape(name)+r' = .text:(0x[0-9A-F]+)',syms,re.M)[1],16)
  rows.append({'symbol':name,'address':hex(address),'retail_bytes':int(left['size']),'compiled_bytes':int(right['size']),'objdiff_match_percent':left['match_percent'],'all_normalized_instructions_identical':True,'all_reference_targets_identical':True,'references':ar,'dol_function_sha256':hashlib.sha256(compiler.dol_bytes(dol,address,int(left['size']))).hexdigest()})
  print(name,left['match_percent'],'normalized exact',len(ac))
evidence={'dol_sha1':hashlib.sha1(dol).hexdigest(),'compiler_commands':commands,'source_sha256':{rel:hashlib.sha256((ROOT/'src'/(rel+'.cpp')).read_bytes()).hexdigest() for rel in units},'functions':rows,'instruction_bytes_verified':sum(r['retail_bytes'] for r in rows),'normalization':'Only relocation fields cleared; identical offsets, kinds, addends and resolved constant/string bytes or external symbol identities required. NOBITS reads as zeros.'}
for p in (BUILD,NOTES):(p/'compiler-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
