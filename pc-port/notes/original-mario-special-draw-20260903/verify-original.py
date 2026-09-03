#!/usr/bin/env python3
from pathlib import Path
import hashlib,importlib.util,json,re,subprocess
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-mario-special-draw-20260903'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
class Elf(reader.Elf):
 def section_data(self,index):
  s=self.sections[index]
  return bytes(s[5]) if s[1]==8 else super().section_data(index)
source=ROOT/'src/Game/Player/MarioActorSpecialDraw.cpp';obj=BUILD/'SpecialDraw.o';retail=ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Player/MarioActorSpecialDraw.o'
command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(obj)]
with (BUILD/'compile.log').open('w') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(obj),'-o',str(BUILD/'diff.json'),'--format','json-pretty'],cwd=ROOT,check=True)
d=json.loads((BUILD/'diff.json').read_text());a=Elf(retail);b=Elf(obj);dol=compiler.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
base=['calcScreenBoxRange__10MarioActorFv','calcFogLighting__10MarioActorFv','updateAlphaDL__10MarioActorFUc','updateSimpleAlphaDL__10MarioActorFUc','updateReflectAlphaDL__10MarioActorFUc','updateLightDL__10MarioActorFRC6Color8RC6Color8RC6Color8f','swap__9DLchangerFv'];syms=(ROOT/'config/RMGK01/symbols.txt').read_text();rows=[]
for name in base:
 left=next(s for s in d['left']['symbols'] if s['name']==name);right=next(s for s in d['right']['symbols'] if s['name']==name)
 address=int(re.search(r'^'+re.escape(name)+r' = .text:(0x[0-9A-F]+)',syms,re.M)[1],16)
 refs=[e.references(name) for e in (a,b)];calls=[[r['symbol'] for r in rs if r['kind']==10] for rs in refs]
 constants=[{r['value_hex'] for r in rs if r['kind']==109 and 'value_hex' in r} for rs in refs]
 if name.startswith(('updateAlphaDL','updateReflectAlphaDL')):
  assert all(set(bytes.fromhex(v))=={0} for values in constants for v in values)
 else:assert constants[0]==constants[1],(name,constants)
 if name.startswith('calcScreenBoxRange'):
  normalize=lambda c:[n.replace('__ct__Q29JGeometry8TBox2<f>Fffff','set__Q29JGeometry8TBox2<f>Fffff') for n in c if n not in ('_savegpr_29','_restgpr_29','__as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>','__pl__Q29JGeometry8TVec2<f>CFRCQ29JGeometry8TVec2<f>')]
  assert normalize(calls[0])==normalize(calls[1])
  assert left['match_percent']>=84
 else:
  assert calls[0]==calls[1],(name,calls)
  assert left['match_percent']>=91
 row={'symbol':name,'address':hex(address),'retail_bytes':int(left['size']),'compiled_bytes':int(right['size']),'objdiff_match_percent':left['match_percent'],'direct_calls_identical':calls[0]==calls[1],'retail_calls':calls[0],'compiled_calls':calls[1],'retail_references':refs[0],'compiled_references':refs[1],'retail_function_sha256':hashlib.sha256(compiler.dol_bytes(dol,address,int(left['size']))).hexdigest()}
 rows.append(row);print(name,left['match_percent'],left['size'],right['size'])
evidence={'dol_sha1':hashlib.sha1(dol).hexdigest(),'compiler_command':command,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'functions':rows,'weighted_objdiff_percent':sum(r['retail_bytes']*r['objdiff_match_percent'] for r in rows)/sum(r['retail_bytes'] for r in rows),'reader_note':'NOBITS sections read as zero storage; only actual loaded constants are compared. Screen helper inlining/ctor-to-literal-set differences are explicitly recorded, not counted as exact calls.'}
for p in (BUILD,NOTES):(p/'compiler-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('Weighted match',evidence['weighted_objdiff_percent'])
