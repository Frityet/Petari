#!/usr/bin/env python3
"""Compile root hand/draw recovery with original Game flags and compare retail."""
from pathlib import Path
import hashlib, importlib.util, json, subprocess
ROOT=Path(__file__).resolve().parents[3]
NOTES=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-mario-hand-draw-20260903'
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
compiler=module('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=module('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
BUILD.mkdir(parents=True,exist_ok=True)
dol=compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
rows=[];commands=[]
for unit,names in [('MarioActorHand',['updateHand__10MarioActorFv']),('MarioActorSpecialDraw',['createRainbowDL__10MarioActorFv']),('MarioActorDraw',['addDL__9DLchangerFP9J3DModelX'])]:
    source=ROOT/f'src/Game/Player/{unit}.cpp'; obj=BUILD/(unit+'.o')
    command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(obj)];commands.append(command)
    with (BUILD/(unit+'-compile.log')).open('w') as log:
        subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
    retail=ROOT/f'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Player/{unit}.o'
    output=BUILD/(unit+'-diff.json')
    subprocess.run([str(ROOT/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(obj),'-o',str(output),'--format','json-pretty'],cwd=ROOT,check=True)
    diff=json.loads(output.read_text()); a=reader.Elf(retail); b=reader.Elf(obj)
    for name in names:
        left=next(s for s in diff['left']['symbols'] if s['name']==name)
        right=next(s for s in diff['right']['symbols'] if s['name']==name)
        ar=a.references(name);br=b.references(name)
        calls=lambda refs:[r['symbol'] for r in refs if r['kind']==10]
        assert calls(ar)==calls(br),(name,calls(ar),calls(br))
        constants=lambda refs:{r['value_hex'] for r in refs if 'value_hex' in r}
        assert constants(ar)==constants(br),(name,constants(ar),constants(br))
        assert left['match_percent'] >= 94
        row=dict(symbol=name,retail_bytes=int(left['size']),compiled_bytes=int(right['size']),objdiff_match_percent=left['match_percent'],original_call_order=calls(ar),constant_values=sorted(constants(ar)),retail_references=ar,compiled_references=br,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
        rows.append(row)
        print(name,left['match_percent'],left['size'],right['size'])
(NOTES/'compiler-evidence.json').write_text(json.dumps(dict(dol_sha1=hashlib.sha1(dol).hexdigest(),commands=commands,functions=rows),indent=2)+'\n')
print('PASS: original call order and constants, high fuzzy original-compiler match')
