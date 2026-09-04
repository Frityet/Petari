#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-clipping-20260903';S=B/'staged'
for source,rel in [('src/Game/Map/PlanetMap.cpp','Game/Map/PlanetMap.cpp'),('include/Game/Map/PlanetMap.hpp','Game/Map/PlanetMap.hpp')]:
 dest=S/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes((R/source).read_bytes())
base=json.loads((R/'pc-port/notes/original-planet-map-data-20260903/native-build.json').read_text())['compiles'][0]['command'];base=base[:base.index('-c')];base[1]='-I'+str(S)
source=S/'Game/Map/PlanetMap.cpp';obj=B/'PlanetMap-native.o';cmd=base+['-c',str(source),'-o',str(obj)];r=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(N/'native-compile.log').write_text(r.stdout+r.stderr);(N/'native-compile.json').write_text(json.dumps(dict(command=cmd,exit_code=r.returncode,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),root_header_fallback=False),indent=2)+'\n');print(r.returncode,(r.stdout+r.stderr)[-2600:])
if r.returncode:raise SystemExit(r.returncode)
nm='/opt/homebrew/opt/llvm/bin/llvm-nm';raw=subprocess.run([nm,'--undefined-only','--just-symbol-name',str(obj)],check=True,capture_output=True,text=True).stdout.splitlines();demangled=subprocess.run([nm,'--undefined-only','--demangle','--just-symbol-name',str(obj)],check=True,capture_output=True,text=True).stdout.splitlines();assert len(raw)==len(demangled)
libs=[R/'pc-port/build/macosx/arm64/debug'/n for n in ['libsmg-pc-game.a','libsmg-pc-common.a','libsmg-pc-render.a']];defined=set()
for lib in libs:defined.update(subprocess.run([nm,'--defined-only','--just-symbol-name',str(lib)],check=True,capture_output=True,text=True).stdout.splitlines())
rows=[dict(symbol=a,demangled=b,present_in_current_native_archives=a in defined) for a,b in zip(raw,demangled)];(N/'native-dependencies.json').write_text(json.dumps(dict(libraries=[dict(path=str(p.relative_to(R)),size=p.stat().st_size,mtime_ns=p.stat().st_mtime_ns) for p in libs],dependencies=rows),indent=2)+'\n');print(len(rows),'direct undefined symbols')
