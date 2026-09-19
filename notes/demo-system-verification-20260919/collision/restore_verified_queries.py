#!/usr/bin/env python3
"""One-time explicit restoration from previously instruction-verified donor bodies."""
from pathlib import Path
import subprocess,hashlib,json
ROOT=Path(__file__).resolve().parents[3]
NOTES=Path(__file__).resolve().parent

def historical(commit,path):
    return subprocess.check_output(['git','-C',str(ROOT/'decomp'),'show',commit+':'+path],text=True)

def function(text,name):
    start=text.index(name)
    start=text.rfind('\n',0,start)+1
    opening=text.index('{',start)
    depth=0
    for i in range(opening,len(text)):
        if text[i]=='{':depth+=1
        elif text[i]=='}':
            depth-=1
            if depth==0:return text[start:i+1]
    raise ValueError(name)

def insert(path,marker,bodies):
    p=ROOT/path
    original=p.read_text()
    assert marker in original
    for body in bodies:
        signature=body[:body.index('{')].strip()
        assert signature not in original
    (NOTES/'before').mkdir(exist_ok=True)
    (NOTES/'before'/path.replace('/','__')).write_text(original)
    p.write_text(original.replace(marker,'\n\n'.join(bodies)+'\n\n'+marker,1))

proof=[]
parts_source=historical('1419b6e601aee71b3e1483cd776cb47a4d648dc6','src/Game/Map/CollisionParts.cpp')
parts=[]
for name in ['CollisionParts::checkStrikeBall(', 'CollisionParts::checkStrikeBallCore(',
             'CollisionParts::checkStrikeBallWithThickness(', 'CollisionParts::calcCollidePosition(']:
    body=function(parts_source,name);parts.append(body)
    proof.append({'function':name,'donor_commit':'1419b6e601aee71b3e1483cd776cb47a4d648dc6',
                  'body_sha256':hashlib.sha256(body.encode()).hexdigest(),
                  'historical_instruction_proof':'notes/original-collision-parts-owner-20260903/source-evidence.json'})
insert('decomp/src/Game/Map/CollisionParts.cpp','void CollisionParts::projectToPlane(',parts)
insert('src/compat/OriginalCollisionPartsCompat.cpp','void CollisionParts::projectToPlane(',parts)
# Keep the excluded mirror current as well; the linked provider adds only its ownership publication hooks.
insert('src/Game/Map/CollisionParts.cpp','void CollisionParts::projectToPlane(',parts)
for path in ['decomp/include/Game/Map/CollisionParts.hpp']:
    p=ROOT/path;s=p.read_text()
    s=s.replace('bool checkStrikeBall(', 'u32 checkStrikeBall(').replace('void checkStrikeBallCore(', 'u32 checkStrikeBallCore(').replace('void checkStrikeBallWithThickness(', 'u32 checkStrikeBallWithThickness(')
    p.write_text(s)
keeper_source=historical('b10d8d4e7','src/Game/Map/CollisionCategorizedKeeper.cpp')
keeper=[]
for name in ['CollisionCategorizedKeeper::checkStrikeBall(', 'CollisionCategorizedKeeper::checkStrikeBallWithThickness(']:
    body=function(keeper_source,name);keeper.append(body)
    proof.append({'function':name,'donor_commit':'b10d8d4e7', 'body_sha256':hashlib.sha256(body.encode()).hexdigest(),
                  'historical_instruction_proof':'notes/original-collision-keeper-query-20260903/source-evidence.json'})
insert('decomp/src/Game/Map/CollisionCategorizedKeeper.cpp','s32 CollisionCategorizedKeeper::checkStrikeLine(',keeper)
insert('src/Game/Map/CollisionCategorizedKeeper.cpp','s32 CollisionCategorizedKeeper::checkStrikeLine(',keeper)
kcl_commit='104a86bec087f85d96675f0e8f8eaed46c43f9ac'
kcl_source=historical(kcl_commit,'src/Game/Map/KCollision.cpp')
kcl=[]
for name in ['KCollisionServer::checkSphere(', 'KCollisionServer::checkSphereWithThickness(',
             'KCollisionServer::KCHitSphere(', 'KCollisionServer::KCHitSphereWithThickness(']:
    body=function(kcl_source,name);kcl.append(body)
    canonical=ROOT/'decomp/src/Game/Map/KCollision.cpp'
    text=canonical.read_text();canonical.write_text(text.replace(function(text,name),body,1))
    proof.append({'function':name,'donor_commit':kcl_commit,
                  'body_sha256':hashlib.sha256(body.encode()).hexdigest(),
                  'historical_instruction_proof':'notes/original-kcollision-traversal-20260903/source-evidence.json' if '::checkSphere' in name else 'notes/original-kcollision-query-20260903/source-evidence.json'})
insert('src/compat/OriginalKCollisionCompat.cpp','KC_PrismData* KCollisionServer::checkArrow(',kcl)
patch=(ROOT/'notes/original-map-query-access-20260903/root.patch').read_text()
map_source='\n'.join(line[1:] for line in patch.splitlines() if line.startswith('+') and not line.startswith('+++'))
wrappers=[]
for name in ['s32 checkStrikeBallToMap(', 's32 checkStrikeBallToMapWithMovingReaction(', 's32 checkStrikeBallToMapWithThickness(']:
    body=function(map_source,name);wrappers.append(body)
    proof.append({'function':name,'donor_source':'notes/original-map-query-access-20260903/root.patch',
                  'body_sha256':hashlib.sha256(body.encode()).hexdigest(),
                  'historical_instruction_proof':'notes/original-map-query-access-20260903/compiler-evidence.json'})
insert('decomp/src/Game/Util/MapUtil.cpp','    s32 checkStrikeLineToMap(',wrappers)
insert('src/Game/Util/MapUtil.cpp','    s32 checkStrikeLineToMap(',wrappers)
p=ROOT/'src/compat/GameMapCollisionCompat.cpp';s=p.read_text()
(NOTES/'before'/'src__compat__GameMapCollisionCompat.cpp').write_text(s)
for body in wrappers:
    name=body.strip().split('(')[0].split()[-1]
    old=function(s,'s32 '+name+'(')
    s=s.replace(old,body,1)
# Remove the now-unreachable sphere alternative and its exclusively used filter.
for name in ['smgpc::scene::StageCollisionTriangleFilter make_query_filter(', '[[nodiscard]] s32 store_sphere_contacts(']:
    old=function(s,name);s=s.replace(old+'\n','',1)
s=s.replace('    constexpr auto cMaximumStrikeInfos = std::size_t{32U};\n','')
p.write_text(s)
(NOTES/'restored-query-provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
