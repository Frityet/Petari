from pathlib import Path
import concurrent.futures,hashlib,json,subprocess
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
prior=json.loads((out/'native-probe.json').read_text())
def run(item):
 n=item['name'];source=root/'src/Game/Demo'/(n+'.cpp');obj=out/(n+'.final-native.o')
 previous=item['command'];split=previous.index('--');flags=previous[split+1:];flags=[s for s in flags if s!='-I'+str(out/'native-staged')];flags[flags.index(str(out/'native-staged/Game/Demo'/(n+'.cpp')))]=str(source);flags[-1]=str(obj)
 cmd=previous[:previous.index('--game-root')]+['--game-root',str(root/'src/Game'),'--report',str(out/(n+'.charset.json')),'--']+flags
 p=subprocess.run(cmd,cwd=root,capture_output=True,text=True);(out/(n+'.final-native.log')).write_text(p.stdout+p.stderr)
 j={'name':n,'command':cmd,'exit_code':p.returncode,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
 if p.returncode==0:
  j['object_sha256']=hashlib.sha256(obj.read_bytes()).hexdigest()
  nm=subprocess.run(['/opt/homebrew/opt/llvm/bin/llvm-nm','--undefined-only','--demangle',str(obj)],capture_output=True,text=True)
  j['undefined']=[s.strip().removeprefix('U ').strip() for s in nm.stdout.splitlines() if s.strip()]
 else:j['errors']=[s for s in p.stderr.splitlines() if 'error:' in s]
 return j
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:rs=list(pool.map(run,prior))
(out/'final-native-proof.json').write_text(json.dumps(rs,indent=2)+'\n')
for j in rs:print(j['name'],j['exit_code'],j.get('errors',''))
raise SystemExit(any(j['exit_code'] for j in rs))
