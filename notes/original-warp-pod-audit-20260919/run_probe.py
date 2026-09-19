import hashlib
import json
import os
import pathlib
import subprocess
import time

root = pathlib.Path(__file__).resolve().parents[2]
notes = pathlib.Path(__file__).resolve().parent
binary = root / 'build/macosx/arm64/debug/smg-pc-original-process-warp-pod-tests'
environment = os.environ.copy()
environment['SMGPC_REAL_DISC'] = str(root / 'Super Mario Wii - Galaxy Adventure (Korea).rvz')
environment['AURORA_BACKEND'] = 'metal'
environment['SMGPC_DEBUG_FRAME_TIMING'] = '1'
for name in ('SMGPC_SCREENSHOT_PATH', 'SMGPC_SCREENSHOT_FRAME', 'SMGPC_DEBUG_ACTOR_TRACE_PATH',
             'SMGPC_DEBUG_LAYOUT_DUMP_PATH', 'SMGPC_DEBUG_LAYOUT_DUMP_FRAME'):
    environment.pop(name, None)
started = time.monotonic()
with (notes / 'original-process-run.log').open('wb') as output:
    process = subprocess.Popen([str(binary)], cwd=root, env=environment, stdout=output, stderr=subprocess.STDOUT)
    timed_out = False
    try:
        code = process.wait(timeout=120)
    except subprocess.TimeoutExpired:
        timed_out = True
        process.terminate()
        try:
            code = process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            code = process.wait()
try:
    os.kill(process.pid, 0)
    process_exists = True
except ProcessLookupError:
    process_exists = False
log = (notes / 'original-process-run.log').read_text()
result = {
    'command': [str(binary)],
    'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'pid': process.pid, 'exit_code': code, 'timeout_terminated': timed_out,
    'process_still_exists': process_exists, 'elapsed_seconds': time.monotonic() - started,
    'original_completed_360_frames': 'Original GameSystem stopped after 360 completed frames' in log,
    'probe_and_retirement_pass': 'PASS original-process WarpPod:' in log,
    'debugger_attached': False,
}
(notes / 'original-process-run.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
raise SystemExit(0 if code == 0 and not timed_out and not process_exists and
                 result['original_completed_360_frames'] and result['probe_and_retirement_pass'] else 1)
