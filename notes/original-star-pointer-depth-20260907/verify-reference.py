from pathlib import Path
import json,subprocess
r=Path(__file__).resolve().parents[2];n=Path(__file__).resolve().parent
base=json.loads((r/'notes/original-camera-holder-activation-20260907/wii-compile-results.json').read_text())[0]
results=[]
for name in ['StarPointerController','StarPointerDirector']:
 cmd=list(base['command']);cmd[cmd.index('-c')+1]=f'src/Game/Screen/{name}.cpp';cmd[cmd.index('-o')+1]=str(n/(name+'.wii.o'))
 p=subprocess.run(cmd,cwd=r/'decomp',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 (n/(name+'.wii.log')).write_text(p.stdout)
 item={'source':f'decomp/src/Game/Screen/{name}.cpp','command':cmd,'exit_code':p.returncode}
 if p.returncode==0:
  cmd2=list(base['objdiff_command'])
  cmd2[cmd2.index('-1')+1]=str(r/f'decomp/build/original-player-state-recovery-20260907/retail/obj/Game/Screen/{name}.o')
  cmd2[cmd2.index('-2')+1]=str(n/(name+'.wii.o'))
  cmd2[cmd2.index('-o')+1]=str(n/(name+'.objdiff.json'))
  q=subprocess.run(cmd2,cwd=r/'decomp',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  item.update(objdiff_command=cmd2,objdiff_exit_code=q.returncode)
  if q.returncode==0:
   obj=json.loads((n/(name+'.objdiff.json')).read_text())
   funcs=obj['left']['sections'] if False else obj
   item['objdiff_keys']=list(obj.keys())
 print(name,p.returncode,item.get('objdiff_exit_code'),flush=True);results.append(item)
(n/'wii-compile-results.json').write_text(json.dumps(results,indent=2)+'\n')
