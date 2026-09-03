#!/usr/bin/env python3
"""Prove the root-only original start-count forwarding recovery."""
import hashlib,importlib.util,json,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-camera-registration-20260903'
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
source=ROOT/'src/Game/Util/SceneUtil.cpp';output=BUILD/'SceneUtil.o'
command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(output)]
BUILD.mkdir(parents=True,exist_ok=True)
with (BUILD/'compile.log').open('w') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
retail=ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/Game/Util/SceneUtil.o'
subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(output),'-o',str(BUILD/'objdiff.json'),'--format','json-pretty'],check=True)
name='getStartPosNum__2MRFv';address=0x803F757C
elf=reader.Elf(output);_,start,size,section=next(s for s in elf.symbols if s[0]==name)
assert size==0x24
code=bytearray(elf.section_data(section)[start:start+size]);references=elf.references(name)
addresses={name:int(value,16) for name,value in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
for ref in references:
 assert ref['kind']==10 and ref['addend']==0
 offset=int(ref['offset'],16);word=struct.unpack_from('>I',code,offset)[0]
 struct.pack_into('>I',code,offset,(word&0xfc000003)|((addresses[ref['symbol']]-address-offset)&0x3fffffc))
dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
assert code==reader.dol_bytes(dol,address,size)
assert [r['symbol'] for r in references]==['getStageDataHolder__2MRFv','getStartPosNum__15StageDataHolderCFv']
diff=json.loads((BUILD/'objdiff.json').read_text());symbol=next(s for s in diff['left']['symbols'] if s['name']==name)
assert symbol['match_percent']==100
report={'scope':'Root-only MR::getStartPosNum recovery; no native import/activation.', 'compiler':'GC/3.0a3 with configure.py cflags_game, VERSION=0','command':command,'dol_sha1':hashlib.sha1(dol).hexdigest(),'root_source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'symbol':name,'address':hex(address),'retail_and_compiled_size':size,'objdiff_percent':symbol['match_percent'],'relocated_compiled_bytes_equal_current_dol':True,'verified_instruction_words':size//4,'references':references}
(HERE/'recovery-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print('[pass] MR::getStartPosNum: 100%; all 9 relocated compiled instructions equal current RMGK01 DOL')
