#!/usr/bin/env python3
"""Original-compiler and raw Gekko instruction oracle for the native matrix APIs."""
import ast, ctypes, hashlib, importlib.util, json, os, random, shlex, struct, subprocess, types
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-resource-holder-activation-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('original', HERE.parent / 'original-only-camera-20260903/verify-object.py')
original = importlib.util.module_from_spec(spec); spec.loader.exec_module(original)
dol = (ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
assert original.dol_bytes(dol,0x80004224,8) == bytes.fromhex('3c40806b6042fc20')
read = lambda addr, size: original.dol_bytes(dol,addr,size)
# Prove the native definitions mirror the root portable bodies, independent of
# their local formatting. This does not compare the existing perspective API.
root_text=(ROOT/'src/RVL_SDK/mtx/mtx44.c').read_text().split('#else\n\n// clang-format on\n\n',1)[1]
native_text=(ROOT/'pc-port/aurora/lib/dolphin/mtx/mtx44.c').read_text()
def body(text,name):
    begin=text.index('void '+name+'(');brace=text.index('{',begin);depth=0
    for end in range(brace,len(text)):
        if text[end]=='{':depth+=1
        if text[end]=='}':
            depth-=1
            if depth==0:return text[begin:end+1]
    raise AssertionError(name)
import re
for method in ('PSMTX44Identity','PSMTX44Copy'):
    assert re.sub(r'\s+','',body(root_text,method))==re.sub(r'\s+','',body(native_text,method))

original.CONSTANTS = {'3f800000':0x806c2108, '00000000':0x806c2110}
for value, address in original.CONSTANTS.items(): assert read(address,4).hex() == value

def run(command, log):
    with (HERE/log).open('w') as output:
        subprocess.run([str(x) for x in command],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True,timeout=60)

for node in ast.parse((ROOT/'configure.py').read_text()).body:
    if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='cflags_sdk' for t in node.targets):
        flags=eval(compile(ast.Expression(node.value),'configure.py','eval'),{'config':types.SimpleNamespace(version='RMGK01'),'version_num':0});break
command=['build/tools/wibo','build/tools/sjiswrap.exe','build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags: command.extend(shlex.split(flag))
command += ['-c','src/RVL_SDK/mtx/mtx44.c','-o',str(BUILD/'mtx44-ppc.o')]
run(command,'mtx44-original-compile.log')
elf=original.Elf(BUILD/'mtx44-ppc.o')
compiled=[]
for name, address, size in [('PSMTX44Identity',0x804b8f74,0x34),('PSMTX44Copy',0x804b8fa8,0x44)]:
    symbol=next(s for s in elf.symbols if s[0]==name)
    relocated,relocations=original.relocate(elf,symbol,address,{},dol)
    expected=read(address,size)
    compiled.append({'name':name,'retail_address':hex(address),'retail_size':size,'compiled_size':symbol[2],
                     'relocated_bytes_equal':relocated==expected,'relocations':relocations})
    assert relocated==expected, (name,relocated.hex(),expected.hex())

# Gekko float load and quantized float store bit algorithms. These reproduce
# Dolphin's hardware-tested ConvertToDouble and ConvertToSingleFTZ, independently
# of the production reduction to an exponent mask.
def to_double(x):
    exp=(x>>23)&255; fraction=x&0x7fffff
    if 0<exp<255:
        y=int(not (exp>>7));z=(y<<61)|(y<<60)|(y<<59)
        return ((x&0xc0000000)<<32)|z|((x&0x3fffffff)<<29)
    if exp==0 and fraction:
        exp=1023-126
        while True:
            fraction<<=1;exp-=1
            if fraction&0x800000:break
        return ((x&0x80000000)<<32)|(exp<<52)|((fraction&0x7fffff)<<29)
    y=exp>>7;z=(y<<61)|(y<<60)|(y<<59)
    return ((x&0xc0000000)<<32)|z|((x&0x3fffffff)<<29)
def store_float(x):
    return (((x>>32)&0xc0000000)|((x>>29)&0x3fffffff))&0xffffffff

def store_quantized(x):
    if ((x>>52)&2047)>896 or not(x&0x7fffffffffffffff):return store_float(x)
    return (x>>32)&0x80000000

def execute(address,size,words,src=0,dst=0):
    regs={2:0x806bfc20,3:src if address==0x804b8fa8 else dst,4:dst};fp={}
    result=words.copy()
    for offset in range(0,size,4):
        instruction=struct.unpack('>I',read(address+offset,4))[0]
        op=instruction>>26;rt=(instruction>>21)&31;ra=(instruction>>16)&31
        disp=instruction&0xffff;disp-=0x10000 if disp&0x8000 else 0
        if instruction==0x4e800020: return result
        if op==48: # lfs, used only for identity constants
            assert ra==2
            bits=struct.unpack('>I',read(regs[ra]+disp,4))[0]
            fp[rt]=(to_double(bits),to_double(bits))
        elif op==52: # stfs, only exact 0 or 1 here
            result[(regs[ra]+disp)//4]=store_float(fp[rt][0])
        elif op in (56,60): # psq_l / psq_st, GQR0 float pair
            assert ((instruction>>12)&15)==0
            disp=instruction&0xfff;disp-=0x1000 if disp&0x800 else 0
            index=(regs[ra]+disp)//4
            if op==56:fp[rt]=(to_double(result[index]),to_double(result[index+1]))
            else:result[index:index+2]=[store_quantized(v) for v in fp[rt]]
        else:raise AssertionError(f'unhandled instruction {instruction:08x}')
    raise AssertionError('missing return')

clang='/opt/homebrew/opt/llvm/bin/clang'; cxx=clang+'++'
run([clang,'-DAURORA','-DTARGET_PC','-O2','-dynamiclib','-Ipc-port/aurora/include','pc-port/aurora/lib/dolphin/mtx/mtx44.c','-o',BUILD/'libmtx44.dylib'],'mtx44-oracle-build.log')
lib=ctypes.CDLL(str(BUILD/'libmtx44.dylib'));lib.PSMTX44Copy.argtypes=[ctypes.c_void_p,ctypes.c_void_p];lib.PSMTX44Identity.argtypes=[ctypes.c_void_p]
rng=random.Random(0x804b8fa8);cases=0
for case in range(12000):
    words=[rng.getrandbits(32) for _ in range(48)]
    source=16
    dest=source+rng.choice([-15,-8,-1,0,1,2,3,8,15,16])
    expected=execute(0x804b8fa8,0x44,words,source*4,dest*4)
    buffer=(ctypes.c_uint32*48)(*words)
    lib.PSMTX44Copy(ctypes.byref(buffer,source*4),ctypes.byref(buffer,dest*4))
    assert list(buffer)==expected,case
    cases+=1
words=[0xdeadbeef]*20
expected=execute(0x804b8f74,0x34,words,dst=8)
buffer=(ctypes.c_uint32*20)(*words);lib.PSMTX44Identity(ctypes.byref(buffer,8));assert list(buffer)==expected
for label,flags in [('normal',['-O2']),('asan',['-O1','-fsanitize=address,undefined'])]:
    obj=BUILD/('mtx44-'+label+'.o');binary=BUILD/('mtx44-'+label+'-tests')
    run([clang,'-DAURORA','-DTARGET_PC',*flags,'-Ipc-port/aurora/include','-c','pc-port/aurora/lib/dolphin/mtx/mtx44.c','-o',obj],'mtx44-'+label+'-c.log')
    run([cxx,'-DAURORA','-DTARGET_PC','-std=c++23',*flags,'-Ipc-port/aurora/include','pc-port/aurora/tests/mtx44_test.cpp',obj,'-o',binary],'mtx44-'+label+'-build.log')
    run([binary],'mtx44-'+label+'.log')
    print((HERE/('mtx44-'+label+'.log')).read_text(),end='')
evidence={'dol_sha1':hashlib.sha1(dol).hexdigest(),'compiler_command':command,'compiled':compiled,
          'random_raw_instruction_copy_cases':cases,'identity_raw_instruction_cases':1,
          'integer_bit_patterns_include': 'normal, subnormal, signed zero, infinity, quiet/signaling NaN',
          'scope': 'GQR0=0 as required by original SDK; memory bits verified, host FPSCR/traps not emulated'}
(HERE/'mtx44-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS original relocated bytes and 12000 raw retail/native copy cases')
