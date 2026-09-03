#!/usr/bin/env python3
"""Prove the wall-shade method in an isolated copy of the shared source TU."""
from pathlib import Path
import hashlib,importlib.util,json,subprocess,struct,difflib
ROOT=Path(__file__).resolve().parents[3];NOTE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-shadow-native-20260903';BUILD.mkdir(parents=True,exist_ok=True)
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
c=module('c','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
r=module('r','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
assert hashlib.sha1(c.DOL.read_bytes()).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
source=ROOT/'src/Game/Player/MarioActorSpecialDraw.cpp';old=source.read_text();method=(NOTE/'method.cpp').read_text().rstrip()
placeholder='// void MarioActor::drawWallShade(const TVec3f&, const TVec3f&, f32) const {}'
if placeholder in old: new=old.replace(placeholder,method)
else:
 assert method in old,'Current root method differs from the proved method'
 new=old
staged=BUILD/'WallSpecialDraw.cpp';staged.write_text(new);obj=BUILD/'WallSpecialDraw.o'
command=c.compiler('cflags_game')+['-c',str(staged),'-o',str(obj)]
result=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
(BUILD/'WallSpecialDraw.log').write_text(result.stdout);result.check_returncode()
retail=ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Player/MarioActorSpecialDraw.o'
diff=BUILD/'WallSpecialDraw.diff.json'
subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(obj),'-o',str(diff),'--format','json-pretty'],check=True,stdout=subprocess.PIPE)
symbol='drawWallShade__10MarioActorCFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>f'
d=json.loads(diff.read_text());sides=[next(s for s in d[k]['symbols']if s['name']==symbol)for k in ('left','right')]
assert sides[0]['match_percent']>=99.0,sides[0]['match_percent']
def calls(elf):
 _,start,size,index=next(s for s in elf.symbols if s[0]==symbol);out=[]
 for section in elf.sections:
  if section[1]!=4 or section[7]!=index:continue
  for off in range(section[4],section[4]+section[5],section[9]):
   at,info,addend=struct.unpack_from('>IIi',elf.data,off)
   if start<=at<start+size and info&255==10:out.append(elf.symbols[info>>8][0])
 return out
callsets=[calls(r.Elf(p))for p in (retail,obj)];assert callsets[0]==callsets[1]
constants={hex(a):c.dol_bytes(c.DOL.read_bytes(),a,4).hex()for a in [0x806bf7ec,0x806bf7f4,0x806bf7f8,0x806bf820,0x806bf82c,0x806bf830,0x806bf83c,0x806bf840,0x806bf844,0x806bf848]}
(NOTE/'compiler-evidence.json').write_text(json.dumps(dict(symbol=symbol,address='0x802c2d0c',retail_size=0x3c8,compiled_size=sides[1]['size'],match_percent=sides[0]['match_percent'],same_direct_calls=callsets[0],retail_constants=constants,method_sha256=hashlib.sha256((NOTE/'method.cpp').read_bytes()).hexdigest(),command=command),indent=2)+'\n')
if old!=new:
 (NOTE/'root.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/src/Game/Player/MarioActorSpecialDraw.cpp',tofile='b/src/Game/Player/MarioActorSpecialDraw.cpp')))
print(sides[0]['match_percent'])
