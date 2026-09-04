#!/usr/bin/env python3
from pathlib import Path
import json, subprocess
ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-effect-sync-checker-20260903'
subprocess.run(['python3', str(NOTES / 'stage.py')], cwd=ROOT, check=True)
rows = []
def run(command, label):
    result = subprocess.run(command, cwd=ROOT / 'pc-port', capture_output=True, text=True)
    (BUILD / (label + '.log')).write_text(result.stdout + result.stderr)
    rows.append({'command': command, 'exit': result.returncode})
    (NOTES / 'native-probe-proof.json').write_text(json.dumps(rows, indent=2) + '\n')
    if result.returncode:
        print(result.stdout + result.stderr)
    result.check_returncode()
    return result.stdout + result.stderr
for row in json.loads((NOTES / 'native-compiles.json').read_text()):
    run(row['command'], row['name'] + '-native-compile')
base = json.loads((NOTES / 'native-compiles.json').read_text())[1]['command']
for name, source in [('SyncBckEffectInfo', BUILD / 'staged/Game/Effect/SyncBckEffectInfo.cpp'), ('SyncBckEffectChecker', BUILD / 'staged/Game/Effect/SyncBckEffectChecker.cpp'), ('checker-probe', NOTES / 'checker-probe.cpp')]:
    command = base[:]
    command[command.index('-c') + 1] = str(source)
    command[command.index('-o') + 1] = str(BUILD / (name + '-asan.o'))
    command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    run(command, name + '-asan-compile')
base = json.loads((ROOT / 'pc-port/notes/original-auto-effect-registration-20260903/native-metadata-proof.json').read_text())[-2]['command']
command = [arg for arg in base if not arg.endswith('.o')]
command[1:1] = [str(BUILD / 'checker-probe-asan.o'), str(BUILD / 'SyncBckEffectInfo-asan.o'), str(BUILD / 'SyncBckEffectChecker-asan.o')]
command[command.index('-o') + 1] = str(BUILD / 'checker-probe-asan')
run(command, 'link')
output = run([str(BUILD / 'checker-probe-asan')], 'checker-probe-asan')
(NOTES / 'checker-probe-asan.log').write_text(output)
print(output)
print('Complete native emitter/checker/keeper TUs compile; actual player/checker tests pass ASan/UBSan')
