#!/usr/bin/env python3
"""Original compiler, root/native body correspondence, and retail constants."""
import ast, hashlib, json, re, shlex, struct, subprocess, types
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-j3d-mtx-buffer-20260903'

def body(text,marker):
 start=text.index(marker);at=text.index('{',start)+1;end=at;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
def normal(text):return re.sub(r'\s+','',re.sub(r'//[^\n]*|/\*.*?\*/','',text,flags=re.S))
root_mtx=(ROOT/'src/JSystem/J3DGraphAnimator/J3DMtxBuffer.cpp').read_text();native_mtx=(ROOT/'pc-port/src/compat/J3DMtxBufferCompat.cpp').read_text()
methods=['initialize','create','createAnmMtx','createWeightEnvelopeMtx','setNoUseDrawMtx','createDoubleDrawMtx','createBumpMtxArray','calcWeightEnvelopeMtx','calcDrawMtx','calcNrmMtx','calcBBoardMtx']
for name in methods:
 marker='J3DMtxBuffer::'+name+'(';assert normal(body(root_mtx,marker))==normal(body(native_mtx,marker)),name
assert normal(body(root_mtx,'void J3DCalcViewBaseMtx('))==normal(body(native_mtx,'void J3DCalcViewBaseMtx('))
assert (ROOT/'libs/JSystem/include/JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp').read_bytes()==(ROOT/'pc-port/src/JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp').read_bytes()
root_transform=(ROOT/'src/JSystem/J3DGraphBase/J3DTransform.cpp').read_text();native_transform=(ROOT/'pc-port/src/compat/J3DTransformMtxCompat.cpp').read_text()
helpers=['J3DCalcBBoardMtx','J3DCalcYBBoardMtx','J3DPSCalcInverseTranspose','J3DScaleNrmMtx','J3DScaleNrmMtx33','J3DPSMtxArrayConcat']
for name in helpers:
 marker='\nvoid '+name+'(';assert normal(body(root_transform,marker))==normal(body(native_transform,marker)),name
assert normal(body((ROOT/'src/JSystem/J3DGraphBase/J3DShapeMtx.cpp').read_text(),'void J3DPSMtx33CopyFrom34('))==normal(body(native_transform,'void J3DPSMtx33CopyFrom34('))
for node in ast.parse((ROOT/'configure.py').read_text()).body:
 if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cflags_jsys' for t in node.targets):
  flags=eval(compile(ast.Expression(node.value),'configure.py','eval'),{'config':types.SimpleNamespace(version='RMGK01'),'version_num':0});break
base=['build/tools/wibo','build/tools/sjiswrap.exe','build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags:base.extend(shlex.split(flag))
units={'J3DMtxBuffer':'J3DGraphAnimator','J3DTransform':'J3DGraphBase'};matches={}
for name,folder in units.items():
 obj=BUILD/(name+'-verified.o');cmd=base+['-c',str(ROOT/'src/JSystem'/folder/(name+'.cpp')),'-o',str(obj)]
 (BUILD/(name+'-verified.command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 p=subprocess.run(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/(name+'-verified.log')).write_text(p.stdout);p.check_returncode()
 target=ROOT/'build/original-j3d-joint-traversal-20260903/retail/obj/JSystem'/folder/(name+'.o')
 dest=BUILD/(name+'-objdiff.json')
 subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(target),'-2',str(obj),'-o',str(dest),'--format','json-pretty'],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
 diff=json.loads(dest.read_text())
 for symbol in diff['left']['symbols']:
  n=symbol['name']
  if (name=='J3DMtxBuffer' and any(n.startswith(v+'__12J3DMtxBuffer') for v in methods)) or n.startswith(tuple(v+'__F' for v in helpers)+('J3DCalcViewBaseMtx__','j3dDefaultScale','j3dDefaultMtx')):
   right=next(x for x in diff['right']['symbols'] if x['name']==n)
   matches[n]={'match_percent':symbol.get('match_percent'),'retail_size':int(symbol['size']),'compiled_size':int(right['size'])}
dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
def read(address,size):
 for i in range(18):
  offset,base,length=[struct.unpack_from('>I',dol,field+i*4)[0] for field in (0,0x48,0x90)]
  if base<=address and address+size<=base+length:return dol[offset+address-base:offset+address-base+size]
 raise AssertionError(hex(address))
assert read(0x8055C1D8,12)==struct.pack('>3f',1,1,1)
assert read(0x8055C1E4,48)==struct.pack('>12f',1,0,0,0,0,1,0,0,0,0,1,0)
assert matches['J3DCalcBBoardMtx__FPA4_f']['match_percent']==100.0
out={'dol_sha1':hashlib.sha1(dol).hexdigest(),'root_native_unchanged_bodies':methods+['J3DCalcViewBaseMtx']+helpers+['J3DPSMtx33CopyFrom34'],'native_matrix_buffer_header_exact':True,'default_constants_equal_retail':True,'original_compiler':matches}
(HERE/'source-evidence.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
