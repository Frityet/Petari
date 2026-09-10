from pathlib import Path
import concurrent.futures,hashlib,json,subprocess
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
prior=json.loads((out/'time-native-proof.json').read_text())['command']
base=prior[1:];base.remove('-I'+str(out/'native-headers'));base=base[:base.index('-c')]
helper=root/'build/.tools/game-execution-charset/9f8b08a6c256ed1e771d13b564d146aa334180070727ffb5717e5c56f101a17a/game-literal-preprocessor'
compiler=prior[0]
def compile_one(name):
 source=root/'src/Game/Demo'/(name+'.cpp');obj=out/(name+'.final-native.o');report=out/(name+'.charset.json')
 cmd=['python3',str(root/'script/game_execution_charset.py'),'--compiler',compiler,'--helper',str(helper),'--game-root',str(root/'src/Game'),'--report',str(report),'--']+base+['-c',str(source),'-o',str(obj)]
 p=subprocess.run(cmd,cwd=root,capture_output=True,text=True);(out/(name+'.final-native.log')).write_text(p.stdout+p.stderr)
 result={'name':name,'command':cmd,'exit_code':p.returncode,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
 if p.returncode==0:
  result['object_sha256']=hashlib.sha256(obj.read_bytes()).hexdigest()
  n=subprocess.run(['/opt/homebrew/opt/llvm/bin/llvm-nm','--undefined-only','--demangle',str(obj)],capture_output=True,text=True)
  result['undefined']=[s.strip().removeprefix('U ').strip() for s in n.stdout.splitlines() if s.strip()]
 return result
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:results=list(pool.map(compile_one,['DemoTimeKeeper','DemoWipeKeeper','DemoSoundKeeper','DemoExecutor']))
(out/'final-native-proof.json').write_text(json.dumps(results,indent=2)+'\n')
for r in results:print(r['name'],r['exit_code'],r.get('source_sha256'))
raise SystemExit(any(r['exit_code'] for r in results))
