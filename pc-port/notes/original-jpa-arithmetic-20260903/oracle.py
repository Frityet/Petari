#!/usr/bin/env python3
"""Execute the affected actual retail PPC instruction slices; no source-level JPA oracle."""
from pathlib import Path
import hashlib,importlib.util,json,math,random,struct
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-arithmetic-20260903'
s=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
D=(R/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(D).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
def s32(x):return (x&0x7fffffff)-(x&0x80000000)
def s16(x):return (x&0x7fff)-(x&0x8000)
def f32(x):
 try:return struct.unpack('>f',struct.pack('>f',x))[0]
 except OverflowError:return math.copysign(math.inf,x)
def fctiwz(x):
 # Dolphin Interpreter_FloatingPoint.cpp ConvertToInteger TowardsZero, result word only.
 if math.isnan(x):return 0x80000000
 if x>=2147483648:return 0x7fffffff
 if x< -2147483648:return 0x80000000
 return math.trunc(x)&0xffffffff
def divide(a,b):
 # Dolphin Interpreter_Integer.cpp divwx, including hardware-defined overflow result.
 a,b=s32(a),s32(b)
 if not b or (a==-2147483648 and b==-1):return 0xffffffff if a<0 else 0
 quotient=abs(a)//abs(b)
 return (-quotient if (a<0)!=(b<0) else quotient)&0xffffffff
class PPC:
 def __init__(self):self.r={1:0x20000,2:0x806bfc20,26:0x10000,31:0x10000};self.f={};self.raw={};self.mem={}
 def read(self,a,n):
  if a>=0x80000000:return h.dol_bytes(D,a,n)
  return bytes(self.mem[a+i] for i in range(n))
 def write(self,a,data):
  for i,b in enumerate(data):self.mem[a+i]=b
 def word(self,a,v):self.write(a,struct.pack('>I',v&0xffffffff))
 def execute(self,address,end):
  for pc in range(address,end,4):
   w=struct.unpack('>I',h.dol_bytes(D,pc,4))[0];op=w>>26;rt=(w>>21)&31;ra=(w>>16)&31;rb=(w>>11)&31;rc=(w>>6)&31;imm=s16(w)
   if op in (14,15):self.r[rt]=((self.r[ra] if ra else 0)+(imm<<(16 if op==15 else 0)))&0xffffffff
   elif op==21:
    mb=(w>>6)&31;me=(w>>1)&31;v=self.r[rt];rot=((v<<rb)|(v>>(32-rb if rb else 32)))&0xffffffff
    mask=sum(1<<(31-i) for i in range(32) if (mb<=i<=me if mb<=me else i>=mb or i<=me));self.r[ra]=rot&mask
   elif op==31:
    xo=(w>>1)&1023
    if xo==491:self.r[rt]=divide(self.r[ra],self.r[rb])
    elif xo==922:self.r[ra]=s16(self.r[rt])&0xffffffff
    elif xo==0:pass # comparison only controls a later branch outside the conversion slice
    else:raise AssertionError((hex(pc),xo))
   elif op==32:self.r[rt]=struct.unpack('>I',self.read((self.r[ra]+imm)&0xffffffff,4))[0]
   elif op==36:self.word((self.r[ra]+imm)&0xffffffff,self.r[rt])
   elif op==44:self.write((self.r[ra]+imm)&0xffffffff,struct.pack('>H',self.r[rt]&0xffff))
   elif op in (48,50):self.f[rt]=struct.unpack('>f' if op==48 else '>d',self.read((self.r[ra]+imm)&0xffffffff,4 if op==48 else 8))[0];self.raw.pop(rt,None)
   elif op==54:self.write((self.r[ra]+imm)&0xffffffff,struct.pack('>Q',self.raw[rt]) if rt in self.raw else struct.pack('>d',self.f[rt]))
   elif op==59:
    xo=(w>>1)&31
    if xo==20:v=self.f[ra]-self.f[rb]
    elif xo==21:v=self.f[ra]+self.f[rb]
    elif xo==25:v=self.f[ra]*self.f[rc]
    else:raise AssertionError((hex(pc),xo))
    self.f[rt]=f32(v);self.raw.pop(rt,None)
   elif op==63 and (w>>1)&1023==15:self.raw[rt]=0xfff8000000000000|fctiwz(self.f[rb])
   else:raise AssertionError((hex(pc),hex(w)))
rows=[];rng=random.Random(0x4a5041)
floatbits={0,0x80000000,0x7f800000,0xff800000,0x7fc00000,0xffc00000,1,0x80000001,0x4effffff,0x4f000000,0xcf000000,0xcf000001}
for i in range(-65537,65538):
 for v in (float(i),i+0.5):floatbits.add(struct.unpack('>I',struct.pack('>f',v))[0])
for _ in range(4096):floatbits.add(rng.getrandbits(32))
for bits in sorted(floatbits):
 p=PPC();p.f[0]=struct.unpack('>f',struct.pack('>I',bits))[0];p.execute(0x8044add8,0x8044ade8)
 result=p.r[0];half=struct.unpack('>H',p.read(0x10088,2))[0];rows.append(f'c {bits:08x} {result:08x} {half:04x} {s16(half)}')
ints=[0,1,2,0x7fffffff,0x80000000,0x80000001,0xffffffff,0xfffffffe,32768,65536]
for a in ints:
 for b in ints:
  p=PPC();p.r[7]=a;p.r[5]=b;p.execute(0x804484a0,0x804484a4);rows.append(f'd {a:08x} {b:08x} {p.r[5]:08x}')
sphere=[]
# Includes both poles (AngleMax == 1), normal rings, invalid zero divisions, and signed word overflow edges.
for angleNum,angleMax,x,div,sweep in [(0,1,0,9,1),(0,1,8,9,1),(0,0,0,1,1)]+[(rng.randrange(0,260),rng.randrange(1,260),rng.randrange(0,260),rng.randrange(1,260),rng.choice([0.,.125,.5,1.,1.5,2.,-1.])) for _ in range(4096)]:
 p=PPC()
 for off,v in ((0x1ec,angleNum),(0x1f0,angleMax),(0x1f4,x),(0x1f8,div)):p.word(0x10000+off,v)
 p.write(0x1003c,struct.pack('>f',sweep));p.execute(0x80448480,0x804484fc)
 phi,theta=s16(p.r[30]),s16(p.r[29]);inc=struct.unpack('>I',p.read(0x101ec,4))[0]
 rows.append(f's {angleNum} {angleMax} {x} {div} {sweep} {phi} {theta} {inc}')
 if len(sphere)<3:sphere.append({'angleNum':angleNum,'angleMax':angleMax,'x':x,'div':div,'sweep':sweep,'phi':phi,'theta':theta})
(B/'oracle-cases.txt').write_text('\n'.join(rows)+'\n')
report={'dol_sha1':hashlib.sha1(D).hexdigest(),'conversion_slice':['0x8044add8','0x8044ade8'],'sphere_slice':['0x80448480','0x804484fc'],'division_instruction':'0x804484a0','conversion_cases':len(floatbits),'division_cases':100,'sphere_cases':4099,'pole_examples':sphere,'arithmetic_reference':['pc-port/dolphin/Source/Core/Core/PowerPC/Interpreter/Interpreter_Integer.cpp:500','pc-port/dolphin/Source/Core/Core/PowerPC/Interpreter/Interpreter_FloatingPoint.cpp:54'],'case_sha256':hashlib.sha256((B/'oracle-cases.txt').read_bytes()).hexdigest()}
(N/'oracle-evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
