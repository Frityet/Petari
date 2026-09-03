#!/usr/bin/env python3
"""Compare actual PPC code's symbolic GX stream, expanding original sendPoint."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[3]
DIFF=ROOT/'build/original-direct-draw-20260903/diff.json'

def evaluate(symbol):
    regs={1:0x100000,3:0x200000,4:0x200100,5:0x200200,6:0x200300,7:'color'}
    floats={};memory={};events=[]
    for base,name in [(0x200000,'position'),(0x200100,'height'),(0x200200,'side'),(0x200300,'depth')]:
        for i,axis in enumerate('xyz'): memory[base+i*4]=name+'.'+axis
    def number(text): return int(text,0)
    def register(text): return int(text[1:])
    def address(text):
        match=re.fullmatch(r'(-?0x[0-9a-f]+|\d+)\(r(\d+)\)',text)
        return regs[int(match[2])]+number(match[1])
    def vector(pointer):return tuple(memory[pointer+i*4]for i in range(3))
    def store(pointer,value):
        for i,part in enumerate(value):memory[pointer+i*4]=part
    for entry in symbol['instructions']:
        if 'instruction'not in entry:continue
        text=entry['instruction']['formatted'];operation,*tail=text.split(' ',1)
        args=tail[0].split(', ')if tail else[]
        if operation=='li':regs[register(args[0])]=number(args[1])
        elif operation=='lis':regs[register(args[0])]=(number(args[1])<<16)&0xffffffff
        elif operation=='mr':regs[register(args[0])]=regs[register(args[1])]
        elif operation=='addi':regs[register(args[0])]=regs[register(args[1])]+number(args[2])
        elif operation=='mflr':regs[register(args[0])]='return_address'
        elif operation=='stw':
            at=address(args[1]);value=regs[register(args[0])]
            if at==0xcc008000:events.append(('color',value))
            else:memory[at]=value
        elif operation=='stwu':
            at=address(args[1]);memory[at]=regs[register(args[0])];regs[1]=at
        elif operation=='lwz':regs[register(args[0])]=memory[address(args[1])]
        elif operation=='lfs':floats[register(args[0])]=memory[address(args[1])]
        elif operation in ('mtlr','blr'):pass
        elif operation=='bl':
            name=args[0]
            if name.startswith(('_savegpr','_restgpr')):continue
            if name.startswith(('__ct__Q29JGeometry8TVec3','__as__Q29JGeometry8TVec3')):
                store(regs[3],vector(regs[4]))
            elif name.startswith('__apl__Q29JGeometry8TVec3'):
                store(regs[3],tuple(('+',a,b)for a,b in zip(vector(regs[3]),vector(regs[4]))))
            elif name.startswith('__mi__Q29JGeometry8TVec3'):
                store(regs[3],tuple(('-',a,b)for a,b in zip(vector(regs[4]),vector(regs[5]))))
            elif name=='GXBegin':events.append(('begin',regs[3],regs[4],regs[5]))
            elif name=='GXPosition3f32':events.append(('position',floats[1],floats[2],floats[3]))
            elif name.startswith('sendPoint__6TDDraw'):
                events.append(('position',*vector(regs[3])));events.append(('color',regs[4]))
            else:raise AssertionError(name)
        else:raise AssertionError(text)
    return events

def main():
    diff=json.loads(DIFF.read_text())
    sides=[next(s for s in diff[k]['symbols']if s['name'].startswith('drawFillBox3D__6TDDraw'))for k in ('left','right')]
    streams=[evaluate(s)for s in sides]
    assert streams[0]==streams[1]
    assert sum(e[0]=='begin'for e in streams[0])==6
    assert sum(e[0]=='position'for e in streams[0])==24
    assert sum(e[0]=='color'for e in streams[0])==24
    output=Path(__file__).with_name('box-stream-evidence.json')
    output.write_text(json.dumps({'source':'actual retail and original-compiler PPC instructions','same_symbolic_gx_stream':True,'quads':6,'vertices':24,'colors':24,'retail_stream':streams[0]},indent=2)+'\n')
    print('Both actual PPC instruction streams emit identical six quads / 24 symbolic vertices / 24 colors')
if __name__=='__main__':main()
