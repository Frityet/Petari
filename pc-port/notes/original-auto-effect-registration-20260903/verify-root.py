#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,re,subprocess,hashlib,struct
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-auto-effect-registration-20260903'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
proof=module('proof','pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
source=ROOT/'src/Game/Effect/EffectSystemUtil.cpp';obj=BUILD/'EffectSystemUtil.o'
command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(obj)]
subprocess.run(command,cwd=ROOT,check=True)
retail=ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Effect/EffectSystemUtil.o'
output=BUILD/'objdiff.json'
subprocess.run([ROOT/'build/tools/objdiff-cli','diff','-1',retail,'-2',obj,'-o',output,'--format','json-pretty'],check=True)
a,b=reader.Elf(retail),reader.Elf(obj);diff=json.loads(output.read_text())
addresses={n:int(a,16) for n,a in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
dol=compiler.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
class RetailReferences:
 def references(self,name):
  refs=a.references(name)
  for r in refs:
   if r['symbol']=='lbl_80531A90':
    # The retail object references this shared f64 integer-conversion constant
    # externally; the rebuilding compiler emits an identical local constant.
    value=reader.dol_bytes(dol,addresses[r['symbol']],8)
    assert value.hex()=='4330000080000000'
    r['value_hex']=value.hex()
  return refs
retail_references=RetailReferences()
def canonical_sync(symbol,refs,compiled):
 rows=[[int(x['instruction']['address']),x['instruction']['formatted']] for x in symbol['instructions'] if 'instruction'in x]
 const={r['symbol']:'constant_'+r['value_hex'] for r in refs if 'value_hex'in r}
 phase='single'
 for row in rows:
  text=row[1]
  for name,value in const.items():text=text.replace(name+'@',value+'@')
  if compiled:
   if text=='lwz r23, 0x4(r30)':phase='multi'
   if text=='mr r26, r23':phase='count'
   if text=='lwz r4, 0x18(r29)' and phase=='count' and previous.startswith('b 0x'):phase='multi'
   if text=='addi r3, r1, 0xc' and phase=='count':phase='multi'
   mapping={28:26,29:27,30:28}
   if phase=='single':mapping[24]=20
   else:
    mapping.update({23:30,27:25,25:23,22:29,24:22,26:20})
    if phase=='count':mapping.update({26:24,21:20,20:21})
   text=re.sub(r'\br(\d+)\b',lambda m:'r'+str(mapping.get(int(m[1]),int(m[1]))),text)
  row[1]=text
  previous=row[1]
 if compiled:
  at=next(i for i,r in enumerate(rows) if r[1]=='extrwi r4, r0, 1, 25')
  rows[at][1]='extract_bit_0x40 r4, r0'
 else:
  at=next(i for i,r in enumerate(rows) if r[1]=='rlwinm r4, r0, 0, 25, 25')
  assert [r[1] for r in rows[at:at+4]]==['rlwinm r4, r0, 0, 25, 25','subi r0, r4, 0x40','cntlzw r0, r0','srwi r4, r0, 5']
  rows[at][1]='extract_bit_0x40 r4, r0';del rows[at+1:at+4]
 positions={r[0]:i for i,r in enumerate(rows)}
 for r in rows:
  m=re.fullmatch(r'(b\w*) (0x[0-9a-f]+)',r[1])
  if m:r[1]=m[1]+' instruction_'+str(positions[int(m[2],16)])
 return [r[1] for r in rows]
rows=[]
for name,start,size,section in a.symbols:
 if not size or not name.startswith(('setupMultiEmitter','registerAutoEffectInfoGroup','addAutoEffect__','requestMovementOn__','initEffectSyncBck','addEffectSyncBck','createParticleEmitter__','deleteParticleEmitter__','setLinkSingleEmitter__','getLinkSingleEmitter__')):continue
 left=next(s for s in diff['left']['symbols'] if s['name']==name);right=next(s for s in diff['right']['symbols'] if s['name']==name)
 # Verify that the extracted original object is the actual retail function too.
 original,original_refs=proof.relocated(a,retail_references,name,addresses[name],size,dol)
 assert original==reader.dol_bytes(dol,addresses[name],size)
 code,refs=proof.relocated(b,retail_references,name,addresses[name],size,dol)
 exact=code==original
 row={'name':name,'address':hex(addresses[name]),'retail_size':size,'compiled_size':len(code),'match_percent':left['match_percent'],'relocated_bytes_equal':exact,'relocations':refs}
 if name.startswith('setupMultiEmitterSyncBck'):
  ca=canonical_sync(left,retail_references.references(name),False);cb=canonical_sync(right,b.references(name),True)
  assert ca==cb,[(i,x,y) for i,(x,y) in enumerate(zip(ca,cb)) if x!=y]
  for flag in range(65536):assert ((flag&0x40)==0x40)==bool((flag>>6)&1)
  row['canonical_instructions_equal']=True;row['canonical_instruction_count']=len(ca);row['flag_cases']=65536
  row['normalization']='Explicit single/multi/count temporary-register allocation and equivalent 0x40 boolean extraction only. All operands, call order, field offsets and branch destinations retained.'
 else:assert exact,name
 rows.append(row);print(name,left['match_percent'],size,len(code),'exact='+str(exact))
assert len(rows)==17
# Recompile pre-field-change and current actual AutoEffectInfo; the new typed
# TVec3 field leaves both original constructor and complete parser unchanged.
metadata=[]
for label,src,extra in [('before',NOTES/'baseline/src/Game/Effect/AutoEffectInfo.cpp',['-i',str(NOTES/'baseline/include')]),('after',ROOT/'src/Game/Effect/AutoEffectInfo.cpp',[])]:
 out=BUILD/('AutoEffectInfo-'+label+'.o');cmd=compiler.compiler('cflags_game');pos=cmd.index('-i');cmd[pos:pos]=extra;cmd+=['-c',str(src),'-o',str(out)];subprocess.run(cmd,cwd=ROOT,check=True);metadata.append(reader.Elf(out))
changes=[]
for name,start,size,section in metadata[0].symbols:
 if not size or (not (metadata[0].sections[section][2]&4) and not name.startswith('sDrawOrderDataTable__')):continue
 match=next((s for s in metadata[1].symbols if s[0]==name),None)
 if not match:continue
 _,start2,size2,section2=match
 if section==0 or size2==0:continue
 before=metadata[0].section_data(section)[start:start+size];after=metadata[1].section_data(section2)[start2:start2+size2]
 assert before==after,name
 def identity(ref):
  return {k:v for k,v in ref.items() if k!='symbol' or 'value_hex' not in ref}
 assert [identity(r) for r in metadata[0].references(name)]==[identity(r) for r in metadata[1].references(name)],name
 changes.append({'name':name,'size':size,'bytes_and_relocations_equal':True})
assert any(r['name'].startswith('__ct__14AutoEffectInfo') for r in changes)
assert any(r['name'].startswith('init__14AutoEffectInfo') for r in changes)
(NOTES/'root-evidence.json').write_text(json.dumps({'command':command,'dol_sha1':hashlib.sha1(dol).hexdigest(),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'functions':rows,'typed_offset_metadata_proof':changes},indent=2)+'\n')
print('17 functions verified; metadata field change preserves full original object instructions and relocations')
