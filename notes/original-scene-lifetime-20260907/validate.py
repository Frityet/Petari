from pathlib import Path
import hashlib
import json
import re
import shlex
import subprocess

root = Path(__file__).resolve().parents[2]
note = Path(__file__).resolve().parent
base = json.loads((root / 'notes/original-mario-owner-activation-20260907/Mario-compile.json').read_text())['command']
base = base[:base.index('-o')]
results = []
objects = []
for source in ['src/scene/SceneLifetimeBinding.cpp', 'src/compat/SceneLifetimeCompat.cpp', 'tests/SceneLifetimeBindingTests.cpp']:
    path = root / source
    obj = note / (path.stem + '.o')
    log = note / (path.stem + '-object.log')
    command = base + ['-o', str(obj), str(path)]
    with log.open('w') as stream:
        proc = subprocess.run(command, cwd=root, stdout=stream, stderr=subprocess.STDOUT)
    results.append({'source': source, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'exit': proc.returncode, 'command': command})
    print(source, proc.returncode, flush=True)
    if proc.returncode:
        break
    objects.append(str(obj))
else:
    text = (root / 'notes/original-stage-initialization-20260907/smg-pc-original-player-status-storage-tests-build.log').read_text()
    lines = [re.sub(r'\x1b\[[0-9;]*m', '', line) for line in text.splitlines()]
    command = shlex.split(next(line for line in lines if 'clang++ -o' in line))
    command[2] = str(note / 'scene-lifetime-tests')
    command[3:4] = objects
    with (note / 'link.log').open('w') as stream:
        proc = subprocess.run(command, cwd=root, stdout=stream, stderr=subprocess.STDOUT)
    results.append({'phase': 'link', 'exit': proc.returncode, 'command': command})
    print('link', proc.returncode, flush=True)
    if proc.returncode == 0:
        with (note / 'run.log').open('w') as stream:
            proc = subprocess.run([str(note / 'scene-lifetime-tests')], cwd=root, stdout=stream, stderr=subprocess.STDOUT, timeout=60)
        results.append({'phase': 'run', 'exit': proc.returncode})
        print('run', proc.returncode, (note / 'run.log').read_text(), flush=True)
(note / 'lifetime-test-result.json').write_text(json.dumps(results, indent=2) + '\n')
