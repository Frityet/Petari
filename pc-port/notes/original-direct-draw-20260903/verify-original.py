#!/usr/bin/env python3
"""Compile the actual root TU and prove the bounded shadow/footprint cluster."""
from pathlib import Path
import hashlib,importlib.util,json,re,subprocess,struct
ROOT=Path(__file__).resolve().parents[3];NOTE=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-direct-draw-20260903'
def module(name,path):
 s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
c=module('c','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
r=module('r','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
wanted={'setViewMtx','loadViewMtx','setModelMtx','resetViewMtx','close','setup','sendPoint','drawFillCircle','drawTexture3D','drawFillBox3D','cameraInit3D','cameraInit2D','fix2Dpos'}
BUILD.mkdir(parents=True,exist_ok=True);dol=c.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
source=ROOT/'src/Game/Util/DirectDraw.cpp';obj=BUILD/'DirectDraw.o'
command=c.compiler('cflags_game')+['-c',str(source),'-o',str(obj)]
p=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/'compile.log').write_text(p.stdout);p.check_returncode()
retail=ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Util/DirectDraw.o';diff=BUILD/'diff.json'
subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(obj),'-o',str(diff),'--format','json-pretty'],check=True,stdout=subprocess.PIPE)
symbols={n:(int(a,16),int(s,16))for n,a,s in re.findall(r'^(\S+) = \S+:0x([0-9A-Fa-f]+);.*?size:0x([0-9A-Fa-f]+)',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
def calls(elf,name):
 _,start,size,index=next(s for s in elf.symbols if s[0]==name);out=[]
 for section in elf.sections:
  if section[1]!=4 or section[7]!=index:continue
  for off in range(section[4],section[4]+section[5],section[9]):
   at,info,addend=struct.unpack_from('>IIi',elf.data,off)
   if start<=at<start+size and info&255==10:out.append(elf.symbols[info>>8][0])
 return out
elves=[r.Elf(p)for p in(retail,obj)];d=json.loads(diff.read_text());results=[]
for left in d['left']['symbols']:
 name=left['name'];short=name.split('__6TDDraw')[0]
 if short not in wanted or '__6TDDraw'not in name:continue
 right=next(s for s in d['right']['symbols']if s['name']==name)
 threshold=65 if short=='drawFillBox3D'else 90 if short=='cameraInit2D'else 99
 assert left['match_percent']>=threshold,(name,left['match_percent'])
 pair=[calls(e,name)for e in elves]
 if short=='cameraInit2D':
  # The rebuilt static integer-vector constructor stores its three constant
  # components inline; all remaining calls retain original order.
  assert [v for v in pair[0]if not v.startswith('__ct<i>__Q29JGeometry8TVec3')]==pair[1]
 elif short!='drawFillBox3D':assert pair[0]==pair[1],name
 address,size=symbols[name]
 results.append(dict(name=name,address=hex(address),retail_size=size,compiled_size=right['size'],match_percent=left['match_percent'],retail_calls=pair[0],compiled_calls=pair[1],retail_bytes_sha256=hashlib.sha256(c.dol_bytes(dol,address,size)).hexdigest()))
assert len(results)==15,len(results)
(NOTE/'compiler-evidence.json').write_text(json.dumps(dict(dol_sha1=hashlib.sha1(dol).hexdigest(),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),command=command,functions=results),indent=2)+'\n')
subprocess.run(['python3',str(NOTE/'verify-box-stream.py')],check=True,cwd=ROOT)
print('\n'.join(f"{x['name']}: {x['match_percent']}%"for x in results))
