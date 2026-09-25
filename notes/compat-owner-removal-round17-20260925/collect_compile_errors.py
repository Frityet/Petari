from pathlib import Path
import concurrent.futures,json,subprocess,time,sys
n=Path(__file__).resolve().parent
root=n.parent.parent
imports=json.loads((n/'missing-game-source-imports.json').read_text())
selected={x['destination'] for x in imports}
selected.update(('src/Game/NameObj/NameObjFactory.cpp','src/Game/Scene/SceneObjHolder.cpp'))
commands=[x for x in json.loads((root/'compile_commands.json').read_text()) if x['file'] in selected]
print('Compile diagnostics for',len(commands),'active imported/factory translation units',flush=True)
started=time.monotonic()
def check(entry):
 args=entry['arguments'];out=[];i=0
 while i<len(args):
  if args[i]=='-o':i+=2;continue
  if args[i]=='-c':i+=1;continue
  out.append(args[i]);i+=1
 out.extend(['-fsyntax-only','-ferror-limit=12','-w'])
 r=subprocess.run(out,cwd=entry['directory'],capture_output=True,text=True)
 return {'file':entry['file'],'exit_code':r.returncode,'diagnostics':r.stderr if r.returncode else ''}
results=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=20) as pool:
 for item in pool.map(check,commands):
  results.append(item)
  if item['exit_code']:print(item['file'],flush=True)
report={'seconds':time.monotonic()-started,'translation_units':len(results),'failed':sum(bool(x['exit_code']) for x in results),'results':results}
(n/('compiler-diagnostics'+(sys.argv[1] if len(sys.argv)>1 else '')+'.json')).write_text(json.dumps(report,indent=2)+'\n')
print('Finished:',report['failed'],'failed in',round(report['seconds'],1),'seconds',flush=True)
