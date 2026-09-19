#!/usr/bin/env python3
"""Check restored function bodies against instruction-verified historical sources."""
import hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
NOTES=Path(__file__).resolve().parent

def get_file(commit,path):
    return subprocess.check_output(['git','-C',str(ROOT/'decomp'),'show',commit+':'+path],stderr=subprocess.DEVNULL)
def function(text,name):
    start=text.index(name);start=text.rfind('\n',0,start)+1;begin=text.index('{',start);depth=0
    for i in range(begin,len(text)):
        if text[i]=='{':depth+=1
        elif text[i]=='}':
            depth-=1
            if not depth:return text[start:i+1]
    raise ValueError(name)
def tokens(text):
    return re.sub(r'\s+','',re.sub(r'//[^\n]*|/\*.*?\*/','',text,flags=re.S))
checks=[]
for entry in json.loads((NOTES/'restored-query-provenance.json').read_text()):
    name=entry['function']
    if 'canonical' in entry:
        canonical=entry['canonical'];native=entry['native']
    elif name.startswith('CollisionParts::'):
        canonical='decomp/src/Game/Map/CollisionParts.cpp';native='src/compat/OriginalCollisionPartsCompat.cpp'
    elif name.startswith('CollisionCategorizedKeeper::'):
        canonical='decomp/src/Game/Map/CollisionCategorizedKeeper.cpp';native='src/Game/Map/CollisionCategorizedKeeper.cpp'
    elif name.startswith('KCollisionServer::'):
        canonical='decomp/src/Game/Map/KCollision.cpp';native='src/compat/OriginalKCollisionCompat.cpp'
    else:
        canonical='decomp/src/Game/Util/MapUtil.cpp';native='src/compat/GameMapCollisionCompat.cpp'
    a=function((ROOT/canonical).read_text(),name);b=function((ROOT/native).read_text(),name)
    b=b.replace('    requirePublishedGeometry(*this);\n','')
    if name.startswith('CollisionCategorizedKeeper::'):
        b=b.replace('#if defined(TARGET_PC)\n    smgpc::compat::require_published_collision_geometry(_9C);\n#endif\n','')
    if name.startswith('u32 createAreaPolygonList'):
        b=b.replace('        const TVec3f points[] = {rParam3, rParam4};\n','')
        b=b.replace('        if (!validateAreaQuery(pTriangle, param2, points, 2U)) return 0;\n','')
        b=b.replace('        if (!validateAreaQuery(pTriangle, param2, pParam3, param4)) return 0;\n','')
    if name=='KCollisionServer::checkPoint(':
        for axis in 'xyz':
            b=b.replace('aurora::ppc::truncate_s32(pPos->'+axis+' - mFile->mMin.'+axis+')', 'static_cast< s32 >(pPos->'+axis+' - mFile->mMin.'+axis+')')
    assert tokens(a)==tokens(b),(name,'native mismatch')
    assert hashlib.sha256(a.encode()).hexdigest()==entry['body_sha256'],(name,'canonical differs from recovered body')
    checks.append({'function':name,'canonical':canonical,'native':native,'canonical_recovered_bytes_equal':True,
                   'native_tokens_equal_excluding_listed_host_boundaries':True})
# Match the exact historical source hashes recorded with the retail instruction
# proofs. This proves those reports belong to source available in donor history.
proof_files=[
 ('notes/original-collision-parts-owner-20260903/source-evidence.json','src/Game/Map/CollisionParts.cpp'),
 ('notes/original-collision-keeper-query-20260903/source-evidence.json','src/Game/Map/CollisionCategorizedKeeper.cpp'),
 ('notes/original-kcollision-query-20260903/source-evidence.json','src/Game/Map/KCollision.cpp'),
 ('notes/original-kcollision-traversal-20260903/source-evidence.json','src/Game/Map/KCollision.cpp'),
 ('notes/original-map-query-access-20260903/compiler-evidence.json','src/Game/Util/MapUtil.cpp'),
 ('historical:0ccf9ac9b:pc-port/notes/original-map-fast-query-20260903/compiler-evidence.json','src/Game/Util/MapUtil.cpp'),
]
history=[]
for proof,path in proof_files:
    if proof.startswith('historical:'):
        _,revision,report_path=proof.split(':',2);report=json.loads(get_file(revision,report_path))
    else:report=json.loads((ROOT/proof).read_text())
    hashes=report['source_sha256'];expected=hashes.get(path,hashes.get(path.removeprefix('src/').removesuffix('.cpp')))
    if expected is None:raise ValueError((proof,path,hashes))
    found=None
    for commit in subprocess.check_output(['git','-C',str(ROOT/'decomp'),'log','--all','--format=%H','--',path],text=True).splitlines():
        try:data=get_file(commit,path)
        except subprocess.CalledProcessError:continue
        if hashlib.sha256(data).hexdigest()==expected:found=commit;break
    history.append({'proof':proof,'source':path,'recorded_sha256':expected,'matching_history_commit':found})
    assert found,(proof,'recorded source missing from history')
    text=get_file(found,path).decode()
    # Match only methods present in this historical proof (KCL narrow/traversal
    # reports were recorded at different stages of the original recovery).
    names=[r.get('name',r.get('symbol','')) for r in report['functions']]
    for check in checks:
        method=check['function'].split('::')[-1].removesuffix('(').removeprefix('s32 ')
        if check['canonical']=='decomp/'+path and any(n.startswith(method+'__') for n in names):
            old=function(text,check['function'])
            current=function((ROOT/check['canonical']).read_text(),check['function'])
            assert tokens(old)==tokens(current),(proof,check['function'],'different historical body')
            check.setdefault('matching_instruction_reports',[]).append(proof)
# Point recovery's instruction report lacks a whole-source hash. Verify its
# bodies against the atomic donor commit containing source, root.patch and report;
# independently verify the Parts/Keeper whole-source hashes in its native report.
point_commit='4ba3e14250f6d67fbf7c963bf894169b674ab81c'
point_report='notes/original-collision-point-query-20260903/compiler-evidence.json'
point=json.loads((ROOT/point_report).read_text())
point_checks=[]
for check in checks:
    if check['function'] not in ['CollisionParts::checkStrikePoint(', 'CollisionCategorizedKeeper::checkStrikePoint(', 'KCollisionServer::checkPoint(']:continue
    old=function(get_file(point_commit,check['canonical'].removeprefix('decomp/')).decode(),check['function'])
    assert tokens(old)==tokens(function((ROOT/check['canonical']).read_text(),check['function']))
    method=check['function'].split('::')[-1].removesuffix('(')
    report=next(f for f in point['functions'] if f.get('name',f.get('symbol','')).startswith(method+'__'+('14CollisionParts' if 'CollisionParts::' in check['function'] else '26CollisionCategorizedKeeper' if 'CollisionCategorizedKeeper::' in check['function'] else '16KCollisionServer')))
    assert report['all_canonical_instructions_equal']
    check.setdefault('matching_instruction_reports',[]).append(point_report)
    point_checks.append({'function':check['function'],'commit':point_commit,'same_commit_source_and_instruction_report':True})
for row in json.loads((ROOT/'notes/original-collision-point-query-20260903/native-evidence.json').read_text()):
    if row['file'] not in ['Game/Map/CollisionParts.cpp','Game/Map/CollisionCategorizedKeeper.cpp']:continue
    assert hashlib.sha256(get_file(point_commit,'src/'+row['file'])).hexdigest()==row['source_sha256']
area_commit='6ad1afcfb5ea7e90ce0e87074ed16897930d0135'
area_report='pc-port/notes/original-collision-area-query-20260903/compiler-evidence.json'
area=json.loads(get_file(area_commit,area_report))
area_checks=[]
for check in checks:
    if not ('::createAreaPolygonList' in check['function']):continue
    old=function(get_file(area_commit,check['canonical'].removeprefix('decomp/')).decode(),check['function'])
    assert tokens(old)==tokens(function((ROOT/check['canonical']).read_text(),check['function']))
    cls,method=check['function'].split('::');method=method.removesuffix('(')
    row=next(f for f in area['functions'] if f.get('name',f.get('symbol','')).startswith(method+'__'+str(len(cls))+cls))
    assert row['all_canonical_instructions_equal']
    check.setdefault('matching_instruction_reports',[]).append('historical:'+area_commit+':'+area_report)
    area_checks.append({'function':check['function'],'commit':area_commit,'same_commit_source_and_instruction_report':True})
result={'allowed_native_boundaries':['generated geometry publication guard at Parts and TARGET_PC Keeper entries before culling','three PPC saturating point-grid conversions','existing native area finite/null/fixed-buffer contract'], 'function_checks':checks,'historical_source_proofs':history,'point_recovery_proofs':point_checks,'area_recovery_proofs':area_checks,
        'limits':'Source/token and historical proof association only; no new MWCC or native build/run performed by this script.'}
(NOTES/'source-provenance-check.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS:',len(checks),'canonical/native bodies and',len(history),'historical instruction-proof source hashes')
