#!/usr/bin/env python3
"""Compile staged C++ enum corrections and verify actual SC accessors/product data."""
from pathlib import Path
import hashlib, importlib.util, json, re, struct, subprocess, difflib
NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
BUILD = ROOT / 'build/original-sc-service-20260903'
def module(name, rel):
    s = importlib.util.spec_from_file_location(name, ROOT / rel)
    result = importlib.util.module_from_spec(s); s.loader.exec_module(result); return result
h = module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
r = module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
dol = h.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
symbols = {name: int(address,16) for name,address in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9a-fA-F]+);',(ROOT/'config/RMGK01/symbols.txt').read_text(),re.M)}
commands, records = [], []

def relocate(compiled, target, name, address, size):
    _, start, length, section = next(s for s in compiled.symbols if s[0] == name)
    assert length == size
    code = bytearray(compiled.section_data(section)[start:start+length])
    retail = h.dol_bytes(dol,address,size)
    constants = {}
    refs = target.references(name)
    for ref in refs:
        if 'value_hex' not in ref: continue
        at, kind = int(ref['offset'],16), ref['kind']
        if kind == 109:
            word = struct.unpack_from('>I',retail,at)[0]
            ra = (word >> 16) & 31
            assert ra in (2,13)
            base = 0x806BFC20 if ra == 2 else 0x806B9620
            effective = base + struct.unpack_from('>h',retail,at+2)[0]
        else:
            pair = {v['kind']:int(v['offset'],16) for v in refs if v['symbol']==ref['symbol'] and v['addend']==ref['addend']}
            effective = ((struct.unpack_from('>H',retail,pair[6])[0] << 16) + struct.unpack_from('>h',retail,pair[4])[0]) & 0xFFFFFFFF
        data = bytes.fromhex(ref['value_hex'])
        assert h.dol_bytes(dol,effective,len(data)) == data
        constants[(ref['value_hex'],ref['addend'])] = effective
    resolved = []
    for ref in compiled.references(name):
        at,kind = int(ref['offset'],16),ref['kind']
        effective = constants[(ref['value_hex'],ref['addend'])] if 'value_hex' in ref else symbols[ref['symbol']] + ref['addend']
        if kind in (4,6): struct.pack_into('>H',code,at,(effective if kind==4 else (effective+0x8000)>>16)&65535)
        else:
            word=struct.unpack_from('>I',code,at)[0]
            if kind==10: word=(word&0xFC000003)|((effective-address-at)&0x3FFFFFC)
            elif kind==109:
                ra=(struct.unpack_from('>I',retail,at)[0]>>16)&31
                base=0x806BFC20 if ra==2 else 0x806B9620
                word=(word&~0x1FFFFF)|(ra<<16)|((effective-base)&65535)
            else: raise AssertionError(ref)
            struct.pack_into('>I',code,at,word)
        resolved.append({**ref,'actual_target':hex(effective)})
    assert bytes(code)==retail,name
    return resolved

for unit,source in [('scapi', BUILD/'staged-root/src/RVL_SDK/sc/scapi.c'),('scapi_prdinfo',ROOT/'src/RVL_SDK/sc/scapi_prdinfo.c')]:
    output=BUILD/(unit+'.proof.o');command=h.compiler('cflags_sdk')+['-c',str(source),'-o',str(output)]
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True);(BUILD/(unit+'.proof.log')).write_text(result.stdout+result.stderr);result.check_returncode();commands.append(command)
    compiled=r.Elf(output);target=r.Elf(ROOT/('build/xanime-core-pose-blending-restoration-20260903/retail/obj/RVL_SDK/sc/'+unit+'.o'))
    for name,_,size,_ in target.symbols:
        if not size or name not in symbols or not (name.startswith('SCGet') or name.startswith('SCSet') or name=='__SCF1'):continue
        refs=relocate(compiled,target,name,symbols[name],size)
        records.append({'name':name,'address':hex(symbols[name]),'size':size,'relocated_bytes_exact':True,'references':refs})
    if unit=='scapi_prdinfo':
        for name in ['ProductAreaAndStringTbl','ProductGameRegionAndStringTbl']:
            _,start,size,index=next(s for s in compiled.symbols if s[0]==name)
            data=compiled.section_data(index)[start:start+size]
            assert data==h.dol_bytes(dol,symbols[name],size)
            records.append({'table':name,'address':hex(symbols[name]),'size':size,'full_table_bytes_exact':True})
assert sum('name' in x for x in records)==22
# Native imports are complete literal bodies: only six enum spellings differ from current root C.
assert (BUILD/'staged/compat/OriginalSystemConfigAccessors.cpp').read_bytes()==(BUILD/'staged-root/src/RVL_SDK/sc/scapi.c').read_bytes()
assert (BUILD/'staged/compat/OriginalSystemProductInfo.cpp').read_bytes()==(ROOT/'src/RVL_SDK/sc/scapi_prdinfo.c').read_bytes()
source=Path('src/RVL_SDK/sc/scapi.c');before=(ROOT/source).read_text();after=(BUILD/'staged-root'/source).read_text()
(NOTE/'root-enum.patch').write_text('diff --git a/'+str(source)+' b/'+str(source)+'\n'+''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),'a/'+str(source),'b/'+str(source))))
(NOTE/'original-evidence.json').write_text(json.dumps({'dol_sha1':hashlib.sha1(dol).hexdigest(),'commands':commands,'functions_and_tables':records,'retail_tail_reservation_instruction':{'address':'0x804D0A18','bytes':'387fffba','decoded':'addi r3,r31,-70'}},indent=2)+'\n')
assert h.dol_bytes(dol,0x804D0A18,4).hex()=='387fffba'
print('22 original accessor/product methods:',sum(x['size'] for x in records if 'name' in x),'relocated bytes exact; both full product tables exact')
