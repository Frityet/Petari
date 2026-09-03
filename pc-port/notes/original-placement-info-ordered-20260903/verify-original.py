#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,re,hashlib,struct,copy
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-placement-info-ordered-20260903'
s=importlib.util.spec_from_file_location('stageproof',R/'pc-port/notes/original-stage-data-holder-20260903/verify-original.py');v=importlib.util.module_from_spec(s);s.loader.exec_module(v)
class Elf(v.reader.Elf):
 def section_data(self,i):
  q=self.sections[i];return bytes(q[5]) if q[1]==8 else super().section_data(i)
 def code(self,name):
  _,start,size,index=next(s for s in self.symbols if s[0]==name);return bytearray(self.section_data(index)[start:start+size])
 def references(self,name):
  refs=super().references(name)
  for ref in refs:
   target=next(q for q in self.symbols if q[0]==ref['symbol'])
   if target[3] and self.sections[target[3]][2]&4:ref.pop('value_hex',None)
   if ref.get('value_hex')=='0000000000000000':
    assert name=='attach__20PlacementInfoOrderedFPC8JMapInfoP20PlacementInfoOrdered' and ref['offset']=='0x3c'
    ref['value_hex']='00'
  return refs

def main():
 B.mkdir(parents=True,exist_ok=True);ret=R/'build/original-collision-owner-20260903/retail/obj/Game/Scene/PlacementInfoOrdered.o';obj=B/'PlacementInfoOrdered.o';cmd=v.c.compiler('cflags_game')+['-c',str(R/'src/Game/Scene/PlacementInfoOrdered.cpp'),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True)
 subprocess.run([R/'build/tools/objdiff-cli','diff','-1',ret,'-2',obj,'-o',B/'diff.json','--format','json-pretty'],check=True,stdout=subprocess.DEVNULL)
 a,b=Elf(ret),Elf(obj);j=json.loads((B/'diff.json').read_text());dol=v.c.DOL.read_bytes();rows=[]
 for left in j['left']['symbols']:
  if not left.get('instructions') or 'match_percent' not in left:continue
  name=left['name'];right=next(s for s in j['right']['symbols'] if s['name']==name)
  if not right.get('instructions'):raise AssertionError(name)
  sides=[copy.deepcopy(left),copy.deepcopy(right)];changes=[]
  for side in sides:
   for ins in side['instructions']:
    if 'instruction' in ins:ins['instruction'].setdefault('address','0')
  if name=='__dt__Q220PlacementInfoOrdered5IndexFv':
   rows0=[i for i in sides[0]['instructions'] if 'instruction' in i]
   assert rows0[8]['instruction']['formatted'].strip().startswith('beq ') and rows0[9]['instruction']['formatted'].strip().startswith('beq ')
   assert rows0[2]['instruction']['formatted'].strip()=='cmpwi r3, 0x0'
   # Both consecutive branches test the same CR0 set by the initial this
   # comparison. The first exits if null; the second can never be taken.
   sides[0]['instructions'].remove(rows0[9]);changes.append('Remove one retail unreachable duplicate beq after the preceding identical-condition null-this exit; no CR write occurs between them.')
  canon=[v.proof.normalize(side,elf.references(name),'placement',bool(i)) for i,(side,elf) in enumerate(zip(sides,(a,b)))]
  if name=='requestFileLoad__20PlacementInfoOrderedFv':
   mapping={27:29,28:27,29:28};canon[0]=[re.sub(r'\br(\d+)\b',lambda m:'r'+str(mapping.get(int(m[1]),int(m[1]))),x) for x in canon[0]];changes.append('Bijective callee-saved GPR mapping r27/r28/r29 -> r29/r27/r28.')
  if name=='initPlacement__20PlacementInfoOrderedFv':
   mapping={24:28,25:27,26:29,27:26,29:25}
   for i,x in enumerate(canon[0]):
    # Retail r28 holds the group through instruction22 and is then dead;
    # from instruction34 it holds the newly created NameObj. Native reuses
    # the retired group register for the name and r24 for the NameObj.
    def change(m):
     reg=int(m[1]);return 'r'+str(24 if reg==28 and i>=34 else mapping.get(reg,reg))
    canon[0][i]=re.sub(r'\br(\d+)\b',change,x)
   changes.append('Explicit disjoint GPR lifetimes: name r24->r28, creator r25->r27, loop r26->r29, link r27->r26, iter r29->r25; group r28 unchanged through22, new actor r28->r24 from34.')
  if name=='getUsedArrayNum__20PlacementInfoOrderedCFv':
   expected={'addi r8, r8, 0x1','addi r4, r4, 0x4','addi r7, r7, 0x1'}
   for side in canon:
    assert set(side[5:8])==expected;side[5:8]=sorted(side[5:8])
   changes.append('Reorder three independent integer counter/address increments; their source and destination registers are pairwise distinct.')
  assert canon[0]==canon[1],(name,[(i,x,y) for i,(x,y) in enumerate(zip(*canon)) if x!=y])
  exact=not changes
  refs=[]
  if exact:refs=v.relocate(b,a,name,v.addresses[name],dol)
  else:
   ar=a.references(name);br=b.references(name)
   def targets(refs):return [(q['kind'],q.get('value_hex',q['symbol']),q['addend']) for q in refs]
   assert targets(ar)==targets(br),name
   refs=br
  rows.append({'symbol':name,'retail_address':hex(v.addresses[name]),'retail_bytes':left['size'],'compiled_bytes':right['size'],'match_percent':left['match_percent'],'all_canonical_instructions_equal':True,'relocated_bytes_equal':exact,'normalization':changes,'references':refs,'canonical':canon[1]});print(name,left['match_percent'],'canonical exact', 'relocated exact' if exact else '')
 files=['src/Game/Scene/PlacementInfoOrdered.cpp','include/Game/Scene/PlacementInfoOrdered.hpp','src/Game/Util/BothDirList.cpp','include/Game/Util/BothDirList.hpp']
 (N/'compiler-evidence.json').write_text(json.dumps({'source_sha256':{p:hashlib.sha256((R/p).read_bytes()).hexdigest() for p in files},'compiler_command':cmd,'functions':rows},indent=2)+'\n')
if __name__=='__main__':main()
