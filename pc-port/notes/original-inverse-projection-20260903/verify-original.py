#!/usr/bin/env python3
"""Compile the actual root TU and prove all inverse-projection control/data paths.

The verifier interprets every instruction in both the relocated compiled routine
and the current DOL. SDK matrix helpers are preserved as identical call nodes,
not replaced by a host inverse-projection implementation.
"""
import hashlib,importlib.util,itertools,json,re,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-inverse-projection-20260903';BUILD.mkdir(parents=True,exist_ok=True)
ADDRESS,SIZE,SDA2=0x80404078,0x234,0x806BFC20
SYMBOL='invProject__6TDDrawFPQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PA4_fPCfPCfb'
CONSTANTS={'4b7fffff':0x806C1944,'3f000000':0x806C1928,'00000000':0x806C1924,'4330000080000000':0x8053F5B8}
CALLS={0x805189F8:'_savegpr_25',0x803FF5A4:'isScreen16Per9__2MRFv',0x803F81E0:'getScreenWidth__2MRFv',
       0x802B5724:'getFrameBufferWidth__2MRFv',0x804B848C:'PSMTXInverse',0x804B8CC0:'PSMTXMultVec',0x80518A44:'_restgpr_25'}
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
def signed(x,n):return x-(1<<n) if x&(1<<(n-1)) else x
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def run(command,log):
 p=subprocess.run([str(x) for x in command],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (BUILD/log).write_text(p.stdout);p.check_returncode()

def relocate(elf,dol):
 _,start,size,section=next(s for s in elf.symbols if s[0]==SYMBOL)
 code=bytearray(elf.section_data(section)[start:start+size]);refs=elf.references(SYMBOL)
 reverse_calls={name:address for address,name in CALLS.items()}
 for ref in refs:
  offset=int(ref['offset'],16);kind=ref['kind'];assert ref['addend']==0
  if kind==10:
   target=reverse_calls[ref['symbol']];word=struct.unpack_from('>I',code,offset)[0]
   struct.pack_into('>I',code,offset,(word&0xfc000003)|((target-ADDRESS-offset)&0x3fffffc))
  else:
   target=CONSTANTS[ref['value_hex']];assert reader.dol_bytes(dol,target,len(ref['value_hex'])//2).hex()==ref['value_hex']
   if kind==109:
    word=struct.unpack_from('>I',code,offset)[0];assert -32768<=target-SDA2<=32767
    struct.pack_into('>I',code,offset,(word&0xffe00000)|(2<<16)|((target-SDA2)&0xffff))
   elif kind==6:struct.pack_into('>H',code,offset,((target+0x8000)>>16)&0xffff)
   elif kind==4:struct.pack_into('>H',code,offset,target&0xffff)
   else:raise AssertionError(ref)
  ref['actual_retail_target']=hex(target)
 return bytes(code),refs

def arithmetic(op,a,b=None):
 # Commuting one add/multiply is permitted; no reassociation, contraction,
 # cancellation or division transformations are performed.
 if op in ('fadds','fmuls'):a,b=sorted((a,b),key=repr)
 return (op,a) if b is None else (op,a,b)

class Machine:
 def __init__(self,code,dol,normalized,wide,perspective):
  self.code,self.dol=code,dol;self.normalized,self.wide,self.perspective=normalized,wide,perspective
  self.g=[('incoming-gpr',i) for i in range(32)];self.f=[('incoming-fpr',i) for i in range(32)]
  self.g[1],self.g[2]=0x200000,SDA2;self.g[3:9]=[0x10000,0x11000,0x12000,0x13000,0x14000,int(normalized)]
  self.initial_g=self.g[:];self.initial_f=self.f[:];self.mem={};self.cr={};self.lr=0x300000
  self.calls=[];self.conditions=[];self.visited=[]
  for base,name,count in ((0x11000,'screen',3),(0x12000,'view',12),(0x13000,'projection',7),(0x14000,'viewport',6)):
   for i in range(count):self.mem[(base+i*4,4)]=(name,i)
 def load(self,at,size):
  if (at,size) in self.mem:return self.mem[(at,size)]
  if at>=0x80000000:
   return ('f32' if size==4 else 'f64',reader.dol_bytes(self.dol,at,size).hex())
  if size==8 and (at,4) in self.mem and (at+4,4) in self.mem:
   return ('f64-from-words',self.mem[(at,4)],self.mem[(at+4,4)])
  raise AssertionError(('uninitialized load',hex(at),size))
 def store(self,at,size,value):self.mem[(at,size)]=value
 def matrix(self,p):return tuple(self.load(p+i*4,4) for i in range(12))
 def call(self,address):
  name=CALLS[address]
  if name=='_savegpr_25':
   for i in range(25,32):self.store(self.g[11]-(32-i)*4,4,self.g[i])
  elif name=='_restgpr_25':
   for i in range(25,32):self.g[i]=self.load(self.g[11]-(32-i)*4,4)
  elif name=='isScreen16Per9__2MRFv':self.calls.append((name,));self.g[3]=int(self.wide)
  elif name in ('getScreenWidth__2MRFv','getFrameBufferWidth__2MRFv'):
   self.calls.append((name,));self.g[3]=('i32-return',name)
  elif name=='PSMTXInverse':
   assert self.g[3]==0x12000
   arg=self.matrix(self.g[3]);self.calls.append((name,arg,self.g[4]-self.g[1]))
   for i in range(12):self.store(self.g[4]+i*4,4,('PSMTXInverse-result',arg,i))
   self.g[3]=('PSMTXInverse-return',)
  elif name=='PSMTXMultVec':
   assert self.g[5]==0x10000
   matrix=self.matrix(self.g[3]);point=tuple(self.load(self.g[4]+i*4,4) for i in range(3))
   self.calls.append((name,matrix,point,'output'))
   for i in range(3):self.store(self.g[5]+i*4,4,('PSMTXMultVec-result',matrix,point,i))
  else:raise AssertionError(name)
 def execute(self):
  pc=0
  for _ in range(300):
   assert 0<=pc<len(self.code);self.visited.append(pc)
   word=struct.unpack_from('>I',self.code,pc)[0]
   op,d,a,b,c=word>>26,(word>>21)&31,(word>>16)&31,(word>>11)&31,(word>>6)&31
   imm=signed(word&65535,16);next_pc=pc+4
   if word==0x4e800020:
    assert self.g[1]==self.initial_g[1] and self.lr==0x300000
    assert self.g[25:]==self.initial_g[25:] and self.f[29:]==self.initial_f[29:]
    return {'conditions':self.conditions,'calls':self.calls,'output':tuple(self.load(0x10000+i*4,4) for i in range(3))}
   if op in (14,15):self.g[d]=((self.g[a] if a else 0)+(imm if op==14 else imm<<16))&0xffffffff
   elif op==27:
    v=self.g[d];mask=(word&65535)<<16
    self.g[a]=v^mask if isinstance(v,int) else ('xor32',v,mask)
   elif op in (36,37):
    at=(self.g[a]+imm)&0xffffffff;self.store(at,4,self.g[d])
    if op==37:self.g[a]=at
   elif op==32:self.g[d]=self.load((self.g[a]+imm)&0xffffffff,4)
   elif op in (48,50):self.f[d]=self.load((self.g[a]+imm)&0xffffffff,4 if op==48 else 8)
   elif op in (52,54):self.store((self.g[a]+imm)&0xffffffff,4 if op==52 else 8,self.f[d])
   elif op in (56,60):
    # Here paired loads/stores only preserve callee-saved registers, never
    # transform input data. Model the complete register identity as one item.
    at=self.g[a]+signed(word&4095,12);assert ((word>>12)&15)==0
    if op==56:self.f[d]=self.load(at,8)
    else:self.store(at,8,self.f[d])
   elif op==11:
    assert (word>>23)&7==0 and isinstance(self.g[a],int)
    self.cr[2]=self.g[a]==imm
    self.conditions.append(('integer-branch',a if a==8 else 'isScreen16Per9',imm,self.cr[2]))
   elif op==59:
    kind={18:'fdivs',20:'fsubs',21:'fadds',25:'fmuls'}[(word>>1)&31]
    self.f[d]=arithmetic(kind,self.f[a],self.f[c if kind=='fmuls' else b])
   elif op==63:
    xo=(word>>1)&1023
    if xo==40:self.f[d]=arithmetic('fneg',self.f[b])
    elif xo==72:self.f[d]=self.f[b]
    elif xo==0:
     assert (word>>23)&7==0
     pair=sorted((self.f[a],self.f[b]),key=repr);assert pair==[('f32','00000000'),('projection',0)]
     self.cr[2]=self.perspective;self.conditions.append(('projection-equals-zero-unordered',self.perspective))
    else:raise AssertionError((hex(pc),hex(word),'op63'))
   elif op==31:
    xo=(word>>1)&1023
    if xo==444:assert d==b;self.g[a]=self.g[d]
    elif word==0x7c0802a6:self.g[0]=self.lr
    elif word==0x7c0803a6:self.lr=self.g[0]
    else:raise AssertionError((hex(pc),hex(word),'op31'))
   elif op==16:
    assert a==2 and d in (4,12) and word&3==0
    taken=self.cr[2] if d==12 else not self.cr[2]
    if taken:next_pc=pc+signed(word&0xfffc,16)
   elif op==18:
    assert word&2==0;dest=pc+signed(word&0x3fffffc,26)
    if word&1:self.call(ADDRESS+dest)
    else:next_pc=dest
   else:raise AssertionError((hex(pc),hex(word),'opcode'))
   pc=next_pc
  raise AssertionError('Instruction limit')

def main():
 source=ROOT/'src/Game/Util/DirectDraw.cpp';header=ROOT/'include/Game/Util/DirectDraw.hpp'
 command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(BUILD/'DirectDraw.o')]
 run(command,'compile.log')
 target=ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/Game/Util/DirectDraw.o'
 run([ROOT/'build/tools/objdiff-cli','diff','-1',target,'-2',BUILD/'DirectDraw.o','-o',BUILD/'objdiff.json','--format','json-pretty'],'objdiff.log')
 diff=json.loads((BUILD/'objdiff.json').read_text());left,right=[next(s for s in diff[k]['symbols'] if s['name']==SYMBOL) for k in ('left','right')]
 assert left['match_percent']>=90 and int(left['size'])==SIZE and int(right['size'])==SIZE
 dol=(ROOT/'build/compat-math-oracle/main.dol').read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
 compiled,refs=relocate(reader.Elf(BUILD/'DirectDraw.o'),dol);retail=reader.dol_bytes(dol,ADDRESS,SIZE)
 (BUILD/'compiled-relocated.bin').write_bytes(compiled);(BUILD/'retail.bin').write_bytes(retail)
 branches=[];covered=[set(),set()]
 for normalized,wide,perspective in itertools.product((False,True),repeat=3):
  machines=[Machine(code,dol,normalized,wide,perspective) for code in (retail,compiled)]
  paths=[machine.execute() for machine in machines]
  assert paths[0]==paths[1],(normalized,wide,perspective,'complete symbolic trace mismatch')
  for i,machine in enumerate(machines):covered[i].update(machine.visited)
  branches.append({'normalized_depth':normalized,'wide_screen':wide,'perspective':perspective,
   'entire_path_equal':True,'path_sha256':hashlib.sha256(repr(paths[0]).encode()).hexdigest(),
   'retail_instruction_count':len(machines[0].visited),'compiled_instruction_count':len(machines[1].visited),
   'camera_space_point_graph':paths[0]['calls'][-1][2]})
 assert covered[0]==covered[1]==set(range(0,SIZE,4))
 assert [ref['symbol'] for ref in refs if ref['kind']==10]==list(CALLS.values())
 report={'symbol':SYMBOL,'address':hex(ADDRESS),'retail_bytes':SIZE,'compiled_bytes':len(compiled),
  'compiler':'GC/3.0a3 with configured Game flags and sjiswrap','command':command,'objdiff_percent':left['match_percent'],
  'dol_sha1':hashlib.sha1(dol).hexdigest(),'retail_sha256':hashlib.sha256(retail).hexdigest(),
  'source_sha256':{str(path.relative_to(ROOT)):sha(path) for path in (source,header,ROOT/'include/Game/Screen/StarPointerDirector.hpp',ROOT/'include/Game/NPC/TalkDirector.hpp',ROOT/'src/RVL_SDK/gx/GXTransform.c')},
  'relocations':refs,'matching_symbolic_paths':len(branches),'all_141_instruction_positions_executed_per_version':True,
  'method':'Every instruction interpreted; all arithmetic rounding nodes, branch predicates, constant bits, helper arguments/order, output writes, stack balance and saved registers agree on all paths. Only commutation within one finite add/multiply is canonicalized; no reassociation, contraction or division simplification.',
  'limits':'Original matrix inverse/multiply helpers are identical abstract call nodes, not reimplemented or run. No native SDK build, GPU, StarPointer owner or whole-game runtime validation.',
  'paths':branches}
 (HERE/'source-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
 (BUILD/'full-paths.json').write_text(json.dumps(branches,indent=2)+'\n')
 mismatch=[]
 for a,b in zip(left['instructions'],right['instructions']):
  if a.get('diff_kind') or b.get('diff_kind'):mismatch.append(a.get('instruction',{}).get('formatted','-')+' | '+b.get('instruction',{}).get('formatted','-'))
 (HERE/'compiler-differences.txt').write_text('\n'.join(mismatch)+'\n')
 print(f'PASS {left["match_percent"]}% objdiff; all eight complete symbolic paths and all141 instructions verified')
if __name__=='__main__':main()
