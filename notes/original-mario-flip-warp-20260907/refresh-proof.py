#!/usr/bin/env python3
"""Fresh isolated Wii proofs for three original Mario state owners."""
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
DECOMP = ROOT / 'decomp'
COMMAND = json.loads((ROOT / 'notes/mario-actor-movement-restoration-20260907/wii-compile-command.json').read_text())['command']
api = runpy.run_path(str(ROOT / 'notes/mario-update-restoration-20260903/verify-object.py'))
Elf = api['Elf']
DOL = (DECOMP / 'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(DOL).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
records = []
results = []
for stem, short in [('MarioFaint', 'Faint'), ('MarioFlip', 'Flip'), ('MarioWarp', 'Warp')]:
    original = DECOMP / 'build/original-player-state-recovery-20260907/retail/obj/Game/Player' / (stem + '.o')
    comparisons = {}
    for kind, source in [('baseline', HERE / 'baseline' / (stem + '.cpp')), ('restored', DECOMP / 'src/Game/Player' / (stem + '.cpp'))]:
        output = HERE / (short + '-' + kind + '.o')
        command = COMMAND.copy()
        command[command.index('-c') + 1] = str(source)
        command[command.index('-o') + 1] = str(output)
        result = subprocess.run(command, cwd=DECOMP, capture_output=True, text=True)
        (HERE / (short + '-' + kind + '.log')).write_text(result.stdout + result.stderr)
        records.append(dict(command=command, exit_code=result.returncode))
        assert result.returncode == 0
        diff = HERE / (short + '-' + kind + '.objdiff.json')
        command = ['build/tools/objdiff-cli', 'diff', '-1', str(original), '-2', str(output), '-o', str(diff), '--format', 'json-pretty']
        result = subprocess.run(command, cwd=DECOMP, capture_output=True, text=True)
        records.append(dict(command=command, exit_code=result.returncode))
        assert result.returncode == 0
        comparisons[kind] = json.loads(diff.read_text())
    before = {s['name']: s.get('match_percent') for s in comparisons['baseline']['left']['symbols']}
    current = {s['name']: s for s in comparisons['restored']['right']['symbols']}
    elf_a, elf_b = Elf(original), Elf(HERE / (short + '-restored.o'))
    methods = []
    for symbol in comparisons['restored']['left']['symbols']:
        name = symbol['name']
        if symbol.get('match_percent') is None or (not symbol.get('instructions') and not name.startswith('__vt__')):
            continue
        entry = dict(symbol=name, retail_bytes=int(symbol['size']), compiled_bytes=int(current[name]['size']), baseline_match=before.get(name), restored_match=symbol['match_percent'])
        if symbol.get('instructions'):
            calls = lambda elf: [ref['symbol'] for ref in elf.references(name) if ref['kind'] == 10]
            a, b = calls(elf_a), calls(elf_b)
            entry.update(direct_calls_equal=a == b, direct_call_count=len(a), retail_direct_calls=a, compiled_direct_calls=b)
        methods.append(entry)
    results.append(dict(source='src/Game/Player/' + stem + '.cpp', reference_sha256=hashlib.sha256((DECOMP / 'src/Game/Player' / (stem + '.cpp')).read_bytes()).hexdigest(), native_byte_identical=(ROOT / 'src/Game/Player' / (stem + '.cpp')).read_bytes() == (DECOMP / 'src/Game/Player' / (stem + '.cpp')).read_bytes(), methods=methods))
(HERE / 'validation-commands.json').write_text(json.dumps(records, indent=2) + '\n')
(HERE / 'function-proof.json').write_text(json.dumps(dict(dol_sha1=hashlib.sha1(DOL).hexdigest(), sources=results), indent=2) + '\n')
for result in results:
    print(result['source'])
    for method in result['methods']:
        if method['baseline_match'] != method['restored_match']:
            print(method['symbol'], method['baseline_match'], '->', method['restored_match'])
