#!/usr/bin/env python3
"""Verify shared bitfield layout against untouched Wii declarations and native masks."""
from pathlib import Path
import hashlib,json,runpy,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
result={'commands':[]}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(cmd,cwd,name,success=True):
 p=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True)
 (OUT/(name+'.log')).write_text(p.stdout+p.stderr)
 result['commands'].append({'name':name,'command':cmd,'exit_code':p.returncode,'expected_success':success})
 assert (p.returncode==0)==success,p.stdout+p.stderr
 return p
wii=json.loads((ROOT/'notes/mario-actor-movement-restoration-20260907/wii-compile-command.json').read_text())['command']
for mode in ['baseline','adapted']:
 c=list(wii);c[c.index('-c')+1]=str(OUT/'wii-fields.cpp');c[c.index('-o')+1]=str(OUT/(mode+'.o'))
 if mode=='adapted':
  i=c.index('-i');c[i:i]=['-i',str(OUT/'wii-overlay')];c.extend(['-i',str(ROOT/'aurora/include')])
 run(c,ROOT/'decomp',mode+'.wii')
Elf=runpy.run_path(str(ROOT/'notes/mario-update-restoration-20260903/verify-object.py'))['Elf']
a=Elf(OUT/'baseline.o');b=Elf(OUT/'adapted.o');proof=[]
for name,value,size,section in a.symbols:
 if not name.startswith(('set_','get_')) or not section:continue
 _,bv,bs,bi=next(x for x in b.symbols if x[0]==name)
 ab=a.section_data(section)[value:value+size];bb=b.section_data(bi)[bv:bv+bs]
 proof.append({'symbol':name,'size':size,'adapted_size':bs,'bytes_equal':ab==bb,'sha256':hashlib.sha256(ab).hexdigest()})
assert len(proof)==248 and all(x['bytes_equal'] for x in proof)
(OUT/'wii-equivalence.json').write_text(json.dumps(proof,indent=2)+'\n')
result['wii_equal_accessors']=len(proof)
base=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-Isrc','-Iaurora/include','-DTARGET_PC','-DAURORA','-include','src/compat/MetrowerksStdCompat.hpp']
for opt in ['O0','O2']:
 exe=OUT/('native-tests-'+opt)
 run(base+['-'+opt,'tests/PpcBitfieldAbiTests.cpp','-o',str(exe)],ROOT,'native-'+opt+'-build')
 run([str(exe)],ROOT,'native-'+opt+'-run')
for name,fields in [('underfilled','(first, 1), (rest, 30)'),('overfilled','(first, 1), (rest, 32)'),('zero-width','(, 0), (rest, 32)')]:
 path=OUT/('invalid-'+name+'.cpp')
 path.write_text('#include <aurora/ppc_bitfield.hpp>\nstruct Bad { AURORA_PPC_BITFIELD_GROUP(unsigned, '+fields+') };\n')
 run(['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-Iaurora/include','-fsyntax-only',str(path)],ROOT,'invalid-'+name,False)
result['sources']=[{'path':p,'sha256':sha(ROOT/p)} for p in ['aurora/include/aurora/ppc_bitfield.hpp','src/Game/Player/Mario.hpp','src/Game/Player/J3DModelX.hpp','tests/PpcBitfieldAbiTests.cpp','decomp/include/Game/Player/Mario.hpp','decomp/include/Game/Player/J3DModelX.hpp']]
(OUT/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS: 248/248 Wii accessor bytes equal; native O0/O2; invalid storage rejected')
