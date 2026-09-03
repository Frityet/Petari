from pathlib import Path
import json, re, subprocess
r=Path(__file__).resolve().parents[3]; d=r/'build/original-j3d-draw-20260903'; d.mkdir(parents=True,exist_ok=True)
entries=json.loads((r/'pc-port/compile_commands.json').read_text())
e=next(e for e in entries if e['file'].endswith('/OriginalJ3DModelResourceTests.cpp'))
prefix=[]; skip=False
for item in e['arguments']:
    if skip: skip=False; continue
    if item=='-o':skip=True; continue
    if item not in ('-c',e['file']):prefix.append(item)
cmd=prefix+['-fno-color-diagnostics','-c',str(Path(__file__).resolve().with_name('probe.cpp')),'-o',str(d/'probe.o')]
s=subprocess.run(cmd,cwd=e['directory'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
(d/'compile.log').write_text(s.stdout)
print('compile', s.returncode, '\n'.join(l for l in s.stdout.splitlines() if 'error:' in l));
if s.returncode: raise SystemExit(s.returncode)
data=(r/'pc-port/build/.deps/smg-pc-showcase/macosx/arm64/debug/smg-pc-showcase.d').read_text()
flags=re.findall(r'"((?:\\.|[^"\\])*)"',re.search(r'values = \{(.*?)\n    \}',data,re.S)[1])
cmd=[flags[0],str(d/'probe.o'),str(r/'pc-port/build/.objs/smg-pc-showcase/macosx/arm64/debug/aurora/lib/compat.cpp.o'),*flags[1:],'-o',str(d/'probe')]
s=subprocess.run(cmd,cwd=r/'pc-port',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
(d/'link.log').write_text(s.stdout);(d/'link.command.json').write_text(json.dumps(cmd,indent=2))
print('link',s.returncode, '\n'.join(l for l in s.stdout.splitlines() if 'error:' in l or 'undefined' in l.lower()));
if s.returncode: raise SystemExit(s.returncode)
