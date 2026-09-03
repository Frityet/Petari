#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,difflib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-resource-loader-20260903'
spec=importlib.util.spec_from_file_location('helper',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
B.mkdir(parents=True, exist_ok=True)
(B/'JPAResource-before.cpp').write_bytes(subprocess.check_output(['git','show','164ae349c:src/JSystem/JParticle/JPAResource.cpp'],cwd=R))
commands=[]
for tag,src in [('before',B/'JPAResource-before.cpp'),('after',R/'src/JSystem/JParticle/JPAResource.cpp')]:
 cmd=h.compiler('cflags_game')+['-c',str(src),'-o',str(B/f'JPAResource-{tag}.o')];commands.append(cmd)
 p=subprocess.run(cmd,cwd=R,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(B/f'JPAResource-{tag}.log').write_text(p.stdout);p.check_returncode()
cmd=[str(R/'build/tools/objdiff-cli'),'diff','-1',str(B/'JPAResource-before.o'),'-2',str(B/'JPAResource-after.o'),'-o',str(B/'allocation-equivalence.json'),'--format','json-pretty'];subprocess.run(cmd,check=True,capture_output=True)
d=json.loads((B/'allocation-equivalence.json').read_text());methods=[{'name':s['name'],'size':s['size'],'match_percent':s.get('match_percent')} for s in d['left']['symbols'] if s.get('match_percent') is not None]
assert all(s['match_percent']==100 for s in methods),methods
(N/'root-evidence.json').write_text(json.dumps({'commands':commands,'methods':methods},indent=2)+'\n')
old=(B/'JPAResource-before.cpp').read_text();new=(R/'src/JSystem/JParticle/JPAResource.cpp').read_text()
(N/'root-pointer-width.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/src/JSystem/JParticle/JPAResource.cpp','b/src/JSystem/JParticle/JPAResource.cpp')))
print('Original compiler: all',len(methods),'compared symbols unchanged; seven pointer-list allocations corrected')
