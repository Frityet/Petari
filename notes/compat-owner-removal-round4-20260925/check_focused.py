from pathlib import Path
import subprocess,os,json,time,sys
root=Path(__file__).resolve().parents[2]
notes=Path(__file__).resolve().parent
names=sys.argv[1:] or ['original-player-status-storage','original-save-owner','original-jkr-heap','original-jkr-heap-finalizer','original-jkr-allocation-domain','jkr-exception-ownership','original-j3d-packet','original-j3d-mtx-buffer','original-j3d-geometry-resource','original-j3d-vertex-buffer','original-j3d-texture-mtx','actor-sensor-real-or-absent','actor-runtime-registry','scene-scheduler-heap','game-actor-physics-real-or-absent']
env={k:v for k,v in os.environ.items() if not k.startswith('SMGPC_')}
env['SMGPC_REAL_DISC']=str(root/'Super Mario Wii - Galaxy Adventure (Korea).rvz')
env['AURORA_BACKEND']='metal'
results=[]
for name in names:
 target='smg-pc-'+name+'-tests'
 result={'name':name,'target':target}
 for stage,cmd,timeout in [('build',['xmake','build',target],300),('run',[str(root/'build/macosx/arm64/debug'/target)],150)]:
  log=notes/(target+'-'+stage+'.log')
  try:
   with log.open('w') as out:
    done=subprocess.run(cmd,cwd=root,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=timeout)
   result[stage]=done.returncode
  except subprocess.TimeoutExpired:result[stage]='timeout'
  result[stage+'_log']=log.name
  print(name,stage,result[stage],flush=True)
  if result[stage]!=0:break
 results.append(result)
 (notes/('focused-'+names[0]+'.json')).write_text(json.dumps(results,indent=2)+'\n')
