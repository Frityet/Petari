#!/usr/bin/env python3
"""Literal original holder gate and current migration source inventory."""
import hashlib,json,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
root=ROOT/'src/Game/System/ResourceHolder.cpp'
pc=ROOT/'pc-port/src/Game/System/ResourceHolder.cpp'
assert root.read_bytes()==pc.read_bytes(), 'original ResourceHolder source must remain byte-identical'
assert (ROOT/'include/Game/System/ResourceHolder.hpp').read_bytes()==(ROOT/'pc-port/src/Game/System/ResourceHolder.hpp').read_bytes()
definitions=[]
for p in (ROOT/'pc-port/src').rglob('*.hpp'):
 if re.search(r'^class ResourceHolder\s*(?:final\s*)?\{',p.read_text(),re.M):definitions.append(str(p.relative_to(ROOT)))
assert definitions==['pc-port/src/Game/System/ResourceHolder.hpp'],definitions
files=[root,pc,ROOT/'pc-port/src/Game/System/ResourceHolder.hpp']
files += [ROOT/('pc-port/src/'+name) for name in ['compat/ResourceHolderCompat.hpp','compat/ResourceHolderCompat.cpp','resource/GameResourceRuntime.hpp','resource/GameResourceRuntime.cpp','runtime/RuntimeServices.hpp','runtime/RuntimeServices.cpp','runtime/RuntimeContext.hpp','runtime/RuntimeContext.cpp','compat/CollisionPartsCompat.cpp','compat/PlanetMapRuntimeCompat.cpp','app/Application.hpp','app/Application.cpp','showcase/Showcase.cpp','debug/StageConstructionProbe.cpp','debug/TitleSequenceProbe.cpp']]
files += [p for p in (ROOT/'pc-port/tests').glob('*.cpp') if 'GameResourceRuntime' in p.read_text()]
evidence={'checkpoint':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
 'original_source_identical':True,'original_header_identical':True,'only_global_holder_definition':definitions[0],
 'sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
(HERE/'source-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS original ResourceHolder source/header identity; one global Game class; '+str(len(files))+' source hashes')
