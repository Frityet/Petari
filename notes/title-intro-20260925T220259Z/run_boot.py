import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('label')
parser.add_argument('--frames', type=int, default=3600)
args = parser.parse_args()
note = Path(__file__).resolve().parent
root = note.parents[1]
binary = root / 'build/macosx/arm64/debug/smg-pc.app/Contents/MacOS/smg-pc'
environment = {k: v for k, v in os.environ.items() if not k.startswith('SMGPC_')}
environment.update(
    SMGPC_SAVE_DIR=str(note / (args.label + '-nand')),
    SMGPC_DEBUG_ACTOR_TRACE_PATH=str(note / (args.label + '-trace.jsonl')),
    SMGPC_DEBUG_ACTOR_TRACE_INTERVAL='30',
    SMGPC_DEBUG_ACTOR_TRACE_TYPES='FileSelector,FileSelectItem,PrologueDirector,MarioActor',
    SMGPC_DEBUG_WPAD_INPUT_FILE=str(note / 'live-input.json'),
)
(note / 'live-input.json').write_text(json.dumps(dict(buttons='', pointer='', stick='')) + '\n')
command = [str(binary), '--disc', str(root / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'),
           '--max-frames', str(args.frames)]
digest = hashlib.sha256(binary.read_bytes()).hexdigest()
start = time.monotonic()
timed_out = False
with (note / (args.label + '.log')).open('xb') as output:
    process = subprocess.Popen(command, env=environment, stdout=output, stderr=subprocess.STDOUT)
    (note / (args.label + '-pid.txt')).write_text(str(process.pid))
    try:
        code = process.wait(timeout=max(60, args.frames / 60 + 40))
    except subprocess.TimeoutExpired:
        timed_out = True
        process.terminate()
        try:
            code = process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            code = process.wait()
result = dict(command=command, exit_code=code, timed_out=timed_out,
              seconds=time.monotonic() - start, binary_sha256=digest)
(note / (args.label + '-result.json')).write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
