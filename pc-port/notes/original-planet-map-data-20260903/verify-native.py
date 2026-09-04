#!/usr/bin/env python3
"""Build and run the real-disc original planet data fixture outside xmake."""
from pathlib import Path
import argparse,json,subprocess,hashlib,os
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-data-20260903'
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args();kind='asan' if args.sanitize else 'native';assert os.environ.get('SMGPC_REAL_DISC'),'SMGPC_REAL_DISC is required'
 records=json.loads((B/'native-build.json').read_text());records.append(json.loads((B/'fixture-build.json').read_text()));objects=[];evidence=[]
 for record in records:
  cmd=record['command'].copy()
  if args.sanitize:cmd[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
  cmd[-1]=str(B/(Path(cmd[-1]).stem+'.'+kind+'.o'));source=Path(cmd[cmd.index('-c')+1]);out=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);assert not out.returncode,out.stderr
  evidence.append(dict(source=str(source.relative_to(R)),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),command=cmd,output=out.stdout+out.stderr));objects.append(cmd[-1])
 link=json.loads((B/'link-command.json').read_text());link=[x for x in link if not x.endswith('.o') or x.endswith('/aurora/lib/compat.cpp.o')];link[1:1]=objects
 if args.sanitize:link.insert(1,'-fsanitize=address,undefined')
 link[-1]=str(B/('planet-map-data-tests-'+kind));out=subprocess.run(link,cwd=R/'pc-port',capture_output=True,text=True);assert not out.returncode,out.stderr
 (N/(kind+'-build.json')).write_text(json.dumps(dict(compiles=evidence,link_command=link,link_output=out.stdout+out.stderr),indent=2)+'\n')
 out=subprocess.run([link[-1]],cwd=R/'pc-port',capture_output=True,text=True);(N/(kind+'-runtime.log')).write_text(out.stdout+out.stderr);print(out.stdout,out.stderr);assert not out.returncode
 (N/(kind+'-runtime-evidence.json')).write_text(json.dumps(dict(binary_sha256=hashlib.sha256(Path(link[-1]).read_bytes()).hexdigest(),exit_code=out.returncode,sanitizers='address,undefined' if args.sanitize else None,expected_allocation_failure_diagnostic='allocFromHead: cannot alloc memory (0x818 byte).'),indent=2)+'\n')
if __name__=='__main__':main()
