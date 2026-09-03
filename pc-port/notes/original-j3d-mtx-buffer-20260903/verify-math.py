#!/usr/bin/env python3
"""Execute raw retail paired matrix instructions against actual native exports."""
import ctypes, hashlib, importlib.util, json, math, random, re, struct, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]; HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-j3d-mtx-buffer-20260903'; BUILD.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('base_oracle',ROOT/'pc-port/notes/xanime-core-matrix-calculation-20260903/math-oracle.py')
base=importlib.util.module_from_spec(spec); spec.loader.exec_module(base)
f32, bits, value, op=base.f32,base.bits,base.value,base.op
DOL=base.DOL.read_bytes(); assert hashlib.sha1(DOL).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
P,Q,OUT,STACK=0x10000,0x20000,0x30000,0x40000
SPECS={'proj':(0x804241F4,0x124),'inverse':(0x80423C0C,0xC8),'concat':(0x80424318,0xDC),'copy':(0x80426304,0x34),'scale34':(0x8042413C,0x64),'scale33':(0x804241A0,0x54),'sqrt':(0x804243F4,0x18)}
text=(ROOT/'pc-port/dolphin/Source/Core/Common/FloatUtils.cpp').read_text().split('frsqrte_expected',1)[1].split('}};',1)[0]
RSQ=[(int(a,16),-int(b,16)) for a,b in re.findall(r'\{(0x[0-9a-f]+), -(0x[0-9a-f]+)\}',text)]
assert len(RSQ)==32

def rsqrt(x):
 raw=struct.unpack('>Q',struct.pack('>d',x))[0]; mant=raw&((1<<52)-1); exp=raw&(2047<<52); sign=raw>>63
 if not exp and not mant: return math.copysign(math.inf,x)
 if exp==(2047<<52): return x if mant else (math.nan if sign else 0.0)
 if sign:return math.nan
 if not exp:
  while not mant&(1<<52): exp-=1<<52;mant<<=1
  mant&=(1<<52)-1;exp+=1<<52
 lsb=exp&(1<<52);new=((1023<<52)-((exp-(1022<<52))//2))&(2047<<52)
 index=(lsb|mant)>>37;v,d=RSQ[index//2048]
 return struct.unpack('>d',struct.pack('>Q',new|((v+d*(index%2048))<<26)))[0]

def execute(kind,a,b=(),count=1,alias=None):
 start,size=SPECS[kind];g=[0]*32;f=[[0.,0.] for _ in range(32)];cr=[False]*32;mem={};ctr=0
 g[1]=STACK;g[2]=0x806BFC20;g[3:7]=[P,Q,OUT,count]
 output=OUT
 if kind in ('inverse','copy'):g[4]=output
 if kind.startswith('scale'):output=P
 if alias=='a':output=P
 elif alias=='b':output=Q
 elif alias=='shift':output=P+4
 if kind in ('inverse','copy'):g[4]=output
 elif kind in ('concat','proj'):g[5]=output
 if kind=='sqrt':f[1]=[a[0],a[0]]
 for at,values in ((P,a),(Q,b),(OUT,[123.0]*36)):
  for i,x in enumerate(values):mem[at+i*4]=x
 def load(at,size=4):
  if at in mem:return mem[at]
  if STACK-256<=at<=STACK:return 0.
  return struct.unpack('>f' if size==4 else '>d',base.dol_read(DOL,at,size))[0]
 def branch(bo,bi):
  nonlocal ctr
  if not bo&4:ctr=(ctr-1)&0xFFFFFFFF
  return (bool(bo&16) or cr[bi]==bool(bo&8)) and (bool(bo&4) or (ctr!=0)!=bool(bo&2))
 pc=start
 for _ in range(2000):
  word=int.from_bytes(base.dol_read(DOL,pc,4),'big');code,d,rA,rB,rC=word>>26,(word>>21)&31,(word>>16)&31,(word>>11)&31,(word>>6)&31
  imm=base.signed(word&65535,16);nxt=pc+4
  if code==19:
   assert (word>>1)&1023==16
   if branch(d,rA):break
  elif code in (14,15):g[d]=((g[rA] if rA else 0)+(imm if code==14 else imm<<16))&0xFFFFFFFF
  elif code in (36,37):
   at=(g[rA]+imm)&0xFFFFFFFF;mem[at]=g[d]
   if code==37:g[rA]=at
  elif code==31:
   assert (word>>1)&1023==467;ctr=g[d]
  elif code in (48,50):
   x=load((g[rA]+imm)&0xFFFFFFFF,4 if code==48 else 8);f[d]=[x,x if code==48 else f[d][1]]
  elif code in (52,54):mem[(g[rA]+imm)&0xFFFFFFFF]=f[d][0]
  elif code in (56,57,60,61):
   at=(g[rA]+base.signed(word&4095,12))&0xFFFFFFFF;w=(word>>15)&1;assert not((word>>12)&7)
   if code<60:f[d]=[load(at),1.0 if w else load(at+4)]
   else:
    mem[at]=f[d][0]
    if not w:mem[at+4]=f[d][1]
   if code&1:g[rA]=at
  elif code==59:
   xo=(word>>1)&31
   if xo==24:x=base.reciprocal(f[rB][0])
   elif xo==25:x=f32(f[rA][0]*f[rC][0])
   else:raise AssertionError((hex(pc),'op59',xo))
   f[d]=[x,x]
  elif code==63:
   xo=(word>>1)&1023
   if xo in (0,32):
    a0,b0=f[rA][0],f[rB][0];cr[(d//4)*4:(d//4)*4+4]=[a0<b0,a0>b0,a0==b0,math.isnan(a0) or math.isnan(b0)]
   elif xo==26:f[d]=[rsqrt(f[rB][0]),f[d][1]]
   else:raise AssertionError((hex(pc),'op63',xo))
  elif code==4:
   xo=(word>>1)&1023;short=xo&31;A,B,C=f[rA][:],f[rB][:],f[rC][:]
   if xo in (0,32):cr[0:4]=[A[0]<B[0],A[0]>B[0],A[0]==B[0],math.isnan(A[0]) or math.isnan(B[0])]
   elif xo in (528,560,592,624):f[d]=[A[(xo-528)//64],B[((xo-528)//32)%2]]
   elif short in (20,21):f[d]=[op('sub' if short==20 else 'add',A[i],B[i]) for i in range(2)]
   elif short in (12,13):f[d]=[op('mul',A[i],C[short-12]) for i in range(2)]
   elif short==25:f[d]=[op('mul',A[i],C[i]) for i in range(2)]
   elif short in (14,15):f[d]=[op('fma',A[i],C[short-14],B[i]) for i in range(2)]
   elif short in (28,29,30):
    f[d]=[op('fma',A[i],C[i],B[i] if short==29 else -B[i]) for i in range(2)]
    if short==30:f[d]=[-x if not math.isnan(x) else x for x in f[d]]
   else:raise AssertionError((hex(pc),'ps',xo))
  elif code==16:
   if branch(d,rA):nxt=pc+base.signed(word&0xFFFC,16)
  else:raise AssertionError((hex(pc),hex(word),code))
  pc=nxt
 else:raise AssertionError('Instruction budget')
 n=12*count if kind=='concat' else 12 if kind in ('scale34','proj') else 9
 return [f[1][0]] if kind=='sqrt' else [mem[output+4*i] for i in range(n)]

def compile_native():
 flags=['-O2','-fPIC','-ffp-contract=off','-Ipc-port/src','-Ipc-port/aurora/include','-DTARGET_PC']
 commands=[]
 for name in ('mtx','vec'):
  commands.append(['clang','-std=c11',*flags,'-c',f'pc-port/aurora/lib/dolphin/mtx/{name}.c','-o',str(BUILD/(name+'-oracle.o'))])
 exports=['proj','inverse','concat','copy','scale34','scale33','sqrt','envelope']
 commands.append(['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23',*flags,'-Wno-register','-include','pc-port/src/compat/MetrowerksStdCompat.hpp','-dynamiclib','-Wl,-dead_strip',*[f'-Wl,-exported_symbol,_native_{name}' for name in exports],'pc-port/src/compat/J3DTransformMtxCompat.cpp','pc-port/src/compat/J3DJointCompat.cpp','pc-port/src/compat/J3DJointTreeCompat.cpp','pc-port/src/compat/J3DMtxBufferCompat.cpp','pc-port/src/compat/J3DTextureMtxCompat.cpp',str(HERE/'math-harness.cpp'),str(BUILD/'mtx-oracle.o'),str(BUILD/'vec-oracle.o'),'-o',str(BUILD/'libmatrix-oracle.dylib')])
 for i,cmd in enumerate(commands):
  p=subprocess.run(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/f'math-build-{i}.log').write_text(p.stdout);p.check_returncode()
 (BUILD/'math-commands.json').write_text(json.dumps(commands,indent=2)+'\n')
compile_native();native=ctypes.CDLL(str(BUILD/'libmatrix-oracle.dylib'));ptr=ctypes.POINTER(ctypes.c_float)
for name in ('inverse','copy','scale34','scale33'):getattr(native,'native_'+name).argtypes=[ptr,ptr]
native.native_proj.argtypes=[ptr,ptr,ptr]
native.native_concat.argtypes=[ptr,ptr,ptr,ctypes.c_uint];native.native_sqrt.argtypes=[ctypes.c_float];native.native_sqrt.restype=ctypes.c_float
native.native_envelope.argtypes=[ctypes.c_ushort,ctypes.POINTER(ctypes.c_ubyte),ctypes.POINTER(ctypes.c_ushort),ptr,ptr,ptr,ctypes.POINTER(ctypes.c_ubyte),ptr,ctypes.POINTER(ctypes.c_ubyte)]
rng=random.Random(0x80423C0C);counts={name:0 for name in SPECS};exceptions=0

def check(kind,a,b=(),count=1,alias=None):
 global exceptions
 expected=execute(kind,a,b,count,alias);abuf=(ctypes.c_float*48)(*a);bbuf=(ctypes.c_float*48)(*b);out=(ctypes.c_float*48)(*[123.]*48)
 if alias=='a':out=abuf
 elif alias=='b':out=bbuf
 elif alias=='shift':out=ctypes.cast(ctypes.byref(abuf,4),ptr)
 if kind=='sqrt':actual=[native.native_sqrt(a[0])]
 elif kind.startswith('scale'):
  getattr(native,'native_'+kind)(abuf,bbuf);actual=list(abuf)[:len(expected)]
 elif kind=='proj':native.native_proj(abuf,bbuf,out);actual=list(out[:len(expected)])
 elif kind=='concat':native.native_concat(abuf,bbuf,out,count);actual=list(out[:len(expected)])
 else:getattr(native,'native_'+kind)(abuf,out);actual=list(out[:len(expected)])
 def key(x):return 'nan' if math.isnan(x) else bits(x)
 assert list(map(key,actual))==list(map(key,expected)),(kind,a,b,alias,list(map(key,actual)),list(map(key,expected)))
 counts[kind]+=1;exceptions+=any(not math.isfinite(x) for x in expected)

identity=[1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.]
for a in [identity,[0.]*12,[1.,2.,3.,4.,2.,4.,6.,8.,3.,6.,9.,12.]]:
 for alias in (None,'a','shift'):check('inverse',a,alias=alias)
for _ in range(1500):
 a=[f32(rng.uniform(-8,8)) for _ in range(12)];b=[f32(rng.uniform(-8,8)) for _ in range(24)]
 for alias in (None,'a','shift'):check('inverse',a,alias=alias);check('copy',a,alias=alias)
 for alias in (None,'b'):check('concat',a,b,count=2,alias=alias)
 for alias in (None,'a','b'):check('proj',a,b[:16],alias=alias)
 check('scale34',a,b[:3]);check('scale33',a[:9],b[:3])
# Every float exponent, both signs, and representative mantissas include
# subnormal values and both infinities/NaNs. Add random complete bit patterns.
for exp in range(256):
 for sign in (0,0x80000000):
  for mant in (0,1,0x3FFFFF,0x7FFFFF):check('sqrt',[value(sign|(exp<<23)|mant)])
for _ in range(5000):check('sqrt',[value(rng.getrandbits(32))])
# The envelope's original C++/inline-assembly register assignment is not an
# instruction match. Execute its actual retail paired arithmetic, including
# every store and both accumulator resets, against the original native method.
def envelope_expected(mix_counts,indices,weights,world,inverse,scales):
 f=[[0.,0.] for _ in range(32)];g=[0]*32;mem={};f[24]=list(struct.unpack('>2f',base.dol_read(DOL,0x805E9A60,8)))
 def block(addresses):
  for pc in addresses:
   word=int.from_bytes(base.dol_read(DOL,pc,4),'big');code,d,rA,rB,rC=word>>26,(word>>21)&31,(word>>16)&31,(word>>11)&31,(word>>6)&31
   if code in (56,60):
    at=g[rA]+base.signed(word&4095,12);assert not((word>>12)&15)
    if code==56:f[d]=[mem[at],mem[at+4]]
    else:mem[at],mem[at+4]=f[d]
   elif code==4:
    xo=(word>>1)&1023;short=xo&31;A,B,C=f[rA][:],f[rB][:],f[rC][:]
    if xo==528:f[d]=[A[0],B[0]]
    elif short in (12,13):f[d]=[op('mul',A[i],C[short-12]) for i in range(2)]
    elif short in (14,15):f[d]=[op('fma',A[i],C[short-14],B[i]) for i in range(2)]
    elif short==29:f[d]=[op('fma',A[i],C[i],B[i]) for i in range(2)]
    else:raise AssertionError((hex(pc),'envelope ps',xo))
   else:raise AssertionError((hex(pc),'envelope unexpected instruction',code))
 block([0x80432270,0x80432274,0x8043227C]);cursor=0;result=[];result_scales=[]
 for count in mix_counts:
  block([0x80432290,0x80432294,0x804322A0]);g[9]=OUT
  flag=1
  for _ in range(count):
   index=indices[cursor];f[0]=[weights[cursor]]*2;g[5]=P;g[6]=Q
   for at,values in ((P,world[index*12:index*12+12]),(Q,inverse[index*12:index*12+12])):
    for i,x in enumerate(values):mem[at+i*4]=x
   block(list(range(0x804322E4,0x80432380,4))+[0x80432384,0x8043238C,0x80432394])
   flag&=scales[index];cursor+=1
  block(range(0x804323A4,0x804323BC,4));result.extend(mem[OUT+i*4] for i in range(12));result_scales.append(flag)
 return result,result_scales
counts['envelope']=0
for _ in range(1000):
 mix_counts=[rng.randrange(1,6) for _ in range(rng.randrange(1,5))];indices=[rng.randrange(4) for _ in range(sum(mix_counts))]
 weights=[f32(rng.uniform(-1,1)) for _ in indices];world=[f32(rng.uniform(-8,8)) for _ in range(48)];inverse=[f32(rng.uniform(-8,8)) for _ in range(48)];scales=[rng.randrange(4) for _ in range(4)]
 expected,expected_scales=envelope_expected(mix_counts,indices,weights,world,inverse,scales)
 out=(ctypes.c_float*len(expected))();out_scales=(ctypes.c_ubyte*len(mix_counts))()
 native.native_envelope(len(mix_counts),(ctypes.c_ubyte*len(mix_counts))(*mix_counts),(ctypes.c_ushort*len(indices))(*indices),(ctypes.c_float*len(weights))(*weights),(ctypes.c_float*48)(*world),(ctypes.c_float*48)(*inverse),(ctypes.c_ubyte*4)(*scales),out,out_scales)
 assert list(map(bits,out))==list(map(bits,expected)),('envelope',mix_counts,indices,list(map(bits,out)),list(map(bits,expected)))
 assert list(out_scales)==expected_scales
 counts['envelope']+=1
evidence={'dol_sha1':hashlib.sha1(DOL).hexdigest(),'raw_instruction_specs':SPECS,'native_calls':counts,'exceptional_classification_cases':exceptions,'comparison':'Every finite output bit including signed zero; exceptional results compare NaN classification and infinity signs. FPSCR and NaN payload/sign are not asserted. Matrix random cases use finite moderate inputs; sqrt spans all float exponent classes.','all_passed':True}
(HERE/'math-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n');print(json.dumps(evidence,indent=2))
