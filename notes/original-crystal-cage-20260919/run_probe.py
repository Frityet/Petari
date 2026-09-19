#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('kind', choices=('cage', 'star-piece'))
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
notes = Path(__file__).resolve().parent
suffix = {'cage': 'crystal-cage', 'star-piece': 'star-piece-placement'}[args.kind]
binary = root / 'build/macosx/arm64/debug' / f'smg-pc-original-process-{suffix}-tests'
marker = {'cage': 'PASS original-process CrystalCage:', 'star-piece': 'PASS original-process StarPiece placement:'}[args.kind]
env = os.environ.copy()
env['SMGPC_REAL_DISC'] = str(root / 'Super Mario Wii - Galaxy Adventure (Korea).rvz')
env['AURORA_BACKEND'] = 'metal'
env['SMGPC_DEBUG_FRAME_TIMING'] = '1'
for key in ('SMGPC_SCREENSHOT_PATH', 'SMGPC_SCREENSHOT_FRAME', 'SMGPC_DEBUG_ACTOR_TRACE_PATH',
            'SMGPC_DEBUG_LAYOUT_DUMP_PATH', 'SMGPC_DEBUG_LAYOUT_DUMP_FRAME'):
    env.pop(key, None)
sha = hashlib.sha256(binary.read_bytes()).hexdigest()
started = time.monotonic()
log_path = notes / f'{suffix}-run.log'
with log_path.open('wb') as output:
    proc = subprocess.Popen([str(binary)], cwd=root, env=env, stdout=output, stderr=subprocess.STDOUT)
    timeout = False
    try:
        code = proc.wait(timeout=120)
    except subprocess.TimeoutExpired:
        timeout = True
        proc.terminate()
        try:
            code = proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            code = proc.wait()
try:
    os.kill(proc.pid, 0)
    exists = True
except ProcessLookupError:
    exists = False
log = log_path.read_text()
result = {'command': [str(binary)], 'binary_sha256': sha, 'binary_unchanged': sha == hashlib.sha256(binary.read_bytes()).hexdigest(),
          'pid': proc.pid, 'exit_code': code, 'timeout_terminated': timeout, 'process_still_exists': exists,
          'elapsed_seconds': time.monotonic() - started,
          'original_completed_120_frames': 'Original GameSystem stopped after 120 completed frames' in log,
          'probe_and_retirement_pass': marker in log, 'debugger_attached': False}
(notes / f'{suffix}-run.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(0 if code == 0 and not timeout and not exists and result['binary_unchanged'] and
                 result['original_completed_120_frames'] and result['probe_and_retirement_pass'] else 1)
