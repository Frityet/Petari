#!/usr/bin/env python3
"""Compile actual root camera stage/rail APIs and resolve every retail instruction."""
import hashlib, importlib.util, json, re, struct, subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-camera-stage-rails-20260903'
BUILD.mkdir(parents=True, exist_ok=True)

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT/path)
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result

compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
SOURCES = {'StageDataHolder':'src/Game/Scene/StageDataHolder.cpp', 'SceneUtil':'src/Game/Util/SceneUtil.cpp'}
FUNCTIONS = (
    ('StageDataHolder','getCommonPathPointInfo__15StageDataHolderCFPPC8JMapInfoi',0x803473C8,0x74),
    ('StageDataHolder','getCommonPathPointInfoFromRailDataIndex__15StageDataHolderCFPPC8JMapInfoi',0x8034743C,0x8C),
    ('StageDataHolder','getCommonPathInfoElementNum__15StageDataHolderCFv',0x803474C8,0x44),
    ('SceneUtil','getRailInfoFromRailId__23@unnamed@SceneUtil_cpp@FP12JMapInfoIterPPC8JMapInfoPC15StageDataHolderi',0x803F70D4,0x44),
    ('SceneUtil','getPlacedRailNum__2MRFl',0x803F7AF0,0x54),
    ('SceneUtil','getCameraRailInfo__2MRFP12JMapInfoIterPPC8JMapInfoll',0x803F7B44,0x5C),
    ('SceneUtil','getCameraRailInfoFromRailDataIndex__2MRFP12JMapInfoIterPPC8JMapInfoil',0x803F7BA0,0x74),
    ('SceneUtil','getCurrentScenarioStartAnimCameraData__2MRFPPvPl',0x803F7CA0,0x98),
)
STRINGS = {'CommonPathInfo':0x805D2603, 'l_id':0x805D2612, 'CommonPathPointInfo.%d':0x805D2617,
           'Camera':0x805E1FC4, 'StartScenario%d.canm':0x805E1FDC}
SDA13 = 0x806B9620
DOL = (ROOT/'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(DOL).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
# __init_registers: lis r13,0x806b; ori r13,r13,0x9620.
assert reader.dol_bytes(DOL,0x8000422C,8) == bytes.fromhex('3da0806b61ad9620')
for string,address in STRINGS.items():
    assert reader.dol_bytes(DOL,address,len(string)+1) == string.encode()+b'\0'
addresses = {name:int(value,16) for name,value in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);', (ROOT/'config/RMGK01/symbols.txt').read_text(), re.M)}
commands = []; units = {}; source_hashes = {}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def run(command, log):
    commands.append(command)
    with (BUILD/log).open('w') as output:
        subprocess.run(command,cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True)

for unit,path in SOURCES.items():
    source_hashes[path] = sha(ROOT/path)
    obj = BUILD/(unit+'.o')
    run(compiler.compiler('cflags_game')+['-c',path,'-o',str(obj)],unit+'.compile.log')
    assert source_hashes[path] == sha(ROOT/path)
    original = ROOT/'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj'/Path(path).relative_to('src').with_suffix('.o')
    diff_path = BUILD/(unit+'.objdiff.json')
    run(['build/tools/objdiff-cli','diff','-1',str(original),'-2',str(obj),'-o',str(diff_path),'--format','json-pretty'],unit+'.objdiff.log')
    units[unit] = (reader.Elf(obj),json.loads(diff_path.read_text()),sha(obj),sha(original))

layout = {'sizeof(JMapInfoIter)':8, 'offsetof(JMapInfoIter, mInfo)':0, 'offsetof(JMapInfoIter, mIndex)':4,
          'offsetof(StageDataHolder, mPathObjs)':0x34, 'offsetof(StageDataHolder, mZoneID)':0xDC}
probe = '#include "Game/Scene/StageDataHolder.hpp"\n#include <stddef.h>\n'
probe += '\n'.join(f'typedef char Check{i}[({expression} == {size}) ? 1 : -1];' for i,(expression,size) in enumerate(layout.items()))
probe += '\ntypedef JMapInfoIter (StageDataHolder::*RailGetter)(const JMapInfo**, int) const;\n'
probe += 'RailGetter byId = &StageDataHolder::getCommonPathPointInfo;\nRailGetter byIndex = &StageDataHolder::getCommonPathPointInfoFromRailDataIndex;\n'
(BUILD/'layout.cpp').write_text(probe)
run(compiler.compiler('cflags_game')+['-c',str(BUILD/'layout.cpp'),'-o',str(BUILD/'layout.o')],'layout.compile.log')
rows = []
for unit,name,address,size in FUNCTIONS:
    assert addresses[name] == address
    elf,diff,objhash,originalhash = units[unit]
    _,start,actual_size,section = next(s for s in elf.symbols if s[0] == name)
    assert actual_size == size
    code = bytearray(elf.section_data(section)[start:start+size]); refs = []
    for reference in elf.references(name):
        reference = reference.copy(); offset=int(reference['offset'],16); kind=reference['kind']; symbol=reference['symbol']
        assert reference['addend'] == 0
        if symbol.startswith('@'):
            data = bytes.fromhex(reference['value_hex']); string=data[:-1].decode()
            assert data[-1] == 0 and kind in (4,6)
            target = STRINGS[string]
            assert reader.dol_bytes(DOL,target,len(data)) == data
        else:
            # Defined globals/functions are resolved by named retail symbols.
            # ELF .bss has no meaningful initializer bytes; do not interpret
            # the generic reader's value_hex for the GameSystem singleton.
            reference.pop('value_hex',None)
            target = addresses[symbol]
        if kind == 10:
            displacement=target-address-offset
            assert displacement%4 == 0 and -0x2000000 <= displacement < 0x2000000
            word=struct.unpack_from('>I',code,offset)[0]
            struct.pack_into('>I',code,offset,(word&0xFC000003)|(displacement&0x3FFFFFC))
        elif kind in (4,6):
            assert offset%4 == 2
            immediate=target if kind == 4 else (target+0x8000)>>16
            struct.pack_into('>H',code,offset,immediate&0xFFFF)
        elif kind == 109:
            assert symbol == 'sInstance__29SingletonHolder<10GameSystem>' and target == 0x806B5BE8
            displacement=target-SDA13; assert -0x8000 <= displacement < 0x8000
            word=struct.unpack_from('>I',code,offset)[0]
            struct.pack_into('>I',code,offset,(word&0xFFE00000)|(13<<16)|(displacement&0xFFFF))
        else: raise AssertionError((name,reference))
        reference['retail_target']=hex(target); refs.append(reference)
    expected=reader.dol_bytes(DOL,address,size)
    assert code == expected, (name, [(hex(i),code[i:i+4].hex(),expected[i:i+4].hex()) for i in range(0,size,4) if code[i:i+4] != expected[i:i+4]])
    record=next(s for s in diff['left']['symbols'] if s['name'] == name)
    mismatches=[]
    for entry in record.get('instructions',[]):
        if entry.get('diff_kind'):
            # All fuzzy score deductions must be only unnamed string label
            # identity. Exact relocation above checks the complete instruction.
            assert entry['diff_kind'] == 'DIFF_ARG_MISMATCH'
            instruction=entry['instruction']; assert instruction['relocation']['type'] in (4,6)
            mismatches.append(instruction['formatted'])
    rows.append({'name':name,'address':hex(address),'size':size,'objdiff_percent':record['match_percent'],
                 'relocated_bytes_equal_current_retail':True,'instructions_verified':size//4,
                 'compiled_object_sha256':objhash,'retail_split_object_sha256':originalhash,
                 'retail_function_sha256':hashlib.sha256(expected).hexdigest(),
                 'objdiff_differences_string_symbol_names_only':mismatches,'relocations':refs})
    print(f'[PASS] {name}: {record["match_percent"]}% objdiff; all {size//4} relocated instructions equal current DOL')
report={'scope':'Five recovered stage/camera APIs, necessary private rail helper, and corrected iterator return types for two existing StageDataHolder path lookups. Root-only; no native imports or camera activation.',
        'compiler':'GC/3.0a3, real configure.py cflags_game, RMGK01 VERSION=0; no generated source/header overlay',
        'dol_sha1':hashlib.sha1(DOL).hexdigest(),'source_sha256':source_hashes,
        'header_sha256':{p:sha(ROOT/p) for p in ('include/Game/Scene/StageDataHolder.hpp','include/Game/Util/JMapInfo.hpp','include/Game/Util/SceneUtil.hpp')},
        'tool_sha256':{p:sha(ROOT/p) for p in ('build/compilers/GC/3.0a3/mwcceppc.exe','build/tools/sjiswrap.exe','build/tools/objdiff-cli')},
        'original_layout':layout,'commands':commands,'verified_sda13_base':hex(SDA13),
        'verified_retail_strings':{s:hex(a) for s,a in STRINGS.items()},'functions':rows,
        'total_verified_instruction_words':sum(r['instructions_verified'] for r in rows),
        'total_resolved_relocations':sum(len(r['relocations']) for r in rows)}
(HERE/'source-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS all eight methods: {report["total_verified_instruction_words"]} exact relocated instruction words')
