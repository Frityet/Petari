from pathlib import Path
import json,subprocess
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
nm='/opt/homebrew/opt/llvm/bin/llvm-nm'
def defined(file):
 p=subprocess.run([nm,'--defined-only','--demangle',str(file)],capture_output=True,text=True);assert p.returncode==0,p.stderr
 return {s.split(None,2)[2] for s in p.stdout.splitlines() if len(s.split(None,2))==3 and len(s.split(None,2)[1])==1}
rs=json.loads((out/'final-native-proof.json').read_text());symbols=set();archives=[]
for f in (root/'build/macosx/arm64/debug').glob('libsmg-pc-*.a'):
 a=f.stat();symbols|=defined(f);b=f.stat();assert(a.st_size,a.st_mtime_ns)==(b.st_size,b.st_mtime_ns),'Archive changed during inspection'
 archives.append({'path':str(f.relative_to(root)),'size':a.st_size,'mtime_ns':a.st_mtime_ns})
for j in rs:symbols|=defined(out/(j['name']+'.final-native.o'))
for n in ['DemoTimeKeeper','DemoWipeKeeper','DemoSoundKeeper','DemoExecutor']:symbols|=defined(root/'notes/original-talk-demo-owners-20260910/demo'/(n+'.final-native.o'))
missing={};system={}
for j in rs:
 for s in j['undefined']:
  if s in symbols:continue
  target=missing if s.startswith(('MR::','Demo','typeinfo for Demo')) else system
  target.setdefault(s,[]).append(j['name'])
result={'archives':archives,'native_sources':{j['name']:j['source_sha256'] for j in rs},'missing_game_imports_from_archives_and_15_tu_original_cohort':missing,'external_language_and_host_runtime_imports':system}
(out/'final-link-frontier.json').write_text(json.dumps(result,indent=2)+'\n')
for s in missing:print(s)
