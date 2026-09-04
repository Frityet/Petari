from pathlib import Path
import importlib.util,struct,re,json,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-predraw-scheduler-20260903'
s=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
dol=(R/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
symbols=(R/'config/RMGK01/symbols.txt').read_text();rows=[]
for name in ['NameObj/NameObjCategoryList','NameObj/NameObjListExecutor','Util/ObjUtil']:
 elf=h.Elf(R/'build/mario-update-restoration-20260903/retail/obj/Game'/(name+'.o'))
 for symbol,start,size,index in elf.symbols:
  if name=='Util/ObjUtil' and symbol!='registerPreDrawFunction__2MRFRCQ22MR11FunctorBasei':continue
  m=re.search(r'^'+re.escape(symbol)+r' = \.text:0x([0-9A-Fa-f]+);.*size:0x([0-9A-Fa-f]+)',symbols,re.M)
  if not m or not size:continue
  addr,n=[int(v,16) for v in m.groups()];assert n==size
  a=bytearray(elf.section_data(index)[start:start+size]);b=bytearray(h.dol_bytes(dol,addr,size));kinds=[]
  for sec in elf.sections:
   if sec[1]!=4 or sec[7]!=index:continue
   for off in range(sec[4],sec[4]+sec[5],sec[9]):
    at,info,addend=struct.unpack_from('>IIi',elf.data,off)
    if not start<=at<start+size:continue
    at-=start;kind=info&255;kinds.append(kind)
    for code in (a,b):
     if kind in (10,109):
      mask=0xfc000003 if kind==10 else 0xffe00000;word=struct.unpack_from('>I',code,at)[0];struct.pack_into('>I',code,at,word&mask)
     elif kind in (4,5,6):code[at:at+2]=b'\0\0'
     elif kind==1:code[at:at+4]=b'\0\0\0\0'
     else:raise AssertionError((symbol,kind))
  assert a==b,(symbol,'DOL mismatch');rows.append({'object':name,'symbol':symbol,'address':hex(addr),'size':size,'relocation_kinds':sorted(set(kinds)),'normalized_retail_dol_equal':True})
(N/'dol-evidence.json').write_text(json.dumps({'dol_sha1':hashlib.sha1(dol).hexdigest(),'methods':rows},indent=2)+'\n');print(len(rows),'retail methods agree with DOL')
