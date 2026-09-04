from pathlib import Path
import importlib.util,subprocess,json,hashlib,struct
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903'
spec=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
rows=[]
for name,flags,source in [('GXMisc','cflags_sdk','src/RVL_SDK/gx/GXMisc.c'),('SceneFunction','cflags_game','src/Game/Scene/SceneFunction.cpp')]:
 cmd=h.compiler(flags)+['-c',source,'-o',str(B/(name+'-original.o'))]
 p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(N/(name+'-original-compile.log')).write_text(p.stdout+p.stderr);rows.append({'command':cmd,'returncode':p.returncode});print(name,p.returncode,p.stderr if p.returncode else '')
(N/'original-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')
dol=(R/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
proof=[]
for unit,symbol,address,size in [('GXMisc','GXSetMisc',0x804BBF14,0x8c),('SceneFunction','initEffectSystem__13SceneFunctionFUlUl',0x803450C8,0x58)]:
 e=h.Elf(B/(unit+'-original.o'));_,start,got,index=next(s for s in e.symbols if s[0]==symbol);assert got==size
 code=bytearray(e.section_data(index)[start:start+size]);retail=bytearray(h.dol_bytes(dol,address,size));refs=[]
 for section in e.sections:
  if section[1]!=4 or section[7]!=index:continue
  for at in range(section[4],section[4]+section[5],section[9]):
   offset,info,addend=struct.unpack_from('>IIi',e.data,at)
   if not start<=offset<start+size:continue
   offset-=start;kind=info&255;refs.append({'offset':offset,'kind':kind,'symbol':e.symbols[info>>8][0]})
   for data in [code,retail]:
    if kind==10:struct.pack_into('>I',data,offset,struct.unpack_from('>I',data,offset)[0]&0xfc000003)
    elif kind==109:struct.pack_into('>I',data,offset,struct.unpack_from('>I',data,offset)[0]&0xffe00000)
    elif kind in [4,5,6]:data[offset:offset+2]=b'\0\0'
    elif kind==1:data[offset:offset+4]=b'\0'*4
    else:raise AssertionError(kind)
 assert code==retail,(unit,'DOL mismatch')
 proof.append({'source_unit':unit,'symbol':symbol,'retail_address':hex(address),'size':size,'normalized_dol_equal':True,'relocations':refs})
 print(symbol,size,'normalized retail DOL exact')
(N/'original-dol-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
