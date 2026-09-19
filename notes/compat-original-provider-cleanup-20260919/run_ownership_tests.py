"""Run bounded original owner checks, preserving failures and executable hashes."""
import hashlib,json,os,pathlib,subprocess,time
root=pathlib.Path(__file__).resolve().parents[2];here=pathlib.Path(__file__).resolve().parent
targets=['smg-pc-original-jpa-manager-tests','smg-pc-original-particle-resource-owner-tests','smg-pc-original-auto-effect-metadata-tests','smg-pc-original-effect-ownership-tests','smg-pc-original-image-effect-ownership-tests','smg-pc-original-layout-group-tests','smg-pc-original-scene-execution-owner-tests','smg-pc-layout-real-or-absent-tests','smg-pc-story-sequence-real-or-absent-tests']
env=os.environ.copy();env['SMGPC_REAL_DISC']=str(root/'Super Mario Wii - Galaxy Adventure (Korea).rvz')
results=[]
for target in targets:
 binary=root/'build/macosx/arm64/debug'/target;log=here/(target+'-ownership-run.log');start=time.monotonic()
 result={'target':target,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'log':str(log.relative_to(root)),'timeout_seconds':120}
 with log.open('w') as out:
  try:
   p=subprocess.run([str(binary)],cwd=root,env=env,stdout=out,stderr=subprocess.STDOUT,timeout=120)
   result['exit']=p.returncode
  except subprocess.TimeoutExpired:
   result['exit']=None;result['timed_out']=True
 result['seconds']=time.monotonic()-start;results.append(result)
 (here/'ownership-run-results.json').write_text(json.dumps(results,indent=2)+'\n')
 print(json.dumps(result),flush=True)
