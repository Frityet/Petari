#!/usr/bin/env python3
"""Verify the typed CollisionShadow SDK calls and original capture override."""
from pathlib import Path
import hashlib, importlib.util, json, subprocess, struct
ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-shadow-native-20260903'
RETAIL = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game'
def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    value = importlib.util.module_from_spec(spec); spec.loader.exec_module(value)
    return value
compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
assert hashlib.sha1(compiler.DOL.read_bytes()).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
BUILD.mkdir(parents=True, exist_ok=True)
def direct_calls(elf, function):
    _, start, size, index = next(s for s in elf.symbols if s[0] == function)
    calls = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != index: continue
        for offset in range(section[4], section[4] + section[5], section[9]):
            at, info, addend = struct.unpack_from('>IIi', elf.data, offset)
            if start <= at < start + size and info & 255 == 10:
                calls.append(elf.symbols[info >> 8][0])
    return calls
records = []
for folder, name, symbol, address, size, minimum in [
    ('Player','MarioShadow','createDL__15CollisionShadowFv',0x80314CCC,0x18C,99.7),
    ('System','Overwrite','captureDolTexture__10JUTTextureFPviiiib9_GXTexFmt',0x803A462C,0x98,100),
]:
    source = ROOT / f'src/Game/{folder}/{name}.cpp'; obj = BUILD / (name+'.proof.o')
    command = compiler.compiler('cflags_game') + ['-c',str(source),'-o',str(obj)]
    result = subprocess.run(command,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    (BUILD/(name+'.proof.log')).write_text(result.stdout); result.check_returncode()
    retail = RETAIL/folder/(name+'.o'); diff = BUILD/(name+'.proof.json')
    subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(obj),'-o',str(diff),'--format','json-pretty'],check=True,cwd=ROOT,stdout=subprocess.PIPE)
    data=json.loads(diff.read_text()); sides=[next(s for s in data[k]['symbols'] if s['name']==symbol)for k in ('left','right')]
    calls=[direct_calls(reader.Elf(p),symbol)for p in (retail,obj)]
    assert calls[0]==calls[1],symbol
    assert sides[0]['match_percent']>=minimum,(symbol,sides[0]['match_percent'])
    records.append(dict(symbol=symbol,address=hex(address),retail_size=size,compiled_size=sides[1]['size'],match_percent=sides[0]['match_percent'],direct_calls=calls[0],source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),retail_bytes_sha256=hashlib.sha256(compiler.dol_bytes(compiler.DOL.read_bytes(),address,size)).hexdigest(),command=command))
    print(symbol,sides[0]['match_percent'])
(NOTES/'compiler-evidence.json').write_text(json.dumps({'dol_sha1':hashlib.sha1(compiler.DOL.read_bytes()).hexdigest(),'functions':records},indent=2)+'\n')
