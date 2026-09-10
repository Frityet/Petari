from pathlib import Path
import hashlib,json,subprocess,sys
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
name=sys.argv[1];label=sys.argv[2]
base=json.loads((root/'notes/original-save-owner-20260910/value-checker/recovered-proof.json').read_text())
cmd=base['command'][:];cmd[cmd.index('src/Game/System/GameEventValueChecker.cpp')]='src/Game/Demo/'+name+'.cpp';obj=out/(name+'.'+label+'.wii.o');cmd[-1]=str(obj)
p=subprocess.run(cmd,cwd=root/'decomp',capture_output=True,text=True);(out/(label+'-wii.log')).write_text(p.stdout+p.stderr);r={'command':cmd,'wii_exit':p.returncode}
if p.returncode==0:
 diff=[str(root/'decomp/build/tools/objdiff-cli'),'diff','-1',str(root/('notes/gateway-audit-20260907/restoration/retail/obj/Game/Demo/'+name+'.o')),'-2',str(obj),'-o',str(out/(label+'-objdiff.json'))]
 d=subprocess.run(diff,capture_output=True,text=True);r.update(objdiff_command=diff,objdiff_exit=d.returncode)
 if d.returncode==0:
  data=json.loads((out/(label+'-objdiff.json')).read_text());r['symbols']=[{k:s.get(k) for k in ['name','demangled_name','size','match_percent','target_symbol']} for s in data['left']['symbols'] if 'Demo' in s['name'] and s.get('size')]
for f in ['decomp/src/Game/Demo/'+name+'.cpp','decomp/include/Game/Demo/'+name+'.hpp','decomp/include/Game/Demo/DemoExecutor.hpp']:r.setdefault('sources',{})[f]=hashlib.sha256((root/f).read_bytes()).hexdigest()
(out/(label+'-proof.json')).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2));print(p.stdout);print(p.stderr);raise SystemExit(p.returncode)
