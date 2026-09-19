"""Serialized normal Xmake target build with retained failure logs."""
import hashlib,json,pathlib,subprocess,sys,time
root=pathlib.Path(__file__).resolve().parents[2];here=pathlib.Path(__file__).resolve().parent
label=sys.argv[1];results=[]
for target in sys.argv[2:]:
 log=here/(label+'-'+target+'.log');started=time.monotonic()
 with log.open('w') as out:p=subprocess.run(['xmake','build',target],cwd=root,stdout=out,stderr=subprocess.STDOUT)
 result={'target':target,'exit':p.returncode,'seconds':time.monotonic()-started,'log':str(log.relative_to(root))}
 binary=root/'build/macosx/arm64/debug'/target
 if binary.exists():result['binary_sha256']=hashlib.sha256(binary.read_bytes()).hexdigest()
 results.append(result);(here/(label+'-build-results.json')).write_text(json.dumps(results,indent=2)+'\n')
 print(json.dumps(result),flush=True)
 if p.returncode:sys.exit(p.returncode)
