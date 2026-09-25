"""Bounded process supervisor for one fresh-save, ordinary-controller Gateway route."""
from pathlib import Path
import json
import subprocess
import sys
import time

notes = Path(__file__).resolve().parent
runner_command = [sys.executable,str(notes/'run_original.py'),'route','36000','--bundle','--ignore-restoration',
    '--input-file',str(notes/'input.json'),'--trace-types',
    'MarioActor,Rabbit,Tico,Rosetta,EarthenPipe,CrystalCage,HeavensDoorDemoObj,GlobalGravityObj,SpinDriver,Kuribo,KeySwitch,FlipPanel,PowerStar',
    '--trace-interval','10','--screenshot-frame','15000','--timeout','720']
operator_command = [sys.executable,str(notes/'operate_first_catch.py'),str(notes/'route-actors.jsonl'),str(notes/'input.json'),
    '--plan',str(notes/'three-rabbit-route.json'),'--start','1400','--end','22000','--timeout','430']
children = []
start = time.monotonic()
climber = None
with (notes/'route-runner.log').open('w') as run_log, (notes/'operator.jsonl').open('w') as operator_log, (notes/'climb.jsonl').open('w') as climb_log:
    try:
        runner = subprocess.Popen(runner_command,stdout=run_log,stderr=subprocess.STDOUT)
        children.append(runner)
        operator = subprocess.Popen(operator_command,stdout=operator_log,stderr=subprocess.STDOUT)
        children.append(operator)
        (notes/'supervision.json').write_text(json.dumps({'runner_pid':runner.pid,'operator_pid':operator.pid,
            'runner_command':runner_command,'operator_command':operator_command},indent=2)+'\n')
        while runner.poll() is None:
            if operator.poll() == 0 and climber is None and (notes/'stair-route.json').exists():
                climber = subprocess.Popen([sys.executable,str(notes/'climb_tower.py')],stdout=climb_log,stderr=subprocess.STDOUT)
                children.append(climber)
                (notes/'climb-launch.json').write_text(json.dumps({'pid':climber.pid,'started_elapsed_seconds':time.monotonic()-start})+'\n')
            print(json.dumps({'elapsed_seconds':round(time.monotonic()-start),'runner':runner.poll(),'operator':operator.poll(),
                'climber':None if climber is None else {'pid':climber.pid,'exit':climber.poll()}}),flush=True)
            time.sleep(20)
        print(json.dumps({'runner_exit':runner.wait(),'operator_exit':operator.poll(),'climber_exit':None if climber is None else climber.poll()}),flush=True)
    finally:
        for child in reversed(children):
            if child.poll() is None:
                child.terminate()
                try:child.wait(timeout=5)
                except subprocess.TimeoutExpired:child.kill();child.wait()
