"""Compile selected canonical units with their real MWCC settings to isolated outputs."""
import argparse,concurrent.futures,hashlib,json,shlex,subprocess,time
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('label');parser.add_argument('paths',nargs='*');args=parser.parse_args()
notes=Path(__file__).resolve().parent;root=notes.parents[1];repo=root/'decomp';out=notes/args.label
out.mkdir(exist_ok=False)
paths=args.paths or [p for p in json.loads((notes/'conflicts.json').read_text()) if p.endswith('.cpp')]
targets=['build/RMGK01/'+str(Path(p).with_suffix('.o')) for p in paths]
r=subprocess.run(['ninja','-t','commands',*targets],cwd=repo,capture_output=True,text=True); (out/'commands.log').write_text(r.stdout+r.stderr);r.check_returncode()
commands={}
for line in r.stdout.splitlines():
 if 'mwcceppc.exe' not in line:continue
 command=shlex.split(line.split(' && ')[0]);source=command[command.index('-c')+1]
 if source not in paths:continue
 command.remove('-MMD')
 command[command.index('-o')+1]=str(out/Path(source).with_suffix('.o'))
 command[command.index('-maxerrors')+1]='10'
 commands[source]=command
assert set(commands)==set(paths),set(paths)-set(commands)
def run(pair):
 source,command=pair; destination=out/Path(source).with_suffix('.o');destination.parent.mkdir(parents=True,exist_ok=True)
 start=time.monotonic();p=subprocess.run(command,cwd=repo,capture_output=True,text=True,timeout=120)
 destination.with_suffix('.log').write_text(p.stdout+p.stderr)
 return {'source':source,'command':command,'exit_code':p.returncode,'elapsed_seconds':time.monotonic()-start,'source_sha256':hashlib.sha256((repo/source).read_bytes()).hexdigest()}
results=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
 for result in pool.map(run,commands.items()):
  results.append(result);print(result['exit_code'],result['source'],flush=True)
  (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS',sum(r['exit_code']==0 for r in results),'TOTAL',len(results),flush=True)
raise SystemExit(any(r['exit_code'] for r in results))
