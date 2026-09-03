#!/usr/bin/env python3
"""Compile existing original material readers against current RMGK01."""
import hashlib, importlib.util, json, re, struct, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-j3d-material-table-20260903'
spec=importlib.util.spec_from_file_location('original', ROOT/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
BUILD.mkdir(parents=True,exist_ok=True)
source=ROOT/'src/JSystem/J3DGraphLoader/J3DModelLoader.cpp'
source_hash=original.sha(source)
command=original.compiler('cflags_jsys')+['-c',str(source),'-o',str(BUILD/'J3DModelLoader.o')]
with (BUILD/'compiler.log').open('w') as log:
 subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
assert original.sha(source)==source_hash
layout=BUILD/'layout.cpp'
layout.write_text('#include "JSystem/J3DGraphBase/J3DMaterial.hpp"\n'
                 'typedef char MaterialSize[(sizeof(J3DMaterial)==0x4C)?1:-1];\n'
                 'typedef char TevOrderSize[(sizeof(J3DTevOrderInfo)==4)?1:-1];\n')
with (BUILD/'layout.log').open('w') as log:
 subprocess.run(original.compiler('cflags_jsys')+['-c',str(layout),'-o',str(BUILD/'layout.o')],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
retail=ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/JSystem/J3DGraphLoader/J3DModelLoader.o'
subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(BUILD/'J3DModelLoader.o'),'-o',str(BUILD/'objdiff.json'),'--format','json-pretty'],check=True)
diff=json.loads((BUILD/'objdiff.json').read_text())
right={s['name']:s for s in diff['right']['symbols'] if int(s.get('size',0))}
functions=[]
for s in diff['left']['symbols']:
 name=s['name']
 if name.startswith(('readMaterial__18J3DModelLoader_v26','readMaterialTable__18J3DModelLoader_v26','readPatchedMaterial__','readMaterialDL__','modifyMaterial__','loadBinaryDisplayList__','loadMaterialTable__14J3DModelLoader')):
  assert name in right
  functions.append(dict(name=name,retail_size=int(s['size']),compiled_size=int(right[name]['size']),objdiff_percent=s.get('match_percent')))
  print(functions[-1])
assert len(functions)==8
spec=importlib.util.spec_from_file_location('reader',ROOT/'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
elf=reader.Elf(retail)
addresses={name:int(address,16) for name,address in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
dol=original.DOL.read_bytes()
for function in functions:
 name=function['name'];address=addresses[name]
 _,start,size,section=next(symbol for symbol in elf.symbols if symbol[0]==name)
 code=bytearray(elf.section_data(section)[start:start+size]);live=reader.dol_bytes(dol,address,size)
 refs=elf.references(name)
 for ref in refs:
  off=int(ref['offset'],16);kind=ref['kind'];target=addresses[ref['symbol']]+ref['addend']
  if kind==10:
   word=struct.unpack_from('>I',code,off)[0]
   struct.pack_into('>I',code,off,(word&0xfc000003)|((target-address-off)&0x3fffffc))
  elif kind in (4,6):
   half=target if kind==4 else (target+0x8000)>>16
   struct.pack_into('>H',code,off,half&65535)
  elif kind==109:
   word=struct.unpack_from('>I',code,off)[0];liveword=struct.unpack_from('>I',live,off)[0]
   base=(liveword>>16)&31;sda={0:0,2:0x806BFC20,13:0x806B9620}[base]
   delta=target-sda;assert -32768<=delta<32768
   struct.pack_into('>I',code,off,(word&~0x1fffff)|(base<<16)|(delta&65535))
  else:raise AssertionError(ref)
 assert code==live,(name,'retail object must reconstruct verified current DOL')
 function.update(retail_address=hex(address),retail_sha256=hashlib.sha256(live).hexdigest(),verified_live_instruction_words=size//4,verified_relocations=len(refs))
 left=next(symbol for symbol in diff['left']['symbols'] if symbol['name']==name)
 def calls(symbol):
  return [i['instruction']['formatted'] for i in symbol['instructions'] if i.get('instruction',{}).get('formatted','').startswith('bl ') and not i['instruction']['formatted'].startswith(('bl JSUConvertOffsetToPtr','bl _savegpr_','bl _restgpr_'))]
 function['calls_after_offset_inlining_and_register_save_helpers_equal']=calls(left)==calls(right[name])
 if name.startswith(('readMaterial__','readMaterialTable__','readPatchedMaterial__','readMaterialDL__','modifyMaterial__')):
  assert function['calls_after_offset_inlining_and_register_save_helpers_equal'], name
  a=[i.get('instruction',{}).get('formatted','') for i in left['instructions']]
  b=[i.get('instruction',{}).get('formatted','') for i in right[name]['instructions']]
  (HERE/(name.split('__')[0]+'.instructions.txt')).write_text('Retail | Original compiler\n'+'\n'.join(f'{x:<80} | {y}' for x,y in zip(a,b))+'\n')
assert hashlib.sha1(original.DOL.read_bytes()).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
(HERE/'compiler-evidence.json').write_text(json.dumps(dict(dol_sha1=hashlib.sha1(original.DOL.read_bytes()).hexdigest(),source_sha256={str(source.relative_to(ROOT)):source_hash},command=command,functions=functions,scope='Existing root original readers and binary/table entrypoint algorithms; native component is a typed lifetime adapter, not a binary-matching loader class.'),indent=2)+'\n')
