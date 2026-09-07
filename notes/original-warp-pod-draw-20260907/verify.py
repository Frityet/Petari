#!/usr/bin/env python3
"""Fresh original compiler/native proof for the recovered WarpPod lifecycle."""
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NOTE = Path(__file__).resolve().parent
HELPERS = runpy.run_path(str(ROOT / 'notes/mario-update-restoration-20260903/verify-object.py'))
RETAIL = ROOT / 'decomp/build/original-player-state-recovery-20260907/retail/obj/Game/MapObj/WarpPod.o'
DOL = ROOT / 'decomp/build/compat-math-oracle/main.dol'

def run(command, cwd, log):
    p = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    (NOTE / log).write_text(p.stdout + p.stderr)
    assert p.returncode == 0, (log, p.returncode)
    return p.returncode

def main():
    results = {}
    for kind, cwd in [('wii', ROOT / 'decomp'), ('native', ROOT)]:
        command = json.loads((NOTE / (kind + '-command.json')).read_text())
        results[kind + '_compile_exit'] = run(command, cwd, kind + '-compile.log')
    run(['build/tools/objdiff-cli', 'diff', '-1', str(RETAIL), '-2', str(NOTE / 'WarpPod.wii.o'),
         '-o', str(NOTE / 'WarpPod.objdiff.json'), '--format', 'json-pretty'], ROOT / 'decomp', 'objdiff.log')
    diff = json.loads((NOTE / 'WarpPod.objdiff.json').read_text())
    names = ['initPair__7WarpPodFv', 'initDraw__7WarpPodFv', 'drawCylinder__7WarpPodCFUl']
    results['functions'] = [{k: s.get(k) for k in ['name', 'size', 'match_percent']} for s in diff['left']['symbols'] if s['name'] in names]
    a, b = HELPERS['Elf'](RETAIL), HELPERS['Elf'](NOTE / 'WarpPod.wii.o')
    results['references'] = {n: {'retail': a.references(n), 'candidate': b.references(n)} for n in names}
    draw = names[2]
    assert [r for r in a.references(draw) if r['kind'] == 10] == [r for r in b.references(draw) if r['kind'] == 10]
    scalar = lambda e: [(r['offset'], r['value_hex']) for r in e.references(draw) if 'value_hex' in r and len(r['value_hex']) in (8, 16)]
    assert scalar(a) == scalar(b)
    symbol = next(s for s in b.symbols if s[0] == 'gGlowEffectEnvColor')
    table = b.section_data(symbol[3])[symbol[1]:symbol[1] + symbol[2]]
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    assert table == HELPERS['dol_bytes'](dol, 0x805A5CD8, 32)
    results['draw_call_offsets_and_targets_equal'] = True
    results['draw_relocated_scalar_constants_equal'] = True
    results['glow_color_table_equal'] = table.hex()
    spans = {'initPair': (0x80251384, 348), 'initDraw': (0x80251998, 992), 'drawCylinder': (0x80251D78, 1516),
             'getPairPod_last_r4_write': (0x80250D4C, 4), 'getPairPod_return': (0x80250D84, 24),
             'restgpr29': (0x80518A54, 16), 'JUTTexture_constructor': (0x80181A90, 68)}
    results['retail_spans'] = {name: {'address': hex(address), 'size': size, 'sha256': hashlib.sha256(HELPERS['dol_bytes'](dol, address, size)).hexdigest()} for name, (address, size) in spans.items()}
    results['retail_dol_sha1'] = hashlib.sha1(dol).hexdigest()
    results['runtime_validation'] = 'Not yet run: this proves source compilation and retail instruction/data review, not live WarpPod placement/drawing.'
    (NOTE / 'proof.json').write_text(json.dumps(results, indent=2) + '\n')
    paths = ['decomp/src/Game/MapObj/WarpPod.cpp', 'decomp/include/Game/MapObj/WarpPod.hpp', 'src/Game/MapObj/WarpPod.cpp', 'src/Game/MapObj/WarpPod.hpp']
    (NOTE / 'source-manifest.json').write_text(json.dumps({p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in paths}, indent=2) + '\n')
    print(json.dumps({k: v for k, v in results.items() if k != 'references'}, indent=2))

if __name__ == '__main__':
    main()
