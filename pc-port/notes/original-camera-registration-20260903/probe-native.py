#!/usr/bin/env python3
"""Compile the complete original registry graph without importing or activating it."""
from concurrent.futures import ThreadPoolExecutor
import hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
PC=ROOT/'pc-port';HERE=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-camera-registration-20260903/native'
SOURCE=ROOT/'src/Game/Camera'
text=(SOURCE/'CameraHolder.cpp').read_text()
classes=re.findall(r'createCamera< (Camera\w+) >, (?:true|false)',text)
assert len(classes)==45
sources=[]
root_sources={}
for name in classes:
 source=SOURCE/(name+'.cpp');sources.append(source)
 translator=re.search(r'#include "Game/Camera/(CamTranslator\w+)\.hpp"',source.read_text())
 if translator: sources.append(SOURCE/(translator[1]+'.cpp'))
 else: assert 'new CamTranslatorDummy(this)' in source.read_text(),name
sources += [SOURCE/(name+'.cpp') for name in ('CameraHolder','CameraParamChunkHolder','CameraParamChunk','CameraParamChunkID','CameraParamString','DotCamParams','GameCameraCreator','CameraPolygonCodeUtil')]
assert len(sources)==len(set(sources))
root_sources={source.stem:source for source in sources}
sources=[HERE/'staged/Game/Camera'/source.name if (HERE/'staged/Game/Camera'/source.name).exists() else source for source in sources]
cache=PC/'build/.deps/smg-pc-game/macosx/arm64/debug/src/compat/J3DJointCompat.cpp.o.d'
args=[json.loads(s) for s in re.findall(r'"(?:\\.|[^"\\])*"',re.search(r'values = \{(.*?)\n    \}',cache.read_text(),re.S).group(1))]
BUILD.mkdir(parents=True,exist_ok=True)
commands=[]
for source in sources:
 command=args+['-I'+str(HERE/'staged'),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'libs/JSystem/include'),'-c',str(source),'-o',str(BUILD/(source.stem+'.o'))]
 commands.append(command)
(BUILD/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
def compile_item(item):
 source,command=item
 with (BUILD/(source.stem+'.log')).open('w') as log:
  result=subprocess.run(command,cwd=PC,stdout=log,stderr=subprocess.STDOUT)
 return {'source':str(source.relative_to(ROOT)),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'root_source':str(root_sources[source.stem].relative_to(ROOT)),'root_source_sha256':hashlib.sha256(root_sources[source.stem].read_bytes()).hexdigest(),'compiled':result.returncode==0,
  'errors': [line for line in (BUILD/(source.stem+'.log')).read_text().splitlines() if 'error:' in line]}
with ThreadPoolExecutor(max_workers=4) as pool: results=list(pool.map(compile_item,zip(sources,commands)))
report={'scope':'Isolated root sources and two explicitly recorded staged compiler adaptations compile against current PC headers first plus original missing-header fallback. This does not import Game code or validate linking/runtime. No substitutions or fake declarations.', 'registry_type_count':len(classes),'sources':results}
(HERE/'native-probe.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'{sum(row["compiled"] for row in results)}/{len(results)} original registry/camera/translator sources compile against current native headers')
for row in results:
 if not row['compiled']:print(row['source']+': '+' | '.join(row['errors'][:3]))
