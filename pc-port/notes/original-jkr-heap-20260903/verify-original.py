#!/usr/bin/env python3
"""Original GC3.0a3 ExpHeap recovery proof, including both live SDA bases."""
import ast,difflib,hashlib,importlib.util,json,re,shlex,struct,subprocess,types
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
NOTES=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-jkr-heap-20260903'
TARGETS=ROOT/'build/xanime-core-matrix-calculation-20260903/entrypoints/retail/obj'
spec=importlib.util.spec_from_file_location('reader',ROOT/'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def run(command,log):
 with (NOTES/log).open('w') as output:subprocess.run(command,cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True,timeout=60)
def main():
 dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes()
 assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
 for node in ast.parse((ROOT/'configure.py').read_text()).body:
  if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cflags_jsys' for t in node.targets):
   flags=eval(compile(ast.Expression(node.value),'configure.py','eval'),{'config':types.SimpleNamespace(version='RMGK01'),'version_num':0});break
 cmd=['build/tools/wibo','build/tools/sjiswrap.exe','build/compilers/GC/3.0a3/mwcceppc.exe']
 for flag in flags:cmd.extend(shlex.split(flag))
 cmd+=['-c','src/JSystem/JKernel/JKRExpHeap.cpp','-o',str(BUILD/'JKRExpHeap.o')]
 (NOTES/'original-command.json').write_text(json.dumps(cmd,indent=2)+'\n')
 run(cmd,'original-compile.log')
 addresses={name:int(address,16) for name,address in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
 records=[]
 for target_path,diff_name,only in [('JSystem/JKernel/JKRExpHeap.o','objdiff',None),('Game/System/Overwrite.o','adjust-objdiff','adjustSize__10JKRExpHeapFv')]:
  target=TARGETS/target_path; elf=reader.Elf(target)
  run(['build/tools/objdiff-cli','diff','-1',str(target),'-2',str(BUILD/'JKRExpHeap.o'),'-o',str(BUILD/(diff_name+'.json')),'--format','json-pretty'],diff_name+'.log')
  diff=json.loads((BUILD/(diff_name+'.json')).read_text())
  for name,start,size,section in elf.symbols:
   if only and name!=only:continue
   if not (0<section<len(elf.sections) and elf.sections[section][2]&4 and size):continue
   address=addresses[name]
   code=bytearray(elf.section_data(section)[start:start+size]);retail=reader.dol_bytes(dol,address,size)
   refs=elf.references(name)
   for ref in refs:
    off=int(ref['offset'],16);kind=ref['kind'];target_address=addresses[ref['symbol']]+ref['addend']
    if kind==10:
     word=struct.unpack_from('>I',code,off)[0]
     struct.pack_into('>I',code,off,(word&0xfc000003)|((target_address-address-off)&0x3fffffc))
    elif kind in (4,6):
     half=target_address if kind==4 else (target_address+0x8000)>>16
     struct.pack_into('>H',code,off,half&65535)
    elif kind==109:
     word=struct.unpack_from('>I',code,off)[0];live=struct.unpack_from('>I',retail,off)[0]
     base=(live>>16)&31;sda={0:0,2:0x806BFC20,13:0x806B9620}[base]
     delta=target_address-sda;assert -32768<=delta<32768,(name,ref,hex(target_address),base)
     struct.pack_into('>I',code,off,(word&~0x1fffff)|(base<<16)|(delta&65535))
    else:raise AssertionError(ref)
   assert code==retail,(name,'retail split does not reproduce live DOL after relocations')
   left=next(s for s in diff['left']['symbols'] if s['name']==name)
   right=next(s for s in diff['right']['symbols'] if s['name']==name)
   assert left['match_percent']>=96.0,(name,left['match_percent'])
   assert int(right['size'])==size,(name,'instruction count differs')
   record={'symbol':name,'retail_address':hex(address),'retail_size':size,'compiled_size':int(right['size']),
    'objdiff_percent':left['match_percent'],'retail_sha256':hashlib.sha256(retail).hexdigest(),
    'verified_live_instruction_words':size//4,'verified_relocations':len(refs)}
   records.append(record)
   print(f"{name}: {left['match_percent']:.6f}% ({size} bytes)")
 sources=['src/JSystem/JKernel/JKRExpHeap.cpp','libs/JSystem/include/JSystem/JKernel/JKRExpHeap.hpp',
  'libs/JSystem/include/JSystem/JKernel/JKRHeap.hpp']
 report={'compiler':'GC/3.0a3, configure.py cflags_jsys, VERSION=0',
  'dol_sha1':hashlib.sha1(dol).hexdigest(),'root_sha256':{p:sha(ROOT/p) for p in sources},
  'compiler_sha256':sha(ROOT/'build/compilers/GC/3.0a3/mwcceppc.exe'),
  'compiled_object_sha256':sha(BUILD/'JKRExpHeap.o'),'functions':records,
  'limit':'Percentages compare the original compiler with retail. Native pointer-width/layout changes are not PowerPC matches.',
  'reference':json.loads((BUILD/'external-reference.json').read_text())}
 (NOTES/'original-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
 root=(ROOT/sources[0]).read_text();native=(ROOT/'pc-port/src/compat/JKRExpHeapCompat.cpp').read_text()
 (NOTES/'native-exp-architecture.patch').write_text(''.join(difflib.unified_diff(root.splitlines(True),native.splitlines(True),fromfile=sources[0],tofile='pc-port/src/compat/JKRExpHeapCompat.cpp')))
 edits=[]
 matcher=difflib.SequenceMatcher(a=root,b=native,autojunk=False)
 for kind,a,b,c,d in matcher.get_opcodes():
  if kind!='equal':edits.append({'root_start':a,'root_end':b,'before':root[a:b],'after':native[c:d]})
 (NOTES/'native-exp-edits.json').write_text(json.dumps(edits,indent=2)+'\n')
 print(f'{len(records)} methods; {sum(x["verified_live_instruction_words"] for x in records)} live retail instruction words verified')
if __name__=='__main__':main()
