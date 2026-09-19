#!/usr/bin/env python3
"""One-time canonical-first area Keeper/Parts recovery and native forwarding."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent
ns={};s=(NOTES/'restore_verified_queries.py').read_text();exec(s[s.index('def function('):s.index('\ndef insert(')],ns);function=ns['function']
commit='6ad1afcfb5ea7e90ce0e87074ed16897930d0135'
proof=json.loads((NOTES/'restored-query-provenance.json').read_text())
for path,native in [('src/Game/Map/CollisionParts.cpp','src/compat/OriginalCollisionPartsCompat.cpp'),('src/Game/Map/CollisionCategorizedKeeper.cpp','src/Game/Map/CollisionCategorizedKeeper.cpp')]:
    donor=subprocess.check_output(['git','-C',str(ROOT/'decomp'),'show',commit+':'+path],text=True)
    cls=Path(path).stem
    for method in ['createAreaPolygonList','createAreaPolygonListArray']:
        name=cls+'::'+method+'(';body=function(donor,name)
        for out in dict.fromkeys(['decomp/'+path,path,native]):
            p=ROOT/out;t=p.read_text()
            if name in t:t=t.replace(function(t,name),body,1)
            else:
                marker='u32 '+cls+'::createAreaPolygonListArray(';assert marker in t;t=t.replace(marker,body+'\n\n'+marker,1)
            p.write_text(t)
        if cls=='CollisionParts':
            p=ROOT/native;t=p.read_text();t=t.replace(body,body.replace('{\n','{\n    requirePublishedGeometry(*this);\n',1),1);p.write_text(t)
        proof.append(dict(function=name,canonical='decomp/'+path,native=native,donor_commit=commit,body_sha256=hashlib.sha256(body.encode()).hexdigest(),historical_instruction_proof='historical:6ad1afcfb:pc-port/notes/original-collision-area-query-20260903/compiler-evidence.json'))
# Public original MapUtil wrappers already exist canonically. Keep their calls,
# with the host's existing validation of original finite stack-buffer contracts.
p=ROOT/'src/compat/GameMapCollisionCompat.cpp';t=p.read_text();canonical=(ROOT/'decomp/src/Game/Util/MapUtil.cpp').read_text()
for name in ['u32 createAreaPolygonList(', 'u32 createAreaPolygonListArray(']:
    body=function(canonical,name);t=t.replace(function(t,name),body,1)
    proof.append(dict(function=name,canonical='decomp/src/Game/Util/MapUtil.cpp',native='src/compat/GameMapCollisionCompat.cpp',donor_source='existing canonical MapUtil',body_sha256=hashlib.sha256(body.encode()).hexdigest()))
p.write_text(t)
(NOTES/'restored-query-provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
