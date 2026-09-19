#!/usr/bin/env python3
"""One-time canonical-first point and segmented-line source restoration."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent

def historical(commit,path):
    return subprocess.check_output(['git','-C',str(ROOT/'decomp'),'show',commit+':'+path],text=True)
def function(text,name):
    start=text.index(name);start=text.rfind('\n',0,start)+1;opening=text.index('{',start);depth=0
    for i in range(opening,len(text)):
        if text[i]=='{':depth+=1
        elif text[i]=='}':
            depth-=1
            if not depth:return text[start:i+1]
    raise ValueError(name)
def replace_or_insert(path,name,body,marker):
    p=ROOT/path;t=p.read_text()
    if name in t:t=t.replace(function(t,name),body,1)
    else:
        assert marker in t,(path,marker);t=t.replace(marker,body+'\n\n'+marker,1)
    p.write_text(t)
proof=json.loads((NOTES/'restored-query-provenance.json').read_text())
point='4ba3e14250f6d67fbf7c963bf894169b674ab81c';fast='0ccf9ac9b4f63f42e4892193505a4798c865ed1c'
for donor,path,name,native,marker in [
(point,'src/Game/Map/CollisionParts.cpp','CollisionParts::checkStrikePoint(','src/compat/OriginalCollisionPartsCompat.cpp','u32 CollisionParts::checkStrikeBall('),
(point,'src/Game/Map/CollisionCategorizedKeeper.cpp','CollisionCategorizedKeeper::checkStrikePoint(','src/Game/Map/CollisionCategorizedKeeper.cpp','s32 CollisionCategorizedKeeper::checkStrikeBall('),
(point,'src/Game/Map/KCollision.cpp','KCollisionServer::checkPoint(','src/compat/OriginalKCollisionCompat.cpp','u32 KCollisionServer::checkArea3D('),
]:
    body=function(historical(donor,path),name)
    replace_or_insert('decomp/'+path,name,body,marker)
    if 'CollisionParts' in name:replace_or_insert(path,name,body,marker)
    replace_or_insert(native,name,body,marker)
    if 'CollisionParts' in name:
        p=ROOT/native;t=p.read_text();t=t.replace(body,body.replace('{\n','{\n    requirePublishedGeometry(*this);\n',1),1);p.write_text(t)
    proof.append(dict(function=name,canonical='decomp/'+path,native=native,donor_commit=donor,body_sha256=hashlib.sha256(body.encode()).hexdigest(),historical_instruction_proof='notes/original-collision-point-query-20260903/compiler-evidence.json'))
p=ROOT/'decomp/include/Game/Map/CollisionParts.hpp';p.write_text(p.read_text().replace('void checkStrikePoint(', 'bool checkStrikePoint('))
# Original MapUtil helpers share the existing original sorted-hit buffer and selector.
map_path='src/Game/Util/MapUtil.cpp';source=historical(fast,map_path)
for name,marker in [
('bool getFirstPolyOnLineCategoryExceptSensor(', '    bool getFirstPolyOnLineCategoryExceptActor('),
('bool getFirstPolyNormalOnLineToMap(', '    u32 getNearPolyOnLineSort('),
('const Triangle* getCameraPolyFast(', '    bool isExistMapCollision('),
('bool getFirstPolyOnLineBFast(', '    bool isExistMapCollision('),
]:
    body=function(source,name)
    for path in ['decomp/'+map_path,map_path]:
        p=ROOT/path;t=p.read_text();comment='    // '+name.split('(')[0].split()[-1]
        if comment in t:t=t.replace(comment,body,1);p.write_text(t)
        else:replace_or_insert(path,name,body,marker)
    replace_or_insert('src/compat/OriginalMapQueries.cpp',name,body,marker)
    if name!='bool getFirstPolyOnLineCategoryExceptSensor(':
        p=ROOT/'src/compat/GameMapCollisionCompat.cpp';t=p.read_text();t=t.replace(function(t,name)+'\n','',1);p.write_text(t)
    proof.append(dict(function=name,canonical='decomp/'+map_path,native='src/compat/OriginalMapQueries.cpp',donor_commit=fast,body_sha256=hashlib.sha256(body.encode()).hexdigest(),historical_instruction_proof='notes/original-map-query-access-20260903/compiler-evidence.json' if 'ExceptSensor' in name or 'NormalOnLine' in name else 'historical:0ccf9ac9b:pc-port/notes/original-map-fast-query-20260903/compiler-evidence.json'))
for name in ['bool checkStrikePointToMap(', 's32 checkStrikePointToMap(']:
    body=function(source,name)
    for path in ['decomp/'+map_path,map_path]:
        replace_or_insert(path,name,body,'    s32 checkStrikeBallToMap(')
    replace_or_insert('src/compat/GameMapCollisionCompat.cpp',name,body,'    s32 checkStrikeBallToMap(')
    proof.append(dict(function=name,canonical='decomp/'+map_path,native='src/compat/GameMapCollisionCompat.cpp',donor_commit=fast,body_sha256=hashlib.sha256(body.encode()).hexdigest(),historical_instruction_proof='notes/original-map-query-access-20260903/compiler-evidence.json'))
# These helpers had no remaining consumers after the exact point and fast wrappers.
p=ROOT/'src/compat/GameMapCollisionCompat.cpp';t=p.read_text()
for name in ['[[nodiscard]] smgpc::scene::StageCollisionService& require_stage_collision(', '[[nodiscard]] HitInfo make_hit_info(', '[[nodiscard]] bool first_line_hit(']:t=t.replace(function(t,name)+'\n','',1)
p.write_text(t)
p=ROOT/'src/compat/OriginalMapQueries.cpp';t=p.read_text();t=t.replace('#include "Game/Util/MapUtil.hpp"','#include "Game/Util/MapUtil.hpp"\n#include "Game/Util/MathUtil.hpp"',1);p.write_text(t)
(NOTES/'restored-query-provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
