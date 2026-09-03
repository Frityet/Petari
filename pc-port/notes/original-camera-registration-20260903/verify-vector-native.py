#!/usr/bin/env python3
"""Independent decoded retail PS-vector oracle, plus actual native regressions."""
import ctypes,hashlib,importlib.util,json,math,os,random,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
CC='/opt/homebrew/opt/llvm/bin/clang';CXX='/opt/homebrew/opt/llvm/bin/clang++'
BUILD=ROOT/'build/original-camera-registration-20260903/math';BUILD.mkdir(parents=True,exist_ok=True)
ORACLE=ROOT/'pc-port/notes/xanime-core-matrix-calculation-20260903/math-oracle.py'
spec=importlib.util.spec_from_file_location('ppc',ORACLE);ppc=importlib.util.module_from_spec(spec);spec.loader.exec_module(ppc)
DOL=(ROOT/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(DOL).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
FUNCTIONS={'dot':(0x804B911C,0x20),'tvec_dot':(0x8001D2A8,0x20),'cross':(0x804B913C,0x3c),'scale':(0x80442858,0x24)}

def stored(value):
 if isinstance(value,tuple):return ('psq_store_f32',value)
 raw=ppc.bits(value)
 return ppc.value(raw&0x80000000) if raw&0x7f800000==0 else value

def execute(name,memory,p=0x10000,q=0x11000,out=0x12000,scale=0.0):
 address,size=FUNCTIONS[name];g=[0]*32;g[3:6]=[p,q,out];f=[[0.0,0.0] for _ in range(32)];f[1]=[scale,scale]
 memory=memory.copy()
 for pc in range(address,address+size,4):
  word=int.from_bytes(ppc.dol_read(DOL,pc,4),'big')
  if word==0x4e800020:return f[1][0] if 'dot' in name else [memory[out+4*i] for i in range(3)]
  op,d,a,b,c=word>>26,(word>>21)&31,(word>>16)&31,(word>>11)&31,(word>>6)&31
  if op==48:
   value=memory[g[a]+ppc.signed(word&65535,16)];f[d]=[value,value]
  elif op in (56,60):
   at=g[a]+ppc.signed(word&4095,12);single=(word>>15)&1;assert (word>>12)&7==0
   if op==56:f[d]=[memory[at],1.0 if single else memory[at+4]]
   else:
    memory[at]=stored(f[d][0])
    if not single:memory[at+4]=stored(f[d][1])
  elif op==4:
   xo=(word>>1)&1023;short=xo&31;A,B,C=f[a][:],f[b][:],f[c][:]
   if xo in (528,560,592,624):f[d]=[A[(xo-528)//64],B[((xo-528)//32)%2]]
   elif xo==40:f[d]=[ppc.op('neg',x) for x in B]
   elif short==10:f[d]=[ppc.op('add',A[0],B[1]),C[1]]
   elif short in (12,13):f[d]=[ppc.op('mul',A[i],C[short-12]) for i in range(2)]
   elif short==25:f[d]=[ppc.op('mul',A[i],C[i]) for i in range(2)]
   elif short in (14,15):f[d]=[ppc.op('fma',A[i],C[short-14],B[i]) for i in range(2)]
   elif short in (28,29):f[d]=[ppc.op('fma',A[i],C[i],B[i] if short==29 else ppc.op('neg',B[i])) for i in range(2)]
   else:raise AssertionError((hex(pc),hex(word),'paired'))
  else:raise AssertionError((hex(pc),hex(word),'instruction'))
 raise AssertionError('Missing return')

commands=[]
def run(command,name,env=None):
 commands.append(command);proc=subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,env=env)
 (BUILD/(name+'.log')).write_text(proc.stdout);proc.check_returncode()
 if proc.stdout:print(proc.stdout,end='')
includes=['-DTARGET_PC','-DMTX_USE_PS','-Ipc-port/aurora/include','-Ipc-port/src']
run([CC,'-std=c11','-O2','-fPIC',*includes,'-c','pc-port/aurora/lib/dolphin/mtx/vec.c','-o',str(BUILD/'vec.o')],'vec-compile')
run([CXX,'-std=c++23','-O2','-fPIC',*includes,'-dynamiclib','pc-port/src/compat/JMathVectorCompat.cpp',str(HERE/'math-harness.cpp'),str(BUILD/'vec.o'),'-o',str(BUILD/'libvectors.dylib')],'oracle-compile')
lib=ctypes.CDLL(str(BUILD/'libvectors.dylib'));ptr=ctypes.POINTER(ctypes.c_float)
lib.PSVECDotProduct.argtypes=[ptr,ptr];lib.PSVECDotProduct.restype=ctypes.c_float
lib.PSVECCrossProduct.argtypes=[ptr,ptr,ptr];lib.PSVECCrossProduct.restype=None
lib.native_scale_add.argtypes=[ptr,ptr,ptr,ctypes.c_float];lib.native_scale_add.restype=None
rng=random.Random(0x804B911C)
cases=[([1,2,0],[2,0,0],-2.0),([0,0,0],[0,0,0],0.0),([-0.0,0,-0.0],[0,-0.0,0],-0.0),
 ([ppc.value(0x3f800001),1,0],[ppc.value(0x3f7ffffe),-1,0],ppc.value(0x3f7ffffe)),
 ([0,ppc.value(0x3f800001),1],[0,1,ppc.value(0x3f7ffffe)],1.0)]
for _ in range(5000):
 p,q=([ppc.f32(math.ldexp(rng.uniform(-1,1),rng.randrange(-10,11))) for _ in range(3)] for _ in range(2))
 cases.append((p,q,ppc.f32(rng.uniform(-3,3))))
P,Q,O=0x10000,0x11000,0x12000
symbolic={P+i*4:('p',i) for i in range(3)}|{Q+i*4:('q',i) for i in range(3)}
assert execute('dot',symbolic)==execute('tvec_dot',symbolic),'TVec dot operation graph differs from SDK dot'
count=0
for p,q,scale in cases:
 memory={P+i*4:x for i,x in enumerate(p)}|{Q+i*4:x for i,x in enumerate(q)}
 a=(ctypes.c_float*3)(*p);b=(ctypes.c_float*3)(*q)
 expected=execute('dot',memory);actual=lib.PSVECDotProduct(a,b)
 assert ppc.bits(expected)==ppc.bits(actual),(p,q,'dot',actual,expected)
 assert ppc.bits(execute('tvec_dot',memory))==ppc.bits(actual)
 count+=1
 for name in ('cross','scale'):
  for alias in (None,P,Q):
   a=(ctypes.c_float*3)(*p);b=(ctypes.c_float*3)(*q);out=a if alias==P else b if alias==Q else (ctypes.c_float*3)()
   expected=execute(name,memory,out=alias or O,scale=scale)
   if name=='cross':lib.PSVECCrossProduct(a,b,out)
   else:lib.native_scale_add(a,b,out,scale)
   assert list(map(ppc.bits,out))==list(map(ppc.bits,expected)),(name,p,q,scale,alias,list(out),expected)
   count+=1
# Partially overlapping pointers exercise original load/store ordering beyond
# identical input/output aliases. All operations see the same bounded buffer.
for name in ('cross','scale'):
 for pindex,qindex,oindex in ((0,6,1),(1,6,0),(6,0,1),(6,1,0)):
  values=[float(i+1) for i in range(12)];data=(ctypes.c_float*12)(*values)
  memory={P+4*i:x for i,x in enumerate(values)}
  expected=execute(name,memory,P+4*pindex,P+4*qindex,P+4*oindex,2.0)
  pointer=lambda index:ctypes.cast(ctypes.byref(data,index*4),ptr)
  if name=='cross':lib.PSVECCrossProduct(pointer(pindex),pointer(qindex),pointer(oindex))
  else:lib.native_scale_add(pointer(pindex),pointer(qindex),pointer(oindex),2.0)
  assert list(map(ppc.bits,data[oindex:oindex+3]))==list(map(ppc.bits,expected)),(name,pindex,qindex,oindex,list(data[oindex:oindex+3]),expected)
  count+=1
print(f'PASS {count} actual native primitive calls match decoded retail output bits; TVec dot graph also matches')
# Regressions exercise the real public SDK/JGeometry/JMath surfaces. Instrument
# every linked source; this small executable has no uninstrumented game archive.
sources=['pc-port/tests/OriginalCameraVectorMathTests.cpp','pc-port/src/compat/JMathVectorCompat.cpp',
         'pc-port/src/JSystem/JMath/JMATrigonometricTable.cpp','pc-port/src/render/JMathTrig.cpp']
for label,san in (('optimized',[]),('asan',['-fsanitize=address,undefined','-fno-omit-frame-pointer'])):
 obj=BUILD/('vec-'+label+'.o');run([CC,'-std=c11','-O2',*san,*includes,'-c','pc-port/aurora/lib/dolphin/mtx/vec.c','-o',str(obj)],label+'-vec-compile')
 exe=BUILD/('vector-tests-'+label);run([CXX,'-std=c++23','-O2',*san,*includes,*sources,str(obj),'-o',str(exe)],label+'-compile')
 env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 run([str(exe)],label+'-tests',env)
tracked=sources+['pc-port/aurora/lib/dolphin/mtx/vec.c','pc-port/aurora/include/dolphin/mtx.h',
 'pc-port/aurora/include/dolphin/ppc_math.h','pc-port/src/JSystem/JMath/JMath.hpp','pc-port/src/JSystem/JGeometry/TVec.hpp']
report={'dol_sha1':hashlib.sha1(DOL).hexdigest(),'native_primitive_calls':count,'all_output_bits_equal_current_retail':True,
 'original_tvec_dot_operation_graph_equal_sdk_dot':True,'test_groups_per_mode':5,'native_modes':['optimized','ASan+UBSan'],
 'scope':'5005 moderate finite input pairs plus eight partial-overlap cases, including fused cancellation and signed zero. Regression separately verifies signed subnormal quantized stores. Does not claim complete FPSCR, signaling-NaN propagation, or subnormal arithmetic emulation.',
 'commands':commands,'source_sha256':{path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in tracked}}
(HERE/'vector-native-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
