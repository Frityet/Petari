from pathlib import Path
import subprocess,os,json,time,sys,hashlib
root=Path(__file__).resolve().parents[2]
notes=Path(__file__).resolve().parent
names=sys.argv[1:] or ['fixed-step-clock','j3d-frame-ctrl','original-j3d-transform-animation','original-j3d-animation-resource','original-system-config','original-language']
env={k:v for k,v in os.environ.items() if not k.startswith('SMGPC_')}
env['SMGPC_REAL_DISC']=str(root/'Super Mario Wii - Galaxy Adventure (Korea).rvz')
env['AURORA_BACKEND']='metal'
results=[]
for name in names:
 target='smg-pc-'+name+'-tests'
 result={'name':name,'target':target}
 for stage,cmd,timeout in [('build',['xmake','build',target],300),('run',[str(root/'build/macosx/arm64/debug'/target)],150)]:
  log=notes/(target+'-'+stage+'.log')
  binary=root/'build/macosx/arm64/debug'/target
  if stage=='run': result['binary_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest()
  try:
   with log.open('w') as out:
    done=subprocess.run(cmd,cwd=root,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=timeout)
   result[stage]=done.returncode
  except subprocess.TimeoutExpired:result[stage]='timeout'
  result[stage+'_log']=log.name
  if stage=='run':
   result['binary_sha256_after']=hashlib.sha256(binary.read_bytes()).hexdigest()
   result['binary_unchanged']=result['binary_sha256']==result['binary_sha256_after']
  print(name,stage,result[stage],flush=True)
  if result[stage]!=0:break
 results.append(result)
 with (notes/'focused-history.jsonl').open('a') as history:
  history.write(json.dumps(result | {'completed_at':time.time()})+'\n')
 (notes/('focused-'+names[0]+'.json')).write_text(json.dumps(results,indent=2)+'\n')
