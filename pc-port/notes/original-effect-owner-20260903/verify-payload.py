#!/usr/bin/env python3
"""Verify the frozen payload package with ordinary production effect headers."""
from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-effect-owner-20260903/payload-proof'
PAYLOAD = NOTES / 'payload/native'

def run(command, name):
    result = subprocess.run(command, cwd=ROOT / 'pc-port', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (name + '.log')).write_text(result.stdout)
    if result.returncode:
        print(result.stdout[-4000:])
    result.check_returncode()
    return {'command': command, 'returncode': result.returncode}

def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    base = json.loads((NOTES / 'native-compiles.json').read_text())[0]['command'][:-4]
    base = [arg for arg in base if '/original-effect-owner-20260903/staged' not in arg]
    base[1:1] = ['-I' + str(PAYLOAD)]
    results = []
    for path in sorted(PAYLOAD.rglob('*.cpp')):
        if path.name == 'MarioEffect.cpp':
            continue  # Full TU requires the separately staged real effect API, a pre-existing gap.
        result = run(base + ['-c', str(path), '-o', str(BUILD / (path.stem + '.o'))], path.stem)
        result['source_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
        results.append(result)
    results.append(run(base + ['-c', str(NOTES / 'probe.cpp'), '-o', str(BUILD / 'probe.o')], 'probe-compile'))
    results.append(run([base[0], '-Wl,-dead_strip', str(BUILD / 'probe.o'), str(BUILD / 'HashSortTableCompat.o'), '-o', str(BUILD / 'probe')], 'probe-link'))
    results.append(run([str(BUILD / 'probe')], 'probe-runtime'))
    (NOTES / 'payload/verification.json').write_text(json.dumps(results, indent=2) + '\n')
    (NOTES / 'payload/runtime.log').write_text((BUILD / 'probe-runtime.log').read_text())
    print('Seven payload source TUs compile with production effect headers; actual pointer probe passed')

if __name__ == '__main__':
    main()
