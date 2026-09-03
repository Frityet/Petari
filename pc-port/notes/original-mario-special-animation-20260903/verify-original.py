#!/usr/bin/env python3
"""Original compiler proof for special-mode animation and its chest-joint data."""
import hashlib,importlib.util,json,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-mario-special-animation-20260903';BUILD.mkdir(parents=True,exist_ok=True)

def module(name,path):
 spec=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
DOL=(ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(DOL).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
SOURCES=('src/Game/Player/MarioActorParts.cpp','src/Game/Player/MarioAnimator.cpp')
commands=[];hashes={};objects={}
for path in SOURCES:
 source=ROOT/path;hashes[path]=hashlib.sha256(source.read_bytes()).hexdigest();obj=BUILD/(source.stem+'.o')
 cmd=compiler.compiler('cflags_game')+['-c',path,'-o',str(obj)];commands.append(cmd)
 with (BUILD/(source.stem+'.compile.log')).open('w') as log:
  subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 assert hashes[path]==hashlib.sha256(source.read_bytes()).hexdigest()
 objects[source.stem]=reader.Elf(obj)
retail=ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/Game/Player/MarioActorParts.o'
cmd=['build/tools/objdiff-cli','diff','-1',str(retail),'-2',str(BUILD/'MarioActorParts.o'),'-o',str(BUILD/'objdiff.json'),'--format','json-pretty'];commands.append(cmd)
subprocess.run(cmd,cwd=ROOT,check=True)
name='updateSpecialModeAnimation__10MarioActorFv';address=0x802BCD08;size=0x1E0
addresses={n:int(v,16) for n,v in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
elf=objects['MarioActorParts'];_,start,compiled_size,section=next(s for s in elf.symbols if s[0]==name);assert compiled_size==size
code=bytearray(elf.section_data(section)[start:start+size]);refs=[]
for r in elf.references(name):
 r=r.copy();offset=int(r['offset'],16);kind=r['kind'];assert r['addend']==0
 target=0x805B8850 if r['symbol']=='...data.0' else addresses[r['symbol']]
 if kind==10:
  displacement=target-address-offset;assert -0x2000000<=displacement<0x2000000 and displacement%4==0
  word=struct.unpack_from('>I',code,offset)[0];struct.pack_into('>I',code,offset,(word&0xFC000003)|(displacement&0x3FFFFFC))
 elif kind in (4,6):
  assert r['symbol']=='...data.0' and offset%4==2
  struct.pack_into('>H',code,offset,(target if kind==4 else (target+0x8000)>>16)&65535)
 else:raise AssertionError(r)
 r['retail_target']=hex(target);refs.append(r)
# The compiler addresses strings as base + local offsets rather than emitting
# per-string ELF relocations. Validate their complete SJIS payloads on each side
# before normalizing those eight data-layout immediates. No other words change.
_,base,_,pool_section=next(s for s in elf.symbols if s[0]=='...data.0');pool=elf.section_data(pool_section)
STRINGS=((0x70,0xD1,'泥低速歩行'),(0x84,0xDC,'泥高速歩行'),(0x108,0xE7,'ハチ匍匐前進'),
         (0x11C,0xE7,'ハチ匍匐前進'),(0x130,0xE7,'ハチ匍匐前進'),(0x144,0xF4,'ハチ匍匐ウエイト'),
         (0x1A4,0x105,'鈍行'),(0x1B8,0x10A,'歩行'))
string_records=[]
for offset,retail_offset,string in STRINGS:
 word=struct.unpack_from('>I',code,offset)[0];assert word&0xFFFF0000==0x38BF0000 # addi r5,r31,imm
 local_offset=struct.unpack_from('>h',code,offset+2)[0];data=string.encode('shift_jis')+b'\0'
 assert pool[base+local_offset:base+local_offset+len(data)]==data
 assert reader.dol_bytes(DOL,0x805B8850+retail_offset,len(data))==data
 struct.pack_into('>H',code,offset+2,retail_offset)
 string_records.append({'instruction_offset':hex(offset),'compiled_pool_offset':hex(local_offset),'retail_pool_offset':hex(retail_offset),'text':string,'sjis_hex':data.hex()})
expected=reader.dol_bytes(DOL,address,size)
assert code==expected,[(hex(i),code[i:i+4].hex(),expected[i:i+4].hex()) for i in range(0,size,4) if code[i:i+4]!=expected[i:i+4]]
diff=json.loads((BUILD/'objdiff.json').read_text());symbol=next(s for s in diff['left']['symbols'] if s['name']==name)
assert symbol['match_percent']==99.6
mismatches=[x for x in symbol['instructions'] if x.get('diff_kind')];assert len(mismatches)==8
assert all(x['diff_kind']=='DIFF_ARG_MISMATCH' and x['instruction']['formatted'].startswith('addi r5, r31,') for x in mismatches)
# Actual source owning the chest symbol is the MarioAnimator split. The value
# is a writable pointer to const char, independently of host pointer width.
elf=objects['MarioAnimator'];_,start,chest_size,section=next(s for s in elf.symbols if s[0]=='jname_chest');assert chest_size==4
chest=bytearray(elf.section_data(section)[start:start+4]);chest_refs=elf.references('jname_chest')
assert len(chest_refs)==1 and chest_refs[0]['kind']==1 and chest_refs[0]['offset']=='0x0' and chest_refs[0]['addend']==0
assert bytes.fromhex(chest_refs[0]['value_hex'])==b'Spine1\0'
assert reader.dol_bytes(DOL,0x806B2288,4)==struct.pack('>I',0x805C3FDA)
assert reader.dol_bytes(DOL,0x805C3FDA,7)==b'Spine1\0'
struct.pack_into('>I',chest,0,0x805C3FDA);assert chest==reader.dol_bytes(DOL,0x806B2288,4)
original=reader.Elf(ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/Game/Player/MarioAnimator.o')
assert any(s[0]=='jname_chest' and s[2]==4 for s in original.symbols)
checks={'offsetof(MarioActor,mMario)':0x230,'offsetof(MarioActor,mMarioAnim)':0x234,'offsetof(MarioActor,mPlayerMode)':0x3D4,
        'offsetof(MarioActor,mBeeWallWalk)':0x9F1,'offsetof(MarioActor,_B96)':0xB96,'offsetof(Mario,mMovementStates)':8,
        'offsetof(Mario,_960)':0x960,'offsetof(Mario,_418)':0x418}
probe='#include "Game/Player/MarioActor.hpp"\n#include <stddef.h>\n'+'\n'.join(f'typedef char Check{i}[({expr}=={val})?1:-1];' for i,(expr,val) in enumerate(checks.items()))+'\n'
(BUILD/'layout.cpp').write_text(probe)
cmd=compiler.compiler('cflags_game')+['-c',str(BUILD/'layout.cpp'),'-o',str(BUILD/'layout.o')];commands.append(cmd)
with (BUILD/'layout.compile.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
report={'scope':'Root-only MarioActor::updateSpecialModeAnimation recovery and authentic MarioAnimator-owned chest data; no native activation.',
 'dol_sha1':hashlib.sha1(DOL).hexdigest(),'compiler':'GC3.0a3, real configure.py cflags_game, RMGK01 VERSION=0, real root headers',
 'source_sha256':hashes,'commands':commands,'original_layout':checks,
 'function':{'name':name,'address':hex(address),'size':size,'objdiff_percent':symbol['match_percent'],
 'all_120_instruction_words_equal_after_verified_relocations_and_string_pool_layout_normalization':True,
 'retail_sha256':hashlib.sha256(expected).hexdigest(),'relocations':refs,'local_string_offset_normalization':string_records},
 'chest':{'symbol':'jname_chest','owning_root_tu':'src/Game/Player/MarioAnimator.cpp','retail_address':'0x806b2288','retail_pointer':'0x805c3fda','value':'Spine1','pointer_bytes_equal_after_verified_ADDR32_relocation':True,'relocation':chest_refs[0]}}
(HERE/'source-evidence.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
print('PASS updateSpecialModeAnimation:99.6%; all120 instruction words agree after16 relocations and8 verified string-pool offsets')
print('PASS jname_chest: original owner and complete pointer/string data verified')
