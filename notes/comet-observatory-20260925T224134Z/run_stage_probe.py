import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('label')
parser.add_argument('--stage', default='AstroGalaxy')
parser.add_argument('--frames', type=int, default=300)
args = parser.parse_args()
note = Path(__file__).resolve().parent
root = note.parents[1]
binary = note / 'stage-probe'
shutil.copy2(root / 'build/macosx/arm64/debug/smg-pc.app/Contents/MacOS/smg-pc', binary)
env = {k: v for k, v in os.environ.items() if not k.startswith('SMGPC_')}
env.update(SMGPC_SAVE_DIR=str(note / (args.label + '-nand')),
           SMGPC_DEBUG_ACTOR_TRACE_PATH=str(note / (args.label + '-trace.jsonl')),
           SMGPC_DEBUG_ACTOR_TRACE_INTERVAL='30',
           SMGPC_DEBUG_ACTOR_TRACE_TYPES='MarioActor,AstroCore,AstroMapObj,GrandStarReturnDemoStarter,Rosetta,AstroDome,StarPiece',
           SDL_WINDOW_ACTIVATE_WHEN_SHOWN='0', SDL_WINDOW_ACTIVATE_WHEN_RAISED='0')
command = ['lldb', '--batch', '-o', 'run', '-k', 'bt 30', '--', str(binary),
           '--disc', str(root / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'),
           '--stage', args.stage, '--scenario', '1', '--max-frames', str(args.frames)]
log = note / (args.label + '-lldb.log')
started = time.monotonic()
with log.open('x') as output:
    result = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT, timeout=max(60, args.frames / 60 + 30))
contents = log.read_text()
exited = re.search(r'Process \d+ exited with status = (\d+)', contents)
report = dict(command=command, debugger_exit=result.returncode,
              game_exit=int(exited[1]) if exited else None,
              stopped_before_exit=exited is None,
              seconds=time.monotonic() - started, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
(note / (args.label + '-result.json')).write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
print(contents[-6000:])
