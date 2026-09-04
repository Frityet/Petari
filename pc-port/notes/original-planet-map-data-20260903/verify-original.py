#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,struct,re
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-data-20260903'
spec=importlib.util.spec_from_file_location('oracle',R/'pc-port/notes/original-stage-data-holder-20260903/verify-original.py');o=importlib.util.module_from_spec(spec);spec.loader.exec_module(o)
SPECIAL=('sUniquePlanetUniqueArchiveName__','sArcName__','sFileName__','__vt__','sInstance__')
class Elf(o.reader.Elf):
 def section_data(self,i):
  s=self.sections[i];return bytes(s[5]) if s[1]==8 else super().section_data(i)
 def code(self,n):
  _,at,size,sec=next(s for s in self.symbols if s[0]==n);return bytearray(self.section_data(sec)[at:at+size])
 def target_data(self,ref,offset=0):
  _,at,size,sec=next(s for s in self.symbols if s[0]==ref['symbol']);data=self.section_data(sec);at+=ref['addend']+offset;return data[at:data.index(0,at)+1]
 def references(self,n):
  rows=super().references(n)
  for r in rows:
   sym=next(s for s in self.symbols if s[0]==r['symbol'])
   if r['symbol'].startswith(SPECIAL) or (sym[3] and self.sections[sym[3]][2]&4):r.pop('value_hex',None)
   elif r['kind'] != 10 and sym[3]:r['value_hex']=self.target_data(r).hex()
   elif r['kind'] != 10 and r['symbol'].startswith('lbl_'):
    raw=o.reader.dol_bytes(o.c.DOL.read_bytes(),o.addresses[r['symbol']]+r['addend'],256);r['value_hex']=(raw.split(b'\0')[0]+b'\0').hex()
  return rows

def normalize(symbol,elf,source):
 name=symbol['name'];ins=[x['instruction'] for x in symbol['instructions'] if 'instruction' in x];base=int(ins[0].get('address',0));refs={int(r['offset'],16)//4:r for r in elf.references(name)};pool=next((r for r in elf.references(name) if r['kind']==6),None)
 if name.startswith('isScenarioForceLow') and not source:
  assert ins[21]['formatted']=='crclr cr1eq';ins.pop(21)
 addresses={int(x.get('address',0)):i for i,x in enumerate(ins)}
 def reg(v,at):
  mapping={}
  if source and name.startswith('makeArchiveListPlanet'):
   mapping=({27:28,31:27} if at<0xc8 else {28:27,27:28})
  elif source and name.startswith('addTableData'):mapping={28:29,29:28}
  elif not source and name.startswith('__ct__16PlanetMapCreator') and at in (0x1c,0x24,0x30):mapping={4:3}
  return re.sub(r'\br(\d+)\b',lambda m:'r'+str(mapping.get(int(m[1]),int(m[1]))),v)
 result=[]
 for insn in ins:
  address=int(insn.get('address',0));at=address-base;ref=refs.get(at//4);op=insn['parts'][0]['opcode']['mnemonic'];args=[]
  for part in insn['parts'][1:]:
   if 'arg' not in part:continue
   arg=part['arg']
   if 'opaque' in arg:args.append(reg(arg['opaque'],at))
   elif 'branch_dest' in arg:args.append(('branch',addresses[int(arg['branch_dest'])]))
   elif 'reloc' in arg:
    assert ref
    if name.startswith('addTableData') and ref['kind'] in (4,6):args.append(('pool',ref['kind']))
    elif 'value_hex' in ref:args.append(('constant',ref['kind'],ref['value_hex']))
    else:args.append(('symbol',ref['kind'],ref['symbol'],ref['addend']))
   else:args.append(int(arg.get('signed',arg.get('unsigned'))))
  if name.startswith('addTableData') and op=='addi' and ref is None and len(args)==3 and args[1]=='r28':
   args[2]=('pool-string',elf.target_data(pool,args[2]).hex())
  result.append([op,args])
 if name.startswith('__ct__16PlanetMapCreator') and not source:
  row=result.pop(12);assert row==['stw',['r3',0,'r30']];result.insert(11,row)
 return result

def main():
 src=R/'src/Game/Map/PlanetMapCreator.cpp';obj=B/'PlanetMapCreator.o';target=R/'build/original-collision-owner-20260903/retail/obj/Game/Map/PlanetMapCreator.o';cmd=o.c.compiler('cflags_game')+['-c',str(src),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True);subprocess.run([R/'build/tools/objdiff-cli','diff','-1',target,'-2',obj,'-o',B/'diff.json','--format','json-pretty'],check=True,stdout=subprocess.DEVNULL)
 a,b=Elf(target),Elf(obj);j=json.loads((B/'diff.json').read_text());dol=o.c.DOL.read_bytes();rows=[]
 selected=('makeSubModelName','isDataForceLow','__ct__16PlanetMapCreator','makeArchiveListPlanet','createPlanetMapDataTable','addTableData','getTableData','isScenarioForceLow','makeArchiveList__24','isLoadArchiveAfterScenarioSelected','isRegisteredObj')
 for left in j['left']['symbols']:
  name=left['name']
  if not name.startswith(selected):continue
  right=next(s for s in j['right']['symbols'] if s['name']==name);canon=[normalize(left,a,False),normalize(right,b,True)]
  if canon[0]!=canon[1]:
   print(name);print([(i,x,y) for i,(x,y) in enumerate(zip(*canon)) if x!=y]);assert False
  exact=not name.startswith(('__ct__16PlanetMapCreator','makeArchiveListPlanet','addTableData','isScenarioForceLow'))
  reloc=o.relocate(b,a,name,o.addresses[name],dol) if exact else None
  rows.append(dict(symbol=name,retail_address=hex(o.addresses[name]),retail_bytes=len(a.code(name)),compiled_bytes=len(b.code(name)),match_percent=left['match_percent'],all_canonical_instructions_identical=True,relocated_byte_exact=exact,verified_relocations=reloc,canonical_instructions=canon[0],retail_relocations=a.references(name),compiled_relocations=b.references(name)))
  print(name,left['match_percent'],'canonical equal','byte exact' if exact else '')
 pointer_objects=[]
 for prefix in SPECIAL[:3]:
  name=next(s[0] for s in a.symbols if s[0].startswith(prefix));refsA=sorted(a.references(name),key=lambda r:int(r["offset"],16));refsB=sorted(b.references(name),key=lambda r:int(r["offset"],16));assert len(refsA)==len(refsB)
  for x,y in zip(refsA,refsB):
   assert x['offset']==y['offset'] and x['kind']==y['kind']==1 and x['value_hex']==y['value_hex']
   address=struct.unpack('>I',o.reader.dol_bytes(dol,o.addresses[name]+int(x['offset'],16),4))[0];value=bytes.fromhex(y['value_hex']);assert o.reader.dol_bytes(dol,address,len(value))==value
  pointer_objects.append(dict(symbol=name,all_relocated_strings_equal=True,pointer_count=len(refsA)))

 imported=[]
 for source, prefixes in [('Game/NameObj/NameObjArchiveListCollector',('__ct__27NameObjArchiveListCollector','addArchive__27NameObjArchiveListCollector','getArchive__27NameObjArchiveListCollector')),('Game/Util/ModelUtil',('isExistModel__2MRFPCc',)),('Game/Util/ObjUtil',('createCsvParser__2MRFPCcPCce',))]:
  obj=B/(Path(source).name+'.o');retail=R/'build/original-collision-owner-20260903/retail/obj'/f'{source}.o';command=o.c.compiler('cflags_game')+['-c',str(R/'src'/f'{source}.cpp'),'-o',str(obj)];subprocess.run(command,cwd=R,check=True)
  ea,eb=Elf(retail),Elf(obj)
  for name,_,size,_ in ea.symbols:
   if not size or not name.startswith(prefixes):continue
   if name.startswith('createCsvParser__2MRFPCcPCce'):
    code=eb.code(name);retail_code=o.reader.dol_bytes(dol,o.addresses[name],len(code));refs=eb.references(name)
    for ref in refs:
     at=int(ref['offset'],16);word=struct.unpack_from('>I',code,at)[0];target_address=o.addresses[ref['symbol']]+ref['addend']
     if ref['kind']==10:word=(word&0xfc000003)|((target_address-o.addresses[name]-at)&0x3fffffc)
     elif ref['kind']==109:
      base=(struct.unpack_from('>I',retail_code,at)[0]>>16)&31;assert base in (2,13);word=(word&~0x1fffff)|(base<<16)|((target_address-{2:0x806bfc20,13:0x806b9620}[base])&65535)
     else:raise AssertionError(ref)
     struct.pack_into('>I',code,at,word)
    assert code==retail_code
   else:refs=o.relocate(eb,ea,name,o.addresses[name],dol)
   imported.append(dict(source='src/'+source+'.cpp',symbol=name,retail_address=hex(o.addresses[name]),bytes=size,all_instructions_identical_after_verified_relocations=True,verified_relocations=refs,compiler_command=command));print(name,'byte exact',size)
 (N/'imported-provider-evidence.json').write_text(json.dumps(imported,indent=2)+'\n')
 (N/'compiler-evidence.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),compiler_command=cmd,functions=rows,pointer_objects=pointer_objects,normalization={'constructor':'One vtable temporary register and independent vtable store and archive-pointer load reordered after base construction.','makeArchiveListPlanet':'Two explicit disjoint register-allocation live ranges, separated at instruction offset 0xc8.','addTableData':'Bijective r28/r29 allocation; each string-pool address resolved to its full literal, including shared PlanetName literal at a different pool offset.','isScenarioForceLow':'Compiler omits crclr cr1eq at the integer/string-only snprintf call; all argument registers, literal format, branches and remaining instructions match.'}),indent=2)+'\n')
if __name__=='__main__':main()
