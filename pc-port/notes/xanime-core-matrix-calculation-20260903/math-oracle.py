#!/usr/bin/env python3
"""Independent raw Gekko instruction oracle for quaternion helpers.

No whole-game/native xmake build. Compile real production providers in a small
shared library, then compare them to decoded retail words. Fused arithmetic uses
libSystem fmaf; reciprocal uses Dolphin's hardware-derived lookup constants.
"""
import argparse
import ast
import ctypes
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import re
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/xanime-core-matrix-calculation-20260903/math'
DOL=ROOT/'build/compat-math-oracle/main.dol'
SPECS={'matrix':(0x804B8928,0xA4),'euler':(0x8044268C,0xD0),'lerp':(0x8044275C,0xFC)}
P,Q,OUT=0x10000,0x11000,0x12000
TABLE=0x8060FC80
SYSTEM=ctypes.CDLL(None)
SYSTEM.fmaf.argtypes=[ctypes.c_float]*3
SYSTEM.fmaf.restype=ctypes.c_float


def f32(x):
    try: return struct.unpack('>f',struct.pack('>f',x))[0]
    except OverflowError: return math.copysign(math.inf,x)


def bits(x): return struct.unpack('>I',struct.pack('>f',x))[0]
def value(x): return struct.unpack('>f',struct.pack('>I',x))[0]
def signed(x,n): return x-(1<<n) if x&(1<<(n-1)) else x


def dol_read(data,address,size):
    for index in range(18):
        offset,base,length=[struct.unpack_from('>I',data,field+index*4)[0] for field in (0,0x48,0x90)]
        if base<=address and address+size<=base+length:
            at=offset+address-base
            return data[at:at+size]
    raise AssertionError((hex(address),size))


FRES_TABLE=[(int(a,16),int(b,16)) for a,b in re.findall(r'\{(0x[0-9a-f]+), (0x[0-9a-f]+)\}',
    (ROOT/'pc-port/dolphin/Source/Core/Common/FloatUtils.cpp').read_text().split('fres_expected',1)[1].split('};',1)[0])]
assert len(FRES_TABLE)==32


def reciprocal(x):
    raw=struct.unpack('>Q',struct.pack('>d',x))[0]
    mantissa,exponent,sign=raw&((1<<52)-1),(raw>>52)&2047,raw>>63
    if exponent==0 and mantissa==0: return math.copysign(math.inf,x)
    if exponent==2047:
        return x if mantissa else math.copysign(0.0,x)
    if exponent<895: return value((sign<<31)|0x7F7FFFFF)
    if exponent>=1149: return math.copysign(0.0,x)
    index=mantissa>>37
    base,decrement=FRES_TABLE[index//1024]
    result=(sign<<63)|((2045-exponent)<<52)|((base-(decrement*(index%1024)+1)//2)<<29)
    return struct.unpack('>d',struct.pack('>Q',result))[0]


def op(name,*args):
    if not any(isinstance(a,tuple) for a in args):
        if name=='mul': return f32(args[0]*args[1])
        if name=='add': return f32(args[0]+args[1])
        if name=='sub': return f32(args[0]-args[1])
        if name=='neg': return -args[0]
        if name=='fma': return float(SYSTEM.fmaf(*args))
        if name=='fres': return reciprocal(args[0])
        raise AssertionError(name)
    if name in ('mul','add'): args=tuple(sorted(args,key=repr))
    if name=='fma': args=(*sorted(args[:2],key=repr),args[2])
    if name=='neg' and isinstance(args[0],tuple) and args[0][0]=='neg': return args[0][1]
    return (name,*args)


class Oracle:
    def __init__(self,dol,table,graph=False,negative=False):
        self.dol,self.table,self.graph,self.negative=dol,table,graph,negative
        self.g=[0]*32;self.f=[[0.0,0.0] for _ in range(32)];self.cr=[False]*32
        self.g[2]=0x806BFC20
        self.memory={};self.ca=0;self.dot=None
        self.graph_table={}

    def load(self,address,size=4):
        if address in self.memory: return self.memory[address]
        if TABLE<=address<TABLE+16384*8:
            if self.graph: return self.graph_table[address]
            return self.table[(address-TABLE)//4]
        return struct.unpack('>f' if size==4 else '>d',dol_read(self.dol,address,size))[0]

    def execute(self,kind,p=None,q=None,t=0.0,angles=None,alias=None):
        address,size=SPECS[kind]
        self.g[3:7]=[P,Q,OUT,OUT]
        if kind=='matrix': self.g[3:5]=[OUT,Q]
        elif kind=='euler':
            self.g[3:7]=[*(x&0xFFFFFFFF for x in angles),OUT]
            for axis,x in zip('xyz',angles):
                index=(int(x/2)&65535)>>2
                self.graph_table[TABLE+index*8]=('sin',axis)
                self.graph_table[TABLE+index*8+4]=('cos',axis)
        for at,values in ((P,p),(Q,q)):
            if values is not None:
                for i,x in enumerate(values): self.memory[at+4*i]=x
        if kind=='lerp': self.f[1]=[t,t]
        if alias is not None:
            if kind=='matrix': self.g[3]=Q
            else: self.g[5]=P if alias=='p' else Q
        output=self.g[3] if kind=='matrix' else self.g[6] if kind=='euler' else self.g[5]
        pc=address
        for _ in range(300):
            word=int.from_bytes(dol_read(self.dol,pc,4),'big')
            if word==0x4E800020: return [self.memory[output+4*i] for i in range(12 if kind=='matrix' else 4)]
            code,d,a,b,c=word>>26,(word>>21)&31,(word>>16)&31,(word>>11)&31,(word>>6)&31
            nxt=pc+4;imm=signed(word&65535,16)
            if code in (14,15): self.g[d]=((self.g[a] if a else 0)+(imm if code==14 else imm<<16))&0xFFFFFFFF
            elif code==21:
                sh,mb,me=b,c,(word>>1)&31
                rotated=((self.g[d]<<sh)|(self.g[d]>>(32-sh)))&0xFFFFFFFF
                mask=sum(1<<(31-i) for i in range(mb,me+1))
                self.g[a]=rotated&mask
            elif code==31:
                xo=(word>>1)&1023
                if xo==824:
                    original=signed(self.g[d],32)
                    self.g[a]=(original>>b)&0xFFFFFFFF
                    self.ca=int(original<0 and (self.g[d]&((1<<b)-1))!=0)
                elif xo==202: self.g[d]=(self.g[a]+self.ca)&0xFFFFFFFF
                elif xo==266: self.g[d]=(self.g[a]+self.g[b])&0xFFFFFFFF
                elif xo==535:
                    x=self.load((self.g[a]+self.g[b])&0xFFFFFFFF);self.f[d]=[x,x]
                else: raise AssertionError((hex(pc),hex(word),'op31'))
            elif code in (48,50):
                x=self.load((self.g[a]+imm)&0xFFFFFFFF,4 if code==48 else 8)
                self.f[d]=[x,x if code==48 else self.f[d][1]]
            elif code==52: self.memory[(self.g[a]+imm)&0xFFFFFFFF]=self.f[d][0]
            elif code in (56,60):
                offset=signed(word&4095,12);w=(word>>15)&1
                assert ((word>>12)&7)==0
                at=(self.g[a]+offset)&0xFFFFFFFF
                if code==56: self.f[d]=[self.load(at),1.0 if w else self.load(at+4)]
                else:
                    self.memory[at]=self.f[d][0]
                    if not w: self.memory[at+4]=self.f[d][1]
            elif code==59:
                xo=(word>>1)&31
                if xo in (20,21): x=op('sub' if xo==20 else 'add',self.f[a][0],self.f[b][0])
                elif xo==25: x=op('mul',self.f[a][0],self.f[c][0])
                elif xo==24: x=op('fres',self.f[b][0])
                else: raise AssertionError((hex(pc),hex(word),'op59'))
                self.f[d]=[x,x]
            elif code==63:
                xo=(word>>1)&1023
                if xo==40: self.f[d]=[op('neg',self.f[b][0]),self.f[d][1]]
                elif xo in (0,32):
                    self.dot=self.f[a][0]
                    self.cr[0]=self.negative if self.graph else self.dot<self.f[b][0]
                else: raise AssertionError((hex(pc),hex(word),'op63'))
            elif code==4:
                xo=(word>>1)&1023;short=xo&31
                A,B,C=self.f[a][:],self.f[b][:],self.f[c][:]
                if xo in (528,560,592,624): self.f[d]=[A[(xo-528)//64],B[((xo-528)//32)%2]]
                elif short==10: self.f[d]=[op('add',A[0],B[1]),C[1]]
                elif short==11: self.f[d]=[C[0],op('add',A[0],B[1])]
                elif short in (12,13): self.f[d]=[op('mul',A[i],C[short-12]) for i in range(2)]
                elif short==25: self.f[d]=[op('mul',A[i],C[i]) for i in range(2)]
                elif short in (14,15): self.f[d]=[op('fma',A[i],C[short-14],B[i]) for i in range(2)]
                elif short in (28,29,30):
                    self.f[d]=[op('fma',A[i],C[i],B[i] if short==29 else op('neg',B[i])) for i in range(2)]
                    if short==30:
                        # Hardware ps_nmsub leaves NaN sign/payload unchanged.
                        self.f[d]=[op('neg',x) if isinstance(x,tuple) or not math.isnan(x) else x for x in self.f[d]]
                else: raise AssertionError((hex(pc),hex(word),'paired'))
            elif code==16:
                assert d==4 and a==0
                if not self.cr[0]: nxt=pc+signed(word&0xFFFC,16)
            else: raise AssertionError((hex(pc),hex(word),code))
            pc=nxt
        raise AssertionError('Instruction budget')


def function_body(text,marker):
    start=text.index(marker);at=text.index('{',start)+1;depth=1;end=at
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[at:end-1]


def assignments(body,initial):
    env=initial.copy();out={}
    def expr(n):
        if isinstance(n,ast.Name): return env[n.id]
        if isinstance(n,ast.Constant): return float(n.value)
        if isinstance(n,ast.BinOp): return op({ast.Mult:'mul',ast.Add:'add',ast.Sub:'sub'}[type(n.op)],expr(n.left),expr(n.right))
        if isinstance(n,ast.UnaryOp): return op('neg',expr(n.operand))
        if isinstance(n,ast.Call):
            if n.func.id in ('JMASSin','JMASCos'): return ('sin' if n.func.id=='JMASSin' else 'cos',n.args[0].left.id)
            return op('fres' if n.func.id=='ppc_fres' else 'fma',*[expr(a) for a in n.args])
        raise AssertionError(ast.dump(n))
    body=re.sub(r'//[^\n]*','',body)
    for raw in body.split(';'):
        line=raw.strip()
        if not line or '=' not in line or line.startswith('#'): continue
        line=re.sub(r'^.*?\b(?:const float|f32)\s+', '',line, count=1,flags=re.S)
        for item in re.split(r',\s*(?=[A-Za-z_]\w*\s*=)',line):
            lhs,rhs=item.split('=',1)
            lhs,rhs=lhs.strip(),rhs.strip().replace('std::fma','fmaf')
            rhs=re.sub(r'(\d(?:[\d.eE+-]*\d)?)f\b',r'\1',rhs)
            for name in ('q','p'):
                rhs=re.sub(name+r'->([xyzw])',name+r'_\1',rhs)
            result=expr(ast.parse(rhs,mode='eval').body)
            if lhs.startswith(('m[','quat->','dst->')): out[lhs]=result
            else: env[lhs]=result
    return out,env


def verify_graphs(dol,table):
    matrix_source=(ROOT/'pc-port/aurora/lib/dolphin/mtx/mtx.c').read_text()
    matrix_body=function_body(matrix_source,'void PSMTXQuat(')
    matrix_body=matrix_body[matrix_body.index('  const float x'):]
    inputs={f'q_{axis}':('q',axis) for axis in 'xyzw'}
    out,_=assignments(matrix_body,inputs)
    actual=Oracle(dol,table,graph=True).execute('matrix',q=list(inputs.values()))
    assert actual==[out[f'm[{r}][{c}]'] for r in range(3) for c in range(4)], 'PSMTXQuat graph differs'
    src=(ROOT/'src/JSystem/JMath/JMath.cpp').read_text()
    native=(ROOT/'pc-port/src/compat/JMathQuaternionCompat.cpp').read_text()
    for marker in ('void JMAEulerToQuat(', 'void JMAQuatLerp('):
        # Native import removes only legacy trailing spaces to pass git diff --check.
        strip_trailing = lambda body: '\n'.join(line.rstrip() for line in body.splitlines())
        assert strip_trailing(function_body(src,marker))==strip_trailing(function_body(native,marker)),marker+' root/native bodies differ'
    euler,_=assignments(function_body(src,'void JMAEulerToQuat('),{})
    actual=Oracle(dol,table,graph=True).execute('euler',angles=(-129,24577,32767))
    assert actual==[euler['quat->'+axis] for axis in 'xyzw'], 'Euler arithmetic graph differs'
    # Portable paired dot and both scalar branches use the same variables/ops
    # as the root source; compare them directly to raw retail symbolic output.
    lerp_body=function_body(src,'void JMAQuatLerp(')
    dot_body=lerp_body.split('#else',1)[1].split('#endif',1)[0]
    inputs={f'{name}_{axis}':(name,axis) for name in ('p','q') for axis in 'xyzw'}
    inputs['t']=('t',)
    _,dot=assignments(dot_body,inputs)
    negative_body=lerp_body.split('int unused;',1)[1].split('} else {',1)[0]
    positive_body=lerp_body.split('} else {',1)[1].rsplit('}',1)[0]
    graphs=[]
    for negative,body in ((True,negative_body),(False,positive_body)):
        expected,_=assignments(body,inputs)
        machine=Oracle(dol,table,graph=True,negative=negative)
        actual=machine.execute('lerp',p=[inputs['p_'+a] for a in 'xyzw'],q=[inputs['q_'+a] for a in 'xyzw'],t=inputs['t'])
        assert machine.dot==dot['dp']
        assert actual==[expected['dst->'+a] for a in 'xyzw'], 'Lerp branch graph differs'
        graphs.append(actual)
    return {'matrix_12_output_graph_sha256':hashlib.sha256(repr(out).encode()).hexdigest(),
            'euler_4_output_graph_sha256':hashlib.sha256(repr(euler).encode()).hexdigest(),
            'lerp_dot_and_8_branch_output_graph_sha256':hashlib.sha256(repr((dot['dp'],graphs)).encode()).hexdigest(),
            'all_graphs_equal_to_raw_retail':True,'root_native_jmath_function_bodies_equal_except_trailing_whitespace':True}


def verify_fres_migration():
    baseline = 'd40387003'
    old = subprocess.check_output(['git', 'show', baseline+':pc-port/src/compat/J3DJointCompat.cpp'], cwd=ROOT, text=True)
    new = (ROOT/'pc-port/aurora/include/dolphin/ppc_math.h').read_text()
    delegate = (ROOT/'pc-port/src/compat/J3DJointCompat.cpp').read_text()
    def table(text, marker):
        return [(int(a,16),int(b,16)) for a,b in re.findall(r'\{(0x[0-9a-f]+), (0x[0-9a-f]+)\}',
            text.split(marker,1)[1].split('};',1)[0])]
    old_table = table(old,'reciprocalEstimateTable[32]')
    new_table = table(new,'RECIPROCAL_TABLE[32]')
    assert old_table == new_table == FRES_TABLE
    old_body = function_body(old,'f32 JMath::fastReciprocal(')
    new_body = function_body(new,'static inline float ppc_fres(')
    def clean(text): return re.sub(r'\s+', '', re.sub(r'//[^\n]*', '', text))
    migrated = old_body.replace('const u32 bits = std::bit_cast<u32>(value);',
        'union c32 input; union c32 result; input.f = value; const u32 bits = input.u;')
    migrated = migrated.replace('u32', 'uint32_t')
    migrated = migrated.replace('static_cast<int>((bits >> 23) & 0xFFU)', '(int)((bits >> 23) & 0xFFU)')
    migrated = migrated.replace('static_cast<int>(shift)', '(int)shift')
    migrated = migrated.replace('static_cast<uint32_t>(253 - exponent)', '(uint32_t)(253 - exponent)')
    migrated = migrated.replace('const ReciprocalEstimateEntry& entry = reciprocalEstimateTable[fraction >> 18];',
        'const struct BaseAndDec32* entry = &RECIPROCAL_TABLE[fraction >> 18];')
    migrated = migrated.replace('entry.base', 'entry->base').replace('entry.decrement', 'entry->dec')
    migrated = re.sub(r'return std::bit_cast<f32>\(([^;]+)\);', r'result.u = \1; return result.f;', migrated)
    assert clean(migrated) == clean(new_body), 'Migrated reciprocal algorithm differs beyond mechanical C adaptation'
    assert clean(function_body(delegate,'f32 JMath::fastReciprocal(')) == 'returnppc_fres(value);'
    return {'baseline_commit':baseline, 'all_32_numeric_entries_unchanged':True,
            'body_unchanged_after_bit_cast_to_union_and_C_type_adaptation':True,
            'jmath_delegates_to_shared_aurora_function':True,
            'normalized_algorithm_sha256':hashlib.sha256(clean(new_body).encode()).hexdigest()}


def compile_native():
    common=['-O2','-fPIC','-DTARGET_PC','-DMTX_USE_PS','-Ipc-port/aurora/include']
    commands=[]
    for name in ('mtx','vec'):
        commands.append(['clang','-std=c11',*common,'-c',f'pc-port/aurora/lib/dolphin/mtx/{name}.c','-o',str(BUILD/(name+'.o'))])
    commands.append(['clang++','-std=c++20',*common,'-dynamiclib','-Ipc-port/src',
                     'pc-port/src/compat/JMathQuaternionCompat.cpp','pc-port/src/JSystem/JMath/JMATrigonometricTable.cpp',
                     str(HERE/'math-harness.cpp'),str(BUILD/'mtx.o'),str(BUILD/'vec.o'),'-o',str(BUILD/'libquaternion.dylib')])
    for i,command in enumerate(commands):
        result=subprocess.run(command,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (BUILD/f'compile-{i}.log').write_text(result.stdout)
        result.check_returncode()
    (BUILD/'compile-commands.json').write_text(json.dumps(commands,indent=2)+'\n')


def compare_native(dol,native,table,count):
    float_ptr=ctypes.POINTER(ctypes.c_float)
    native.oracle_native_mtx.argtypes=[float_ptr,float_ptr]
    native.oracle_native_lerp.argtypes=[float_ptr,float_ptr,ctypes.c_float,float_ptr]
    native.oracle_native_euler.argtypes=[ctypes.c_int]*3+[float_ptr]
    rng=random.Random(0x804B8928)
    totals={'matrix':0,'lerp':0,'euler':0,'alias':0,'exceptional':0}
    result_hash=hashlib.sha256();exceptions=[]
    def check(kind,p=None,q=None,t=0.0,angles=None,alias=None,exceptional=False):
        oracle=Oracle(dol,table)
        expected=oracle.execute(kind,p=p,q=q,t=t,angles=angles,alias=alias)
        pbuf=(ctypes.c_float*12)(*(p or []));qbuf=(ctypes.c_float*12)(*(q or []));out=(ctypes.c_float*12)()
        if alias is not None: out=pbuf if alias=='p' else qbuf
        if kind=='matrix': native.oracle_native_mtx(qbuf,out)
        elif kind=='lerp': native.oracle_native_lerp(pbuf,qbuf,t,out)
        else: native.oracle_native_euler(*angles,out)
        observed=list(out)[:len(expected)]
        if exceptional:
            def classification(x): return 'nan' if math.isnan(x) else 'inf+' if x==math.inf else 'inf-' if x==-math.inf else hex(bits(x))
            assert [classification(x) for x in observed]==[classification(x) for x in expected], (kind,p,q,t,observed,expected)
            exceptions.append({'kind':kind,'input_p_bits':[hex(bits(x)) for x in p or []],'input_q_bits':[hex(bits(x)) for x in q or []],
                               'output_classes':[classification(x) for x in observed]})
        else:
            assert list(map(bits,observed))==list(map(bits,expected)), (kind,p,q,t,angles,alias,list(map(hex,map(bits,observed))),list(map(hex,map(bits,expected))))
            result_hash.update(b''.join(struct.pack('>I',bits(x)) for x in observed))
        totals[kind]+=1;totals['alias']+=alias is not None;totals['exceptional']+=exceptional
    matrix_cases=[[0,0,0,1],[1,0,0,0],[0,1,0,0],[0,0,1,0],[1,2,3,4],[-1,2,-3,4],[-0.0,0.0,-0.0,1.0],
                  [0.125,-0.25,0.5,-1.0],[2**-30,2**-29,-2**-31,2**-32],[2**30,-2**29,2**31,2**32]]
    for q in matrix_cases:
        for alias in (None,'q'): check('matrix',q=q,alias=alias)
    for _ in range(count):
        magnitude=2.0**rng.randrange(-35,36)
        q=[f32(rng.uniform(-4,4)*magnitude) for _ in range(4)]
        check('matrix',q=q)
    for p,q in [([0,0,0,1],[0,0,0,-1]),([1,0,0,0],[0,1,0,0]),([1,2,3,4],[-5,-6,-7,-8]),
                ([-0.0,0.0,0.0,1.0],[0.0,-0.0,0.0,1.0])]:
        for t in (-1.0,-0.0,0.125,0.5,1.0,2.0):
            for alias in (None,'p','q'): check('lerp',p=p,q=q,t=t,alias=alias)
    for _ in range(count):
        p=[f32(rng.uniform(-4,4)) for _ in range(4)];q=[f32(rng.uniform(-4,4)) for _ in range(4)]
        check('lerp',p=p,q=q,t=f32(rng.uniform(-2,3)))
    boundaries=(-32768,-32767,-32765,-16385,-129,-9,-8,-7,-3,-2,-1,0,1,2,3,7,8,9,127,128,129,16383,32767)
    for angle in boundaries:
        for axis in range(3):
            a=[0,0,0];a[axis]=angle;check('euler',angles=a)
    for _ in range(count): check('euler',angles=[rng.randrange(-32768,32768) for _ in range(3)])
    # Exhaust every signed-short input in one coordinate while the other two
    # rotations remain nontrivial. This catches the negative-odd /2 boundary.
    for angle in range(-32768,32768): check('euler',angles=[angle,12345,-23457])
    # Keep overflow/underflow outcomes separate from the ordinary finite-output
    # corpus: finite output bits are still checked, NaNs are classification-only.
    for exponent in (-149,-140,-130,-120,-100,-80,-70,-65,-64,-63,-40,-1,0,1,40,62,63,64,65,100,120,127):
        magnitude=2.0**exponent
        for shape in ((1,0,0,0),(0,0,0,1),(1,0.5,-0.25,-0.5)):
            check('matrix',q=[f32(magnitude*x) for x in shape],exceptional=True)
    for q in ([0,0,0,0],[value(1),0,0,0],[math.inf,0,0,1],[-math.inf,1,2,3],[math.nan,1,2,3],[value(0x7F7FFFFF)]*4):
        check('matrix',q=q,exceptional=True)
    for p,q,t in [([math.inf,0,0,1],[0,1,0,0],0.5),([math.nan,0,0,1],[1,0,0,0],0.5),([1,2,3,4],[4,3,2,1],math.inf)]:
        check('lerp',p=p,q=q,t=t,exceptional=True)
    return {'cases':totals,'finite_output_bits_sha256':result_hash.hexdigest(),'exceptions':exceptions,
            'finite_comparison':'All output bits, including signed zero. Exceptional cases compare NaN classification, infinity sign, and all finite bits; NaN payload/sign and FPSCR exceptions are not asserted.'}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--cases',type=int,default=4096);parser.add_argument('--no-build',action='store_true');args=parser.parse_args()
    BUILD.mkdir(parents=True,exist_ok=True)
    dol=DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
    if not args.no_build: compile_native()
    native=ctypes.CDLL(str(BUILD/'libquaternion.dylib'))
    native.oracle_native_table.restype=ctypes.POINTER(ctypes.c_float)
    table=list(native.oracle_native_table()[:32768])
    graphs=verify_graphs(dol,table)
    numerical=compare_native(dol,native,table,args.cases)
    files=['pc-port/aurora/lib/dolphin/mtx/mtx.c','pc-port/aurora/include/dolphin/ppc_math.h','pc-port/aurora/include/dolphin/mtx.h',
           'pc-port/src/compat/JMathQuaternionCompat.cpp','src/JSystem/JMath/JMath.cpp','pc-port/src/JSystem/JMath/JMATrigonometricTable.cpp']
    evidence={'scope':'Raw retail arithmetic graph and native standalone helper verification; no whole-game build. Euler indexing/math uses the same retained table values, so this does not independently verify table initialization.',
              'dol_sha1':hashlib.sha1(dol).hexdigest(),'compiler':subprocess.check_output(['clang','--version'],text=True).splitlines()[0],
              'native_library_sha256':hashlib.sha256((BUILD/'libquaternion.dylib').read_bytes()).hexdigest(),
              'functions':{name:{'address':hex(addr),'size':size,'retail_sha256':hashlib.sha256(dol_read(dol,addr,size)).hexdigest()} for name,(addr,size) in SPECS.items()},
              'graphs':graphs,'shared_fres_migration':verify_fres_migration(),'native_comparison':numerical,'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in files}}
    (BUILD/'math-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print(json.dumps(evidence,indent=2))


if __name__=='__main__': main()
