#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,hashlib,os
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/light-name-text-boundary-20260903';S=B/'staged'
def main():
 base=json.loads((R/'pc-port/notes/original-planet-map-data-20260903/native-build.json').read_text())['compiles'][0]['command'];base=base[:base.index('-c')];base[1]='-I'+str(S/'src');rows=[];objects=[]
 for rel in ['src/render/light/LightData.cpp','tests/LightNameTextBoundaryTests.cpp','tests/AreaObjRealOrAbsentTests.cpp']:
  src=S/rel;obj=B/(src.stem+'.o');cmd=base+['-c',str(src),'-o',str(obj)];r=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);rows.append(dict(source=str(src.relative_to(R)),source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),command=cmd,exit_code=r.returncode,output=r.stdout+r.stderr));print(rel,r.returncode)
  (N/'native-build.json').write_text(json.dumps(rows,indent=2)+'\n');assert r.returncode==0,r.stdout+r.stderr
  if 'AreaObjRealOrAbsent' not in rel:objects.append(str(obj))
 link=json.loads((R/'build/original-planet-map-data-20260903/link-command.json').read_text());link=[x for x in link if not x.endswith('.o') or x.endswith('/aurora/lib/compat.cpp.o')];link[1:1]=objects;link[-1]=str(B/'light-name-text-tests');r=subprocess.run(link,cwd=R/'pc-port',capture_output=True,text=True);(N/'native-link.json').write_text(json.dumps(dict(command=link,exit_code=r.returncode,output=r.stdout+r.stderr),indent=2)+'\n');assert r.returncode==0,r.stdout+r.stderr
 assert os.environ.get('SMGPC_REAL_DISC'),'actual disc fixture required';r=subprocess.run([link[-1]],cwd=R/'pc-port',capture_output=True,text=True);(N/'native-runtime.log').write_text(r.stdout+r.stderr);(N/'native-runtime-evidence.json').write_text(json.dumps(dict(binary_sha256=hashlib.sha256(Path(link[-1]).read_bytes()).hexdigest(),exit_code=r.returncode,actual_disc=True,gpu=False),indent=2)+'\n');print(r.stdout+r.stderr);assert r.returncode==0
if __name__=='__main__':main()
