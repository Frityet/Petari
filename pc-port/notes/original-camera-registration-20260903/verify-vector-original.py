#!/usr/bin/env python3
"""Compile original root math and compare every selected instruction to retail."""
import hashlib,importlib.util,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-camera-registration-20260903/math';BUILD.mkdir(parents=True,exist_ok=True)
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
units=[('JMath','src/JSystem/JMath/JMath.cpp','cflags_jsys','JSystem/JMath/JMath.o', [('JMAVECScaleAdd__FPC3VecPC3VecP3Vecf',0x80442858,0x24)]),
 ('vec','src/RVL_SDK/mtx/vec.c','cflags_sdk','RVL_SDK/mtx/vec.o',[('PSVECDotProduct',0x804B911C,0x20),('PSVECCrossProduct',0x804B913C,0x3C)])]
checks=[];commands=[]
for unit,source,flags,retail,names in units:
 output=BUILD/(unit+'-original.o');command=compiler.compiler(flags)+['-c',str(ROOT/source),'-o',str(output)];commands.append(command)
 with (BUILD/(unit+'-original.log')).open('w') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 target=ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj'/retail
 diffpath=BUILD/(unit+'-original-objdiff.json')
 subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(target),'-2',str(output),'-o',str(diffpath),'--format','json-pretty'],check=True)
 diff=json.loads(diffpath.read_text());elf=reader.Elf(output)
 for name,address,size in names:
  _,start,actual_size,section=next(s for s in elf.symbols if s[0]==name)
  assert actual_size==size
  assert not elf.references(name)
  code=elf.section_data(section)[start:start+size]
  assert code==reader.dol_bytes(dol,address,size),(name,'retail mismatch')
  percent=next(s for s in diff['left']['symbols'] if s['name']==name)['match_percent'];assert percent==100
  checks.append({'symbol':name,'address':hex(address),'bytes':size,'objdiff_percent':percent,'all_instruction_bytes_equal_current_retail':True})
  print('[pass]',name,'100%; exact',size//4,'retail instructions')
# The actual TVec dot uses different FPR allocation and one reordered load.
# Its operation graph and values are checked by verify-vector-native.py.
report={'dol_sha1':hashlib.sha1(dol).hexdigest(),'compiler':'GC/3.0a3 with configured per-library flags','commands':commands,
 'root_source_sha256':{source:hashlib.sha256((ROOT/source).read_bytes()).hexdigest() for _,source,_,_,_ in units},
 'checks':checks,'original_tvec_dot':{'address':'0x8001D2A8','bytes':32,'difference':'FPR allocation and load scheduling; operation graph verified separately'},
 'scope':'These existing root bodies were not edited. Native implementations translate their exact paired operations; native behavior is checked separately. TVec angle/orientation/orthogonalize preserve the existing root method expressions.'}
(HERE/'vector-original-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
