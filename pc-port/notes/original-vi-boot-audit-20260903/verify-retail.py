#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,struct
root=Path(__file__).resolve()
while not (root/'config/RMGK01/symbols.txt').exists(): root=root.parent
raw=(root/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(raw).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
def read(address,size):
 for i in range(18):
  offset=struct.unpack_from('>I',raw,i*4)[0]
  start=struct.unpack_from('>I',raw,0x48+i*4)[0]
  length=struct.unpack_from('>I',raw,0x90+i*4)[0]
  if start<=address and address+size<=start+length: return raw[offset+address-start:offset+address-start+size]
 raise ValueError(hex(address))
def words(address,size): return struct.unpack('>'+str(size//4)+'I',read(address,size))
def calls(address,size):
 result=[]
 for i,w in enumerate(words(address,size)):
  if w>>26==18 and w&1:
   delta=w&0x3fffffc
   if delta&0x2000000: delta-=0x4000000
   result.append(address+i*4+delta)
 return result
# Actual status reads halfword 0xCC00206E, extracts low two bits before
# restoring interrupts, then returns bit 0; it never reads the scan register.
status=words(0x804b5e6c,0x3c)
assert status[5:8]==(0x3c80cc00,0xa004206e,0x541f07be)
assert status[9]==0x57e307fe
assert calls(0x804b5e6c,0x3c)==[0x804a9778,0x804a97a0]
# Original switch case destinations: NTSC, PAL, preserve current format.
table=words(0x805fd770,36)
expected_dest=[0x804b5e44,0x804b5e4c,0x804b5e50,0x804b5e44,0x804b5e4c,0x804b5e50,0x804b5e44,0x804b5e44,0x804b5e44]
assert list(table)==expected_dest
assert words(0x804b5e44,12)==(0x3be00000,0x48000008,0x3be00001)
assert words(0x804b5e24,8)==(0x281f0008,0x41810028) # >8 keeps current format
selection_calls=calls(0x803a7274,0x134)
assert selection_calls==[0x80518a08,0x804d1bd8,0x804b5e6c,0x804d1d6c,0x804b5e0c,0x804b5e0c,0x804d1ca0,0x80518a54]
assert (root/'src/Game/System/RenderMode.cpp').read_bytes()==(root/'pc-port/src/Game/System/RenderMode.cpp').read_bytes()
result={'dol_sha1':hashlib.sha1(raw).hexdigest(),'VIGetDTVStatus':{'address':'0x804b5e6c','size':60,'register':'0xCC00206E','mask':1,'sha256':hashlib.sha256(read(0x804b5e6c,60)).hexdigest()},'VIGetTvFormat':{'address':'0x804b5e0c','size':96,'table':'0x805FD770','mapping':[0,1,2,0,1,5,0,0,0],'out_of_range':'preserve current format'},'getSuitableRenderMode':{'address':'0x803a7274','size':308,'calls':[hex(x) for x in selection_calls],'native_source_identical_to_root':True}}
print(json.dumps(result,indent=2))
