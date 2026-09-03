#!/usr/bin/env python3
from pathlib import Path
import hashlib,importlib.util,json,re,subprocess,struct
ROOT=Path(__file__).resolve().parents[3];NOTE=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-camera-compile-closure-20260903'
BASELINE='2523c214c0ad6f72ae3dcb16148ed7a49f2919d0'
BUILD.mkdir(parents=True,exist_ok=True)
def baseline(path):
 return subprocess.check_output(['git','show',BASELINE+':'+path],cwd=ROOT)
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
c=module('c','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');r=module('r','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
include=BUILD/'baseline-include/Game/Camera';include.mkdir(parents=True,exist_ok=True);(include/'CameraParamChunk.hpp').write_bytes(baseline('include/Game/Camera/CameraParamChunk.hpp'))
functions={
 'DotCamParams':['hasMoreChunk__17DotCamReaderInBinCFv'],
 'CameraManGame':['createStartAnimCamera__13CameraManGameFv'],
 'CameraParamChunk':['__as__18CameraGeneralParamFRC18CameraGeneralParam','load__16CameraParamChunkFP12DotCamReaderP12CameraHolder'],
 'CamTranslatorAnim':['setParam__17CamTranslatorAnimFPC16CameraParamChunk','getAnimFrame__17CamTranslatorAnimCFPC16CameraParamChunk'],
 'CamTranslatorSpiral':['setParam__19CamTranslatorSpiralFPC16CameraParamChunk'],
}
def image(elf,name):
 _,start,size,index=next(s for s in elf.symbols if s[0]==name)
 code=elf.section_data(index)[start:start+size];refs=[]
 for section in elf.sections:
  if section[1]!=4 or section[7]!=index:continue
  for off in range(section[4],section[4]+section[5],section[9]):
   at,info,addend=struct.unpack_from('>IIi',elf.data,off)
   if start<=at<start+size:refs.append((at-start,info&255,elf.symbols[info>>8][0],addend))
 return code,refs
records=[]
for name,names in functions.items():
 paths=[]
 for state in ['before','after']:
  source=ROOT/f'src/Game/Camera/{name}.cpp'
  if state=='before':
   before=BUILD/'before'/f'{name}.cpp';before.parent.mkdir(exist_ok=True);before.write_bytes(baseline(f'src/Game/Camera/{name}.cpp'));source=before
  obj=BUILD/(name+'.'+state+'.o');cmd=c.compiler('cflags_game')
  if state=='before':cmd[3:3]=['-i',str(BUILD/'baseline-include')]
  cmd+=['-c',str(source),'-o',str(obj)]
  result=subprocess.run(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/(name+'.'+state+'.log')).write_text(result.stdout);result.check_returncode();paths.append(obj)
 for symbol in names:
  sides=[image(r.Elf(p),symbol)for p in paths];assert sides[0]==sides[1],symbol
  records.append(dict(symbol=symbol,unchanged_bytes=len(sides[0][0]),sha256=hashlib.sha256(sides[0][0]).hexdigest(),unchanged_relocations=len(sides[0][1])));print(symbol,len(sides[0][0]),'identical')
probe=BUILD/'layout.cpp';probe.write_text('''#include "Game/Camera/CameraParamChunk.hpp"
#include <stddef.h>
typedef char PointerWord[(sizeof(intptr_t)==4)?1:-1];
typedef char ParamSize[(sizeof(CameraGeneralParam)==0x3C)?1:-1];
typedef char Num1Offset[(offsetof(CameraGeneralParam,mNum1)==0x30)?1:-1];
typedef char Num2Offset[(offsetof(CameraGeneralParam,mNum2)==0x34)?1:-1];
typedef char StringOffset[(offsetof(CameraGeneralParam,mString)==0x38)?1:-1];
''')
result=subprocess.run(c.compiler('cflags_game')+['-c',str(probe),'-o',str(BUILD/'layout.o')],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/'layout.log').write_text(result.stdout);result.check_returncode()
(NOTE/'compiler-evidence.json').write_text(json.dumps(dict(baseline_commit=BASELINE,original_layout_unchanged=True,functions=records),indent=2)+'\n')
