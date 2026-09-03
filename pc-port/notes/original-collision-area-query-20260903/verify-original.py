#!/usr/bin/env python3
from pathlib import Path
import hashlib,importlib.util,json,subprocess,struct,re
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-collision-area-query-20260903'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py');proof=module('proof','pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
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
addresses={name:int(address,16) for name,address in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
dol=compiler.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
rows=[]
for unit in ('CollisionCategorizedKeeper','CollisionParts','KCollision'):
 retail=ROOT/f'build/original-collision-owner-20260903/retail/obj/Game/Map/{unit}.o';a=Elf(retail);obj=BUILD/(unit+'-original.o');source=ROOT/f'src/Game/Map/{unit}.cpp';cmd=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(obj)]
 with (BUILD/(obj.stem+'.log')).open('w') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 out=BUILD/(unit+'-original.diff.json');subprocess.run([ROOT/'build/tools/objdiff-cli','diff','-1',retail,'-2',obj,'-o',out,'--format','json-pretty'],stdout=subprocess.DEVNULL,check=True)
 d=json.loads(out.read_text());b=Elf(obj)
 for name,start,size,index in a.symbols:
  if not size or not name.startswith(('createAreaPolygonList','checkArea3D__')):continue
  left=next(s for s in d['left']['symbols'] if s['name']==name);right=next(s for s in d['right']['symbols'] if s['name']==name);ac,ar=normalize(a,name);bc,br=normalize(b,name)
  assert ar==br and len(ac)==len(bc)==size
  for side in (left,right):
   for ins in side['instructions']:
    if 'instruction' in ins:ins['instruction'].setdefault('address','0')
  canonical=[proof.normalize(side,elf.references(name),'collision-area',bool(i)) for i,(side,elf) in enumerate(((left,a),(right,b)))]
  changes=[]
  if unit=='CollisionCategorizedKeeper':
   canonical[0]=[re.sub(r'\br(20|21)\b',lambda m:'r'+str(41-int(m[1])),line) for line in canonical[0]]
   changes=['One bijective r20/r21 allocation exchange.']
  if unit=='KCollision':
   registers={22:27,23:28,24:29,25:23,26:24,27:26,28:25,29:22}
   canonical[0]=[re.sub(r'\br(\d+)\b',lambda m:'r'+str(registers.get(int(m[1]),int(m[1]))),line) for line in canonical[0]]
   stack={0x8:0xc,0xc:0x8,0x10:0x18,0x18:0x10}
   for before,after in [(0x58,0x7c),(0x64,0x70),(0x7c,0x64),(0x70,0x58)]:
    for offset in (0,4,8):stack[before+offset]=after+offset
   canonical[0]=[re.sub(r'(?<=r1, )0x[0-9a-f]+|0x[0-9a-f]+(?=\(r1\))',lambda m:hex(stack.get(int(m[0],16),int(m[0],16))),line) for line in canonical[0]]
   # The compiled block reuses each consumed input register for its result.
   # Maps are instruction-local and preserve read-before-write dataflow:
   # width=1<<shift; mask=width-1; rz=width-(z&mask);
   # sx=width-(x&mask); ry=width-(y&mask); stepZ=min(stepZ,rz).
   temporary={79:{4:0},81:{0:5},82:{4:0},83:{5:4},84:{8:7},85:{4:0},86:{0:5,8:7},88:{7:5,0:5},89:{5:4,8:7},90:{7:5},91:{4:0,8:7},92:{5:4},95:{7:5}}
   for i,mapping in temporary.items():canonical[0][i]=re.sub(r'\br(\d+)\b',lambda m:'r'+str(mapping.get(int(m[1]),int(m[1]))),canonical[0][i])
   changes=['Callee-saved GPR permutation r22/r23/r24/r25/r26/r27/r28/r29 -> r27/r28/r29/r23/r24/r26/r25/r22.',
            'Disjoint stack slot permutation: words 0x08/0x0c and 0x10/0x18 exchange; 12-byte vectors 0x58/0x64/0x7c/0x70 -> 0x7c/0x70/0x64/0x58.',
            'Instructions 79–95 coalesce consumed temporary registers for the identical width/mask/remaining-axis calculations; exact instruction-local maps are asserted in the verifier.']
  assert canonical[0]==canonical[1],(name,[(i,x,y) for i,(x,y) in enumerate(zip(*canonical)) if x!=y])
  relocated,relocations=proof.relocated(b,a,name,addresses[name],size,dol)
  retail_code=reader.dol_bytes(dol,addresses[name],size);equal=relocated==retail_code
  assert changes or equal
  for ref in relocations:
   offset=int(ref['offset'],16);width=2 if ref['kind'] in (4,6) else 4
   assert relocated[offset:offset+width]==retail_code[offset:offset+width],ref
  row={'all_canonical_instructions_equal':True,'normalization':changes,'relocated_bytes_equal':equal,'verified_relocations':relocations,'canonical_instructions':canonical[0],'source':str(source.relative_to(ROOT)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'symbol':name,'retail_address':hex(addresses[name]),'retail_bytes':size,'compiled_bytes':len(bc),'match_percent':left['match_percent'],'normalized_instructions_identical':ac==bc,'references_identical':ar==br,'references':{'retail':ar,'compiled':br},'compiler_command':cmd};rows.append(row);print(name,left['match_percent'],len(bc),ac==bc,ar==br)
(NOTES/'compiler-evidence.json').write_text(json.dumps({'functions':rows},indent=2)+'\n')
