#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,difflib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-draw-20260903'
spec=importlib.util.spec_from_file_location('helper',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
src=R/'src/Game/System/Overwrite.cpp';cmd=h.compiler('cflags_game')+['-c',str(src),'-o',str(B/'Overwrite.o')]
r=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/'Overwrite.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout,r.stderr);r.check_returncode()
subprocess.run([str(R/'build/tools/objdiff-cli'),'diff','-1',str(R/'build/mario-update-restoration-20260903/retail/obj/Game/System/Overwrite.o'),'-2',str(B/'Overwrite.o'),'-o',str(B/'Overwrite.json'),'--format','json-pretty'],check=True,capture_output=True)
d=json.loads((B/'Overwrite.json').read_text());syms=[]
for s in d['left']['symbols']:
 if (s['name'].startswith('JPADraw') or '@unnamed@Overwrite_cpp@' in s['name'] or s['name'].startswith('calcYBBCam')) and s.get('match_percent') is not None:
  syms.append({k:s[k] for k in ['name','size','match_percent']});print(s['name'],s['match_percent'])
(N/'root-evidence.json').write_text(json.dumps({'command':cmd,'methods':syms,'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest()},indent=2)+'\n')
old=(B/'Overwrite-before.cpp').read_text();new=src.read_text();(N/'root.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/src/Game/System/Overwrite.cpp','b/src/Game/System/Overwrite.cpp')))
# Establish that the retail split being compared is the local verified DOL, masking only relocations.
import re,struct
raw=(R/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(raw).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
retail=h.Elf(R/'build/mario-update-restoration-20260903/retail/obj/Game/System/Overwrite.o')
config=(R/'config/RMGK01/symbols.txt').read_text();proof=[]
for row in syms:
 name=row['name'];sym=next(x for x in retail.symbols if x[0]==name)
 m=re.search(r'^'+re.escape(name)+r' = \.text:0x([0-9A-Fa-f]+);.*size:0x([0-9A-Fa-f]+)',config,re.M)
 if not m:continue
 addr,size=[int(v,16) for v in m.groups()];_,start,objSize,index=sym;assert size==objSize
 obj=bytearray(retail.section_data(index)[start:start+size]);dol=bytearray(h.dol_bytes(raw,addr,size));kinds=[]
 for sec in retail.sections:
  if sec[1]!=4 or sec[7]!=index:continue
  for off in range(sec[4],sec[4]+sec[5],sec[9]):
   at,info,addend=struct.unpack_from('>IIi',retail.data,off)
   if not start<=at<start+size:continue
   at-=start;kind=info&255;kinds.append(kind)
   for code in (obj,dol):
    if kind==10:
     word=struct.unpack_from('>I',code,at)[0];struct.pack_into('>I',code,at,word&0xfc000003)
    elif kind==109:
     word=struct.unpack_from('>I',code,at)[0];struct.pack_into('>I',code,at,word&0xffe00000)
    elif kind in (4,5,6): code[at:at+2]=b'\0\0'
    elif kind==1:code[at:at+4]=b'\0\0\0\0'
    else:raise AssertionError((name,kind))
 assert obj==dol,(name,'retail split does not match DOL')
 proof.append({'name':name,'address':hex(addr),'size':size,'relocation_kinds':sorted(set(kinds)),'dol_normalized_exact':True})
# Native byte arrays and dispatch order are exact original data.
data_names=['jpa_dl','jpa_dl_x'];data_addresses=[0x805dbd20,0x805dbd40]
arrays=[]
for name,addr in zip(data_names,data_addresses):
 match=re.search(r'static u8 '+name+r'\[32\] = \{([^}]+)\}',src.read_text(),re.S)
 values=bytes(int(n,16) for n in re.findall(r'0x([0-9A-Fa-f]+)',match[1]));assert values==h.dol_bytes(raw,addr,32)
 arrays.append({'name':name,'address':hex(addr),'size':32,'dol_exact':True})
(N/'dol-evidence.json').write_text(json.dumps({'dol_sha1':hashlib.sha1(raw).hexdigest(),'methods':proof,'arrays':arrays},indent=2)+'\n')
# Root checkpoint has three files. Preserve a literal before/after provider inventory.
paths=['src/Game/System/Overwrite.cpp','src/JSystem/JParticle/JPABaseShape.cpp','libs/RVL_SDK/include/revolution/mtx.h']
patch=[];inventory=[]
for path in paths:
 old=subprocess.run(['git','show','HEAD:'+path],cwd=R,capture_output=True,text=True,check=True).stdout;new=(R/path).read_text()
 patch.append(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/'+path,'b/'+path)))
 for phase,text in [('before',old),('after',new)]:
  for signature in re.findall(r'^(?:void (?:JPADraw\w+|JPAEmitterManager::calcYBBCam)\([^\n]+\) \{)',text,re.M):
   inventory.append({'phase':phase,'source':path,'definition':signature})
(N/'root.patch').write_text(''.join(patch));(N/'provider-inventory.json').write_text(json.dumps(inventory,indent=2)+'\n')
(N/'Overwrite-draw.cpp').write_text(src.read_text())
# Compile actual SDK source after moving the misplaced line provider.
sdk=R/'src/JSystem/JParticle/JPABaseShape.cpp';cmd=h.compiler('cflags_jsys')+['-c',str(sdk),'-o',str(B/'JPABaseShape-ppc.o')]
p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/'JPABaseShape-ppc.log').write_text(p.stdout+p.stderr);p.check_returncode()
e=h.Elf(B/'JPABaseShape-ppc.o');assert not any(x[0].startswith('JPADrawLine__') and x[2] for x in e.symbols)
(N/'sdk-provider-evidence.json').write_text(json.dumps({'command':cmd,'returncode':0,'sdk_defines_JPADrawLine':False},indent=2)+'\n')
