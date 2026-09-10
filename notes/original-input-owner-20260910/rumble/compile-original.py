import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
notes = Path(__file__).resolve().parent
phase = sys.argv[1]
source = root / 'decomp/src/Game/System/WPadRumble.cpp'
command = json.loads((root / 'notes/original-sequence-start-20260910/stick/baseline-compile.json').read_text())['command']
command = command[:command.index('-c')] + ['-c', str(source), '-o', str(notes / (phase + '.o'))]
(notes / (phase + '.cpp')).write_bytes(source.read_bytes())
result = subprocess.run(command, cwd=root / 'decomp', capture_output=True, text=True)
(notes / (phase + '-compile.log')).write_text(result.stdout + result.stderr)
evidence = dict(command=command, cwd=str(root / 'decomp'), exit=result.returncode,
                source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
(notes / (phase + '-compile.json')).write_text(json.dumps(evidence, indent=2) + '\n')
print('Wii compile', phase, result.returncode)
if result.returncode:
    print(result.stdout + result.stderr)
    sys.exit(1)
retail = root / 'notes/gateway-audit-20260907/restoration/retail/obj/Game/System/WPadRumble.o'
command = [str(root / 'decomp/build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2',
           str(notes / (phase + '.o')), '-o', str(notes / (phase + '-diff.json')), '--format', 'json-pretty']
result = subprocess.run(command, capture_output=True, text=True)
(notes / (phase + '-diff.log')).write_text(result.stdout + result.stderr)
evidence = dict(command=command, exit=result.returncode)
if not result.returncode:
    data = json.loads((notes / (phase + '-diff.json')).read_text())
    evidence['keys'] = list(data)
(notes / (phase + '-proof.json')).write_text(json.dumps(evidence, indent=2) + '\n')
print('objdiff', phase, result.returncode)
