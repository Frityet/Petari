"""Bounded numerical execution of actual RMGK01 scale-method instructions.

The small strict interpreter accepts only the instructions present in the three
recovered methods. Calls to existing SDK matrix/vector routines use their public
arithmetic contracts. This tests recovered control flow and field/global effects;
it is not a test of native SDK FMA/paired-single implementations or a game run.
"""
import math
import struct


def f32(value):
    return struct.unpack('>f', struct.pack('>f', value))[0]


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits-1)) else value


IDENTITY = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0]
CURRENT = 0x8060D61C
CURRENT_S = 0x8060D64C
PARENT_S = 0x8060D658
CORE, JOINT, BUFFER, FLAGS, MATRICES, TRANSFORMS, INFO = 0x10000, 0x11000, 0x12000, 0x13000, 0x14000, 0x15000, 0x16000
SCALE, TRANSLATION = 0x17000, 0x18000
JOINT_INDEX = 1
MATRIX = MATRICES+48
TRANSFORM = TRANSFORMS+0x70
MAYA, SI, SPECIAL = 0x80019F74, 0x8001A688, 0x8001AC5C
R = 0.9998779296875  # Retail fres for powers of two: r / operand.


class Machine:
    def __init__(self, dol, read_dol):
        self.dol, self.read_dol = dol, read_dol
        self.mem = {}
        self.g = [0]*32
        self.f = [0.0]*32
        self.cr = [False]*32
        self.lr = 0
        self.g[1], self.g[2], self.g[13] = 0x200000, 0x806BFC20, 0x806B9620
        self.calls = []
        self.branches = set()
        self.write(0x806B72AC, JOINT)
        self.write(0x806B72A8, BUFFER)
        self.write(JOINT+0x14, JOINT_INDEX, 2)
        self.write(BUFFER+4, FLAGS)
        self.write(BUFFER+12, MATRICES)
        self.write(CORE+16, INFO)
        self.vector(CURRENT, IDENTITY)
        self.vector(MATRIX, IDENTITY)
        self.vector(CURRENT_S, (1, 1, 1))
        self.vector(PARENT_S, (1, 1, 1))
        self.vector(SCALE, (1, 1, 1))
        self.vector(TRANSLATION, (0, 0, 0))
        self.g[3:6] = [CORE, SCALE, TRANSLATION]

    def read(self, addr, size=4):
        if all(addr+i in self.mem for i in range(size)):
            data = bytes(self.mem[addr+i] for i in range(size))
        elif addr >= 0x80000000:
            data = self.read_dol(self.dol, addr, size)
        else:
            data = bytes(self.mem.get(addr+i, 0) for i in range(size))
        return int.from_bytes(data, 'big')

    def write(self, addr, value, size=4):
        data = (value & ((1 << (size*8))-1)).to_bytes(size, 'big')
        self.mem.update({addr+i: byte for i, byte in enumerate(data)})

    def getf(self, addr):
        return struct.unpack('>f', self.read(addr).to_bytes(4, 'big'))[0]

    def putf(self, addr, value):
        self.write(addr, int.from_bytes(struct.pack('>f', value), 'big'))

    def vector(self, addr, values):
        for i, value in enumerate(values):
            self.putf(addr+4*i, value)

    def values(self, addr, count=3):
        return [self.getf(addr+4*i) for i in range(count)]

    def enable_transform(self):
        self.write(CORE+20, TRANSFORMS)
        self.write(TRANSFORM+4, 0xFFFF, 2)
        self.vector(TRANSFORM+8, (1, 1, 1))
        self.vector(TRANSFORM+0x14, (1, 1, 1))
        self.vector(TRANSFORMS+8, (1, 1, 1))

    def call(self, target):
        self.calls.append(hex(target))
        if 0x805189CC <= target <= 0x80518A10:  # _savegpr_14..31
            first = 14+(target-0x805189CC)//4
            for i in range(first, 32):
                self.write(self.g[11]-(32-i)*4, self.g[i])
        elif 0x80518A18 <= target <= 0x80518A5C:  # _restgpr_14..31
            first = 14+(target-0x80518A18)//4
            for i in range(first, 32):
                self.g[i] = self.read(self.g[11]-(32-i)*4)
        elif target in (0x80018EF0, 0x80018E78):  # TVec3 copy constructor / assignment
            self.vector(self.g[3], self.values(self.g[4]))
        elif target == 0x804B838C:  # PSMTXCopy
            self.vector(self.g[4], self.values(self.g[3], 12))
        elif target == 0x804B83C0:  # PSMTXConcat, aliasing permitted
            a, b = self.values(self.g[3], 12), self.values(self.g[4], 12)
            result = [f32(sum(a[4*i+k]*b[4*k+j] for k in range(3))+(a[4*i+3] if j==3 else 0))
                      for i in range(3) for j in range(4)]
            self.vector(self.g[5], result)
        elif target == 0x804B848C:  # PSMTXInverse; fixtures use invertible exact matrices
            m = self.values(self.g[3], 12)
            a,b,c,d,e,f,g,h,i = [m[r*4+c] for r in range(3) for c in range(3)]
            co = [e*i-f*h, c*h-b*i, b*f-c*e, f*g-d*i, a*i-c*g, c*d-a*f, d*h-e*g, b*g-a*h, a*e-b*d]
            determinant = a*co[0]+b*co[3]+c*co[6]
            assert determinant != 0
            inv = [x/determinant for x in co]
            result = []
            for r in range(3):
                result += inv[3*r:3*r+3]+[-sum(inv[3*r+k]*m[k*4+3] for k in range(3))]
            self.vector(self.g[4], result)
            self.g[3] = 1
        elif target == 0x804428A8:  # JMAMTXApplyScale = source * diagonal scale
            m = self.values(self.g[3], 12)
            self.vector(self.g[4], [f32(value*(self.f[1+c] if c<3 else 1)) for r in range(3) for c,value in enumerate(m[4*r:4*r+4])])
        elif target == 0x8001B140:  # Existing fres helper: these cases deliberately use exact powers of two.
            mantissa, _ = math.frexp(abs(self.f[1]))
            assert mantissa == 0.5
            self.f[1] = f32(R/self.f[1])
        elif target == 0x803ED1B0:
            self.vector(self.g[4], self.values(self.g[3],12)[3::4])
        elif target == 0x803ECFB0:
            for i in range(3): self.putf(self.g[3]+i*16+12, self.f[1+i])
        elif target == 0x804B8928:
            x,y,z,w = self.values(self.g[4],4)
            self.vector(self.g[3], [1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),0,
                                    2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),0,
                                    2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),0])
        elif target == MAYA:
            self.execute(MAYA)
        else:
            raise AssertionError(f'Unsupported helper {target:#x}')

    def execute(self, address):
        pc = address
        for _ in range(3000):
            word = int.from_bytes(self.read_dol(self.dol, pc, 4),'big')
            op, d, a, b, c = word>>26, (word>>21)&31, (word>>16)&31, (word>>11)&31, (word>>6)&31
            imm = signed(word&65535,16)
            next_pc = pc+4
            if word == 0x4E800020: return
            if op in (14,15): self.g[d] = ((self.g[a] if a else 0)+(imm if op==14 else imm<<16)) & 0xFFFFFFFF
            elif op == 7: self.g[d] = (signed(self.g[a],32)*imm) & 0xFFFFFFFF
            elif op in (10,11):
                left = self.g[a] if op==10 else signed(self.g[a],32)
                right = word&65535 if op==10 else imm
                field = (word>>23)&7
                self.cr[field*4:field*4+4] = [left<right,left>right,left==right,False]
            elif op in (32,34,40): self.g[d] = self.read((self.g[a]+imm)&0xFFFFFFFF,{32:4,34:1,40:2}[op])
            elif op in (36,37):
                ea = (self.g[a]+imm)&0xFFFFFFFF
                self.write(ea,self.g[d])
                if op==37: self.g[a] = ea
            elif op == 48: self.f[d] = self.getf((self.g[a]+imm)&0xFFFFFFFF)
            elif op == 52: self.putf((self.g[a]+imm)&0xFFFFFFFF,self.f[d])
            elif op == 31:
                xo = (word>>1)&1023
                if xo == 444: self.g[a] = self.g[d] | self.g[b]
                elif xo == 266: self.g[d] = (self.g[a]+self.g[b])&0xFFFFFFFF
                elif xo == 215: self.write((self.g[a]+self.g[b])&0xFFFFFFFF,self.g[d],1)
                elif xo == 339: self.g[d] = self.lr
                elif xo == 467: self.lr = self.g[d]
                else: raise AssertionError((hex(pc),hex(word),'op31'))
            elif op == 59:
                xo = (word>>1)&31
                if xo==25: self.f[d] = f32(self.f[a]*self.f[c])
                elif xo==21: self.f[d] = f32(self.f[a]+self.f[b])
                else: raise AssertionError((hex(pc),hex(word),'op59'))
            elif op == 63:
                xo = (word>>1)&1023
                if xo==0:
                    field = (word>>23)&7
                    x,y = self.f[a],self.f[b]
                    self.cr[field*4:field*4+4] = [x<y,x>y,x==y,math.isnan(x) or math.isnan(y)]
                elif xo==72: self.f[d] = self.f[b]
                else: raise AssertionError((hex(pc),hex(word),'op63'))
            elif op == 16:
                assert d in (4,12)
                taken = self.cr[a] == (d==12)
                self.branches.add((hex(pc),taken))
                if taken: next_pc = pc+signed(word&0xFFFC,16)
            elif op == 18:
                target = pc+signed(word&0x3FFFFFC,26)
                if word&1:
                    self.lr = next_pc
                    self.call(target)
                else: next_pc = target
            else: raise AssertionError((hex(pc),hex(word),op))
            pc = next_pc
        raise AssertionError('Instruction budget exceeded')


def assert_vector(actual, expected):
    assert len(actual)==len(expected)
    for a,b in zip(actual,expected):
        assert abs(a-b) <= 1e-6*max(1,abs(b)), (actual,expected)


def verify(dol, read_dol):
    cases = []
    def machine(): return Machine(dol,read_dol)
    def finish(label,m,entry,expected_matrix,expected_current,expected_current_s,expected_parent_s,flag):
        m.execute(entry)
        assert_vector(m.values(MATRIX,12),expected_matrix)
        assert_vector(m.values(CURRENT,12),expected_current)
        assert_vector(m.values(CURRENT_S),expected_current_s)
        assert_vector(m.values(PARENT_S),expected_parent_s)
        assert m.read(FLAGS+JOINT_INDEX,1)==flag
        cases.append({'case':label,'matrix':m.values(MATRIX,12),'current_matrix':m.values(CURRENT,12),
                      'current_scale':m.values(CURRENT_S),'parent_scale':m.values(PARENT_S),
                      'scale_flag':flag,'helper_calls':m.calls,'branch_outcomes':sorted(m.branches)})

    m=machine()
    m.vector(SCALE,(2,3,4));m.vector(TRANSLATION,(5,7,11))
    m.vector(CURRENT,[1,0,0,100,0,1,0,200,0,0,1,300])
    result=[2,0,0,105,0,3,0,207,0,0,4,311]
    finish('Maya unextended scale and translated parent',m,MAYA,result,result,(1,1,1),(2,3,4),0)

    m=machine();m.enable_transform()
    m.vector(SCALE,(2,3,4));m.vector(TRANSLATION,(5,7,11))
    m.vector(TRANSFORM+8,(5,6,7));m.vector(TRANSFORM+0x14,(2,1,0.5))
    m.write(TRANSFORM+4,0,2);m.vector(TRANSFORMS+8,(2,1,4))
    m.write(JOINT+0x17,1,1);m.vector(PARENT_S,(8,2,1))
    m.vector(TRANSFORM+0x2C,(1,2,3));m.vector(TRANSFORM+0x38,(10,20,30));m.vector(TRANSFORM+0x20,(100,200,300))
    diag=[f32(f32(20*(R/2))*(R/8)),f32(18*(R/2)),f32(14*(R/4))]
    current=[diag[0],0,0,16,0,diag[1],0,29,0,0,diag[2],44]
    result=current.copy();result[3]=116;result[7]=229;result[11]=344
    finish('Maya both compensation sources and three translation stages',m,MAYA,result,current,(1,1,1),(2,3,4),0)

    # Rz * Rx * Ry, then inverse(Rz), yields Rx*Ry. A final left Ry
    # exchanges X/Y and reverses Z, preserving translation and discarding _6C's.
    rz=[0,-1,0,0,1,0,0,0,0,0,1,0]
    rx=[1,0,0,0,0,0,-1,0,0,1,0,0]
    ry=[0,0,1,0,0,1,0,0,-1,0,0,0]
    m=machine();m.enable_transform();m.vector(MATRIX,rz)
    for offset,addr,value in [(0x64,0x19000,rx),(0x68,0x19100,ry),(0x6C,0x19200,[0,0,1,999,0,1,0,888,-1,0,0,777])]:
        m.write(TRANSFORM+offset,addr);m.vector(addr,value)
    m.write(TRANSFORM+4,0,2);m.write(TRANSFORMS+0x68,0x19300);m.vector(0x19300,rz)
    m.vector(TRANSLATION,(2,3,5))
    result=[0,1,0,2,1,0,0,3,0,0,-1,5]
    finish('Maya noncommuting local, parent-inverse and global orientation matrices',m,MAYA,result,result,(1,1,1),(1,1,1),1)

    m=machine();m.enable_transform();m.vector(TRANSFORM+8,(1,1,1));m.write(JOINT+0x17,2,1)
    m.vector(PARENT_S,(2,4,8));m.vector(CURRENT_S,(7,11,13))
    finish('Maya compensate byte two is disabled; unit scale bypasses reciprocal',m,MAYA,IDENTITY,IDENTITY,(7,11,13),(1,1,1),1)

    m=machine();m.vector(SCALE,(2,3,4));m.vector(TRANSLATION,(5,7,11));m.vector(CURRENT_S,(2,4,8))
    result=[8,0,0,10,0,36,0,28,0,0,128,88]
    current=[2,0,0,10,0,3,0,28,0,0,4,88]
    finish('SI scales translation with prior current scale then scales output with updated scale',m,SI,result,current,(4,12,32),(1,1,1),0)

    m=machine();m.enable_transform();m.vector(SCALE,(2,2,2));m.vector(CURRENT_S,(0.5,0.5,0.5))
    m.vector(TRANSFORM+8,(3,4,5));m.vector(TRANSFORM+0x14,(2,2,2));m.vector(TRANSLATION,(10,20,30))
    m.vector(TRANSFORM+0x2C,(1,2,3));m.vector(TRANSFORM+0x20,(100,200,300));m.vector(TRANSFORM+0x38,(999,999,999))
    current=[12,0,0,6,0,16,0,12,0,0,20,18]
    result=current.copy();result[3]=106;result[7]=212;result[11]=318
    finish('SI final unit cumulative scale overrides preliminary nonunit flag; _38 ignored',m,SI,result,current,(1,1,1),(1,1,1),1)

    m=machine();m.enable_transform();m.write(TRANSFORM+4,0,2);m.vector(TRANSFORMS+8,(-2,4,8))
    m.write(JOINT+0x17,1,1);m.vector(PARENT_S,(2,4,-8))
    diag=[f32(f32(R/-2)*(R/2)),f32(f32(R/4)*(R/4)),f32(f32(R/8)*(R/-8))]
    result=[diag[0],0,0,0,0,diag[1],0,0,0,0,diag[2],0]
    finish('SI signed parent and animation scale compensation preserve untouched parent scale',m,SI,result,result,(1,1,1),(2,4,-8),1)

    m=machine();m.enable_transform()
    # Special must select current (_28), not frozen (_0), and always use Maya.
    m.vector(INFO+100,(9,9,9));m.vector(INFO+100+0x28,(2,3,4));m.vector(INFO+100+0x34,(5,7,11))
    m.vector(INFO+100+0x40,(0,0,1,0));m.vector(CURRENT_S,(9,8,7))
    result=[-2,0,0,5,0,-3,0,7,0,0,4,11]
    finish('Special reads current cached pose, quaternion and Maya scale path',m,SPECIAL,result,result,(9,8,7),(2,3,4),0)
    return cases
