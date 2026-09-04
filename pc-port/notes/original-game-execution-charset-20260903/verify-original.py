#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-game-execution-charset-20260903'
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,R/path);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);return mod
compiler=load('compiler','pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader=load('reader','pc-port/notes/mario-update-restoration-20260903/verify-object.py')
source=B/'original-literals.cpp';obj=B/'original-literals-ppc.o'
command=compiler.compiler('cflags_game')+['-c',str(source),'-o',str(obj)]
p=subprocess.run(command,cwd=R,capture_output=True,text=True);(N/'original-compiler.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
elf=reader.Elf(obj);actual=dict(line.split('=',1) for line in (N/'native-fixture.log').read_text().splitlines() if '=' in line)
rows=[]
for name in ['plain','material','trailing','joined','ascii_escape','character']:
 _,start,size,index=next(s for s in elf.symbols if s[0]==name)
 data=elf.section_data(index)[start:start+size]
 assert data.hex()==actual[name],(name,data.hex(),actual[name])
 rows.append({'name':name,'size':size,'original_bytes':data.hex(),'native_bytes':actual[name]})
(N/'original-proof.json').write_text(json.dumps({'command':command,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'comparisons':rows},indent=2)+'\n')
print('Original GC3.0a3 + sjiswrap and native VFS compile have identical data for all six literal/character probes')
