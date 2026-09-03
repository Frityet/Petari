#!/usr/bin/env python3
from pathlib import Path
import hashlib,importlib.util,json,subprocess,struct
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-collision-scene-owner-20260903'
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
retail=ROOT/'build/original-collision-owner-20260903/retail/obj/Game/Map/CollisionParts.o';a=Elf(retail)
units=[('Game/Map/CollisionParts','makeEqualScale__14CollisionPartsFPA4_f',0x801762F0,444),('JSystem/JGeometry/TMatrix','getScale__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry8TVec3<f>',0x80175D4C,172)]
rows=[]
for unit,name,address,size in units:
 obj=BUILD/(Path(unit).name+'-original.o');cmd=compiler.compiler('cflags_game')+['-c',str(ROOT/'src'/(unit+'.cpp')),'-o',str(obj)]
 with (BUILD/(obj.stem+'.log')).open('w') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 out=BUILD/(Path(unit).name+'-original.diff.json');subprocess.run([ROOT/'build/tools/objdiff-cli','diff','-1',retail,'-2',obj,'-o',out,'--format','json-pretty'],stdout=subprocess.DEVNULL,check=True)
 d=json.loads(out.read_text());left=next(s for s in d['left']['symbols'] if s['name']==name);right=next(s for s in d['right']['symbols'] if s['name']==name);b=Elf(obj);ac,ar=normalize(a,name);bc,br=normalize(b,name)
 row={'symbol':name,'retail_address':hex(address),'retail_bytes':size,'compiled_bytes':len(bc),'match_percent':left['match_percent'],'normalized_instructions_identical':ac==bc,'references_identical':ar==br,'references':{'retail':ar,'compiled':br},'compiler_command':cmd,'differences':{side:[ins for ins in next(s for s in d[side]['symbols'] if s['name']==name)['instructions'] if 'diff_kind' in ins] for side in ('left','right')}};rows.append(row);print(name,left['match_percent'],len(bc),ac==bc,ar==br)
(NOTES/'compiler-evidence.json').write_text(json.dumps({'source_sha256':{unit:hashlib.sha256((ROOT/'src'/(unit+'.cpp')).read_bytes()).hexdigest() for unit,*_ in units},'functions':rows},indent=2)+'\n')
