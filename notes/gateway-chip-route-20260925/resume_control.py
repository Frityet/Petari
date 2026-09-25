"""Resume controller supervision for the same live game; never relaunches the game."""
from pathlib import Path
import json,subprocess,sys,time
p=Path(__file__).resolve().parent
commands=[('operator-resumed',[sys.executable,str(p/'operate_first_catch.py'),str(p/'route-actors.jsonl'),str(p/'input.json'),'--plan',str(p/'three-rabbit-route.json'),'--start','1400','--end','40000','--timeout','600','--manual-input-file',str(p/'manual-input.json')]),('climb-resumed',[sys.executable,str(p/'climb_tower.py')]),('navigation-resumed',[sys.executable,str(p/'navigate.py')])]
for label,command in commands:
 if (p/'route.json').exists():break
 with (p/(label+'.jsonl')).open('w') as out:
  proc=subprocess.Popen(command,stdout=out,stderr=subprocess.STDOUT)
  (p/(label+'-launch.json')).write_text(json.dumps({'pid':proc.pid,'command':command})+'\n')
  try:
   while proc.poll() is None and not (p/'route.json').exists():time.sleep(.5)
  finally:
   if proc.poll() is None:proc.terminate()
   try:code=proc.wait(timeout=5)
   except subprocess.TimeoutExpired:proc.kill();code=proc.wait()
 print(label,code,flush=True)
 if code:break
