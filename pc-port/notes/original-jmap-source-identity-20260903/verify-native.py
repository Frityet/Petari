#!/usr/bin/env python3
"""Isolated raw-JMap/index/JKR-heap fixture; does not run a shared build or GPU."""
from pathlib import Path
import argparse,json,subprocess,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jmap-source-identity-20260903';S=B/'staged'
def main():
 args=argparse.ArgumentParser();args.add_argument('--sanitize',action='store_true');options=args.parse_args();kind='asan' if options.sanitize else 'native'
 base=json.loads((B/'native-build.json').read_text())[0]['command'];base=base[:base.index('-c')]
 if options.sanitize:base[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 rows=[];objects=[]
 for rel in ['Game/Util/JMapInfo.cpp','resource/JMapResource.cpp','resource/BcsvTable.cpp','resource/RarcArchive.cpp','compat/OriginalArchiveIndex.cpp','compat/JKRArchiveCompat.cpp','tests/OriginalJMapResourceTests.cpp']:
  p=B/rel if rel.startswith('tests/') else (S/rel if (S/rel).exists() else R/'pc-port/src'/rel);obj=B/(p.stem+'.'+kind+'.o');cmd=base+['-c',str(p),'-o',str(obj)]
  out=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);rows.append(dict(source=str(p.relative_to(R)),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),command=cmd,returncode=out.returncode,output=out.stdout+out.stderr));assert not out.returncode,out.stderr;objects.append(str(obj))
 link=json.loads((R/'build/scenario-catalog-owner-20260903/link-command.json').read_text());link=[x for x in link if not x.endswith('.o') or x.endswith('/aurora/lib/compat.cpp.o')];link[1:1]=objects
 if options.sanitize:link.insert(1,'-fsanitize=address,undefined')
 link[-1]=str(B/('jmap-source-tests-'+kind));out=subprocess.run(link,cwd=R/'pc-port',capture_output=True,text=True);assert not out.returncode,out.stderr
 (N/(kind+'-build.json')).write_text(json.dumps(dict(compiles=rows,link_command=link,link_output=out.stdout+out.stderr),indent=2)+'\n')
 out=subprocess.run([link[-1]],cwd=R/'pc-port',capture_output=True,text=True);(N/(kind+'-runtime.log')).write_text(out.stdout+out.stderr);print(out.stdout,out.stderr);assert not out.returncode
 (N/(kind+'-runtime-evidence.json')).write_text(json.dumps(dict(binary_sha256=hashlib.sha256(Path(link[-1]).read_bytes()).hexdigest(),returncode=out.returncode,pass_groups=out.stdout.count('PASS '),sanitizers='address,undefined' if options.sanitize else None),indent=2)+'\n')
if __name__=='__main__':main()
