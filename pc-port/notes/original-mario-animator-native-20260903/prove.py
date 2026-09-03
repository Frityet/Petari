from pathlib import Path
import importlib.util,json,subprocess,hashlib
ROOT=Path(__file__).resolve().parents[3];BUILD=ROOT/'build/original-mario-animator-native-20260903';BUILD.mkdir(parents=True,exist_ok=True)

def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
commands=[]
for label,source in (('root','src/Game/Player/MarioAnimator.cpp'),('typed','build/original-mario-animator-native-20260903/staged/MarioAnimator.cpp')):
 cmd=compiler.compiler('cflags_game')+['-c',source,'-o',str(BUILD/(label+'-ppc.o'))];commands.append(cmd)
 with (BUILD/(label+'-ppc.log')).open('w') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)


def func(elf,name):
 _,start,size,section=next(s for s in elf.symbols if s[0]==name)
 return elf.section_data(section)[start:start+size],elf.references(name)
a=reader.Elf(BUILD/'root-ppc.o');b=reader.Elf(BUILD/'typed-ppc.o');rows=[]
for name in ('calc__13MarioAnimatorFv','updateJointRumble__13MarioAnimatorFv'):
 left,lref=func(a,name);right,rref=func(b,name);normalize=lambda refs:[{k:v for k,v in r.items() if k!='symbol' or not r['symbol'].startswith('@')} for r in refs]
 assert left==right and normalize(lref)==normalize(rref),name
 rows.append({'name':name,'size':len(left),'root_and_adapted_ppc_instructions_and_relocations_identical':True,'references':lref})
probe='#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"\n#include <stddef.h>\ntypedef char Verify[(offsetof(J3DJoint,mMtxCalc)==0x54)?1:-1];\n'
(BUILD/'layout-ppc.cpp').write_text(probe)
cmd=compiler.compiler('cflags_game')+['-c',str(BUILD/'layout-ppc.cpp'),'-o',str(BUILD/'layout-ppc.o')];commands.append(cmd)
with (BUILD/'layout-ppc.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
json.dump({'commands':commands,'methods':rows,'root_joint_mMtxCalc_offset':84},open(BUILD/'adaptation-proof.json','w'),indent=2)
print('PASS original compiler typed-joint and lexical-scope changes produce identical code/relocations; original field offset0x54')
