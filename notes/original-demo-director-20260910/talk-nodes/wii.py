from pathlib import Path
import json,subprocess,sys,hashlib
root=Path.cwd();out=root/'notes/original-demo-director-20260910/talk-nodes';source=sys.argv[1];label=sys.argv[2]
a=json.loads((root/'notes/original-talk-demo-owners-20260910/RailUtil-wii-proof.json').read_text())['command'];a[a.index('src/Game/Util/RailUtil.cpp')]='src/'+source;a[-1]=str(out/(label+'.o'))
r=subprocess.run(a,cwd=root/'decomp',capture_output=True,text=True);(out/(label+'.wii.log')).write_text(r.stdout+r.stderr);row={'command':a,'exit':r.returncode,'source':source,'sha256':hashlib.sha256((root/'decomp/src'/source).read_bytes()).hexdigest()}
if r.returncode==0:
 d=[str(root/'decomp/build/tools/objdiff-cli'),'diff','-1',str(root/'notes/gateway-audit-20260907/restoration/retail/obj'/Path(source).with_suffix('.o')),'-2',a[-1],'-o',str(out/(label+'.objdiff.json'))]
 q=subprocess.run(d,capture_output=True,text=True);row.update(objdiff_command=d,objdiff_exit=q.returncode)
 if q.returncode==0:
  data=json.loads((out/(label+'.objdiff.json')).read_text());row['symbols']=[{k:s.get(k) for k in ['name','demangled_name','size','match_percent']} for s in data['left']['symbols'] if s.get('size') and any(n in s['name'] for n in ['Message','TalkNode','RecursiveHelper','skipTag'])]
(out/(label+'.proof.json')).write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(row.get('symbols',[]),indent=2));print(r.stdout+r.stderr);raise SystemExit(r.returncode)
