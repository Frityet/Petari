from pathlib import Path
import json, os, subprocess, sys, time
n=Path(__file__).resolve().parent
pid=json.loads((n/'climb-launch.json').read_text())['pid']
game=json.loads((n/'route-launch.json').read_text())['pid']
while True:
 os.kill(game,0)
 try:os.kill(pid,0)
 except ProcessLookupError:break
 time.sleep(.2)
with (n/'navigation.jsonl').open('w') as log:
 child=subprocess.Popen([sys.executable,str(n/'navigate.py')],stdout=log,stderr=subprocess.STDOUT)
 (n/'navigation-launch.json').write_text(json.dumps({'pid':child.pid,'after_climber_pid':pid})+'\n')
 print(child.wait(),flush=True)
