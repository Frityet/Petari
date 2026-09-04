#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,re,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-planet-map-creator-20260903';S=B/'staged'
def put(p,s):
 q=S/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_text(s)
source=(R/'src/Game/Map/PlanetMapCreator.cpp').read_text();types=sorted(set(re.findall(r'createNameObj< (\w+) >',source))-{'PlanetMapFarClippable'})
headers=[]
for t in types:
 if t in ('PlanetMap','FurPlanetMap','RailPlanetMap','PlanetMapAnimLow'):h=R/'include/Game/Map/PlanetMap.hpp'
 elif t=='AstroSimpleObj':h=R/'include/Game/MapObj/AstroMapObj.hpp'
 else:
  hits=list((R/'include/Game').rglob(t+'.hpp'));assert len(hits)==1,(t,hits);h=hits[0]
 headers.append(str(h.relative_to(R/'include')))
headers=sorted(set(headers)|{'Game/Map/PlanetMapCreator.hpp','Game/NameObj/NameObjFactory.hpp','Game/NameObj/NameObjArchiveListCollector.hpp','Game/Scene/SceneObjHolder.hpp','Game/Util/ObjUtil.hpp','Game/Util/ModelUtil.hpp','Game/Util/SceneUtil.hpp','Game/Util/StringUtil.hpp'})
# Only replace umbrella includes; all class bodies, tables and Game algorithms stay literal.
source=''.join('#include "'+h+'"\n' for h in headers)+'#include <cstdio>\n#include <cstring>\n\n'+source[source.index('class PlanetMapFarClippable'):]
put('Game/Map/PlanetMapCreator.cpp',source)
obj=(R/'pc-port/src/Game/Util/ObjUtil.hpp').read_text()
if 'createCsvParser(const char*' not in obj:obj=obj.replace('    JMapInfo* createCsvParser(const ResourceHolder*, const char*, ...);','    JMapInfo* createCsvParser(const ResourceHolder*, const char*, ...);\n    JMapInfo* createCsvParser(const char*, const char*, ...);')
put('Game/Util/ObjUtil.hpp',obj)
for h in ('Game/Map/PlanetMap.hpp','Game/Map/PlanetMapCreator.hpp'):put(h,(R/'include'/h).read_text())
base=json.loads((R/'pc-port/notes/original-planet-map-data-20260903/native-build.json').read_text())['compiles'][0]['command'];base=base[:base.index('-c')];base[1]='-I'+str(S);base+=['-I'+str(R/'include')]
records=[]
for label,src in [('planet',S/'Game/Map/PlanetMapCreator.cpp'),('factory',R/'src/Game/NameObj/NameObjFactory.cpp')]:
 cmd=base+['-ferror-limit=20','-c',str(src),'-o',str(B/(label+'-native.o'))];r=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(N/(label+'-native-compile.log')).write_text(r.stdout+r.stderr);records.append(dict(source=str(src.relative_to(R)),sha256=hashlib.sha256(src.read_bytes()).hexdigest(),command=cmd,exit_code=r.returncode,root_header_fallback=True));print(label,r.returncode,(r.stdout+r.stderr)[-1600:])
(N/'native-compile.json').write_text(json.dumps(records,indent=2)+'\n')
