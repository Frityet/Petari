"""Read-only extraction of RARC tables; preserves unknown hashed field names."""
import hashlib,json,pathlib,re,struct,sys
source=pathlib.Path(sys.argv[1]);data=source.read_bytes();sha=hashlib.sha256(data).hexdigest()
if data[:4]==b'Yaz0':
 expected=struct.unpack_from('>I',data,4)[0];out=bytearray();pos=16
 while len(out)<expected:
  flags=data[pos];pos+=1
  for bit in range(7,-1,-1):
   if len(out)==expected:break
   if flags&(1<<bit):out.append(data[pos]);pos+=1
   else:
    first,second=data[pos:pos+2];pos+=2;count=first>>4
    if count==0:count=data[pos]+0x12;pos+=1
    else:count+=2
    distance=((first&15)<<8|second)+1
    for _ in range(count):out.append(out[-distance])
 data=bytes(out)
assert data[:4]==b'RARC'
u32=lambda off:struct.unpack_from('>I',data,off)[0]
u16=lambda off:struct.unpack_from('>H',data,off)[0]
nodes,entries,strings,payload=[0x20+u32(o) for o in (0x24,0x2c,0x34,0x0c)]
def h(name):
 v=0
 for b in name.encode():v=(v*31+b)&0xffffffff
 return v
keys=set(['name','l_id','ParentID','GroupId','CastId','DemoGroupId','SW_A','SW_B','SW_APPEAR','SW_DEAD','SW_SLEEP','DemoName','TimeSheetName','MessageId','Obj_ID','Camera_id','CommonPath_ID']+[f'Obj_arg{i}' for i in range(8)]+[f'{p}_{c}' for p in ['pos','dir','scale'] for c in 'xyz'])
for path in [*pathlib.Path('src/Game/Demo').glob('*.cpp'),pathlib.Path('src/compat/DemoSheetRuntime.cpp')]:
 keys.update(re.findall(r'"([a-zA-Z_][a-zA-Z0-9_]*)"',path.read_text()))
names={h(k):k for k in keys};result={}
def table(path,blob):
 if len(blob)<16:return
 count,n,row_start,row_size=struct.unpack_from('>4I',blob)
 if count>100000 or n>500 or row_start<16+n*12 or row_start+count*row_size>len(blob):return
 fields=[struct.unpack_from('>IIHBB',blob,16+i*12) for i in range(n)];strings_start=row_start+count*row_size
 rows=[]
 for row in range(count):
  values={'row':row}
  for key,mask,offset,shift,kind in fields:
   at=row_start+row*row_size+offset
   if kind==6:
    at=strings_start+struct.unpack_from('>I',blob,at)[0];value=blob[at:blob.index(0,at)].decode('cp932')
   elif kind==1:value=blob[at:blob.index(0,at)].decode('cp932')
   elif kind==2:value=struct.unpack_from('>f',blob,at)[0]
   elif kind in (0,3,4,5):
    width={0:4,3:4,4:2,5:1}[kind];value=(int.from_bytes(blob[at:at+width],'big')&mask)>>shift
    if shift==0 and mask==(1<<(width*8))-1 and value>=(1<<(width*8-1)):value-=1<<(width*8)
   else:raise ValueError((path,kind))
   values[names.get(key,f'hash_{key:08x}')]=value
  rows.append(values)
 result[path]=rows
def walk(node,path):
 off=nodes+node*16
 for i in range(u16(off+10)):
  entry=entries+(u32(off+12)+i)*20;meta=u32(entry+4);at=strings+(meta&0xffffff);name=data[at:data.index(0,at)].decode('cp932')
  if name in ('.','..'):continue
  current=path+'/'+name
  if meta>>24&2:walk(u32(entry+8),current)
  elif not sys.argv[2] or sys.argv[2].lower() in current.lower():
   at=payload+u32(entry+8);table(current,data[at:at+u32(entry+12)])
walk(0,'')
print(json.dumps({'archive':str(source),'archive_sha256':sha,'tables':result},ensure_ascii=False,indent=2))
