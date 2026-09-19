#!/usr/bin/env python3
"""Run the bounded original-process probe after the shared GPU slot is released."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

root = Path(__file__).resolve().parents[2]
notes = Path(__file__).resolve().parent
binary = root / 'build/macosx/arm64/debug/smg-pc-original-process-butterfly-tests'
env = os.environ.copy()
env['SMGPC_REAL_DISC'] = str(root / 'Super Mario Wii - Galaxy Adventure (Korea).rvz')
env['AURORA_BACKEND'] = 'metal'
start = time.monotonic()
terminated = False
with (notes / 'original-process-run.log').open('w') as output:
    process = subprocess.Popen([str(binary)], cwd=root, env=env, stdout=output, stderr=subprocess.STDOUT)
    try:
        code = process.wait(timeout=60)
    except subprocess.TimeoutExpired:
        terminated = True
        process.terminate()
        try:
            code = process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            code = process.wait()
result = {
    'command': [str(binary)],
    'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'exit_code': code,
    'timeout_terminated': terminated,
    'elapsed_seconds': time.monotonic() - start,
    'pid': process.pid,
    'log': 'original-process-run.log',
}
(notes / 'original-process-run.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(0 if code == 0 and not terminated else 1)
