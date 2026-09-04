#!/usr/bin/env python3
from pathlib import Path
import json,subprocess
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-auto-effect-registration-20260903'
subprocess.run(['python3',str(NOTES/'stage.py')],cwd=ROOT,check=True)
for row in json.loads((NOTES/'native-compiles.json').read_text()):
 result=subprocess.run(row['command'],cwd=ROOT/'pc-port',capture_output=True,text=True)
 (BUILD/(row['name']+'-native-compile.log')).write_text(result.stdout+result.stderr)
 result.check_returncode()
rows=json.loads((NOTES/'native-metadata-proof.json').read_text())
for i,row in enumerate(rows):
 result=subprocess.run(row['command'],cwd=ROOT if i==len(rows)-1 else ROOT/'pc-port',capture_output=True,text=True)
 if i==len(rows)-1:(NOTES/'metadata-probe-asan.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr)
 result.check_returncode()
print('Complete native utility/metadata TUs compile; real authored metadata graph passes sanitizers')
