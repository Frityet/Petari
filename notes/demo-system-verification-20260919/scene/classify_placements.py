"""Read-only audit from hash-revalidated retail tables and current factory sources.
No actors, gameplay state, archive bytes or production source are modified.
"""
from pathlib import Path
import collections, hashlib, json, math, re
out = Path(__file__).parent
prior = Path('notes/gateway-content-inventory-20260919')
inventory = json.loads((prior/'source-placement-inventory.json').read_text())
provenance = json.loads((out/'fresh-disc-provenance.json').read_text())
for record in provenance:
    assert hashlib.sha256(Path(record['extracted_path']).read_bytes()).hexdigest() == record['sha256']
    assert record['matches_prior_disc_extraction']
source_paths = ['src/scene/nameobj/NameObjFactory.cpp','src/scene/AreaObjRuntime.cpp',
 'src/Game/Scene/StageDataHolder.cpp','src/Game/Util/JMapUtil.cpp','src/Game/Util/SceneUtil.cpp',
 'src/Game/AreaObj/RestartCube.cpp','src/compat/StageSessionGameCompat.cpp']
factory = Path(source_paths[0]).read_text()
area = Path(source_paths[1]).read_text()
explicit = set(re.findall(r'NameObjFactory::Name2CreateFunc\{\s*"([^"]+)"',factory))
explicit.update(re.findall(r'\.object_name\s*=\s*"([^"]+)"',area))
donor = Path('decomp/src/Game/NameObj/NameObjFactory.cpp').read_text()
constructors = dict(re.findall(r'\{\s*"([^"]+)"\s*,\s*([^,\n]+)\s*,\s*(?:"[^"]*"|nullptr)\s*,?\s*\}',donor))
zone_tables = {'HeavensDoorGalaxy':json.loads((prior/'root-zone.json').read_text())}
for name in inventory['instantiated_zone_names'][1:]:
    file = Path('notes/original-rabbit-tower-chain-20260919/zone-metadata.json') if name=='HeavensDoorMysteriousZone' else prior/(name+'.json')
    zone_tables[name] = json.loads(file.read_text())
    assert zone_tables[name]['archive_sha256'] == next(r['sha256'] for r in provenance if Path(r['disc_path']).name==name+'.arc')

def rotation(row):
    x,y,z = [math.radians(row['dir_'+c]) for c in 'xyz']
    sx,cx,sy,cy,sz,cz = math.sin(x),math.cos(x),math.sin(y),math.cos(y),math.sin(z),math.cos(z)
    return [[cy*cz,sx*sy*cz-cx*sz,cx*sy*cz+sx*sz],
            [cy*sz,sx*sy*sz+cx*cz,cx*sy*sz-sx*cz],[-sy,sx*cy,cx*cy]]
attachments = []
for path,rows in zone_tables['HeavensDoorGalaxy']['tables'].items():
    if '/stageobjinfo' in path and any('/'+layer+'/' in path for layer in ['common','layera']):
        for row in rows:
            attachments.append({'zone_name':row['name'],'zone_id':next(r['zone_id'] for r in inventory['rows'] if r['zone']==row['name']),
                'table':path,'row':row['row'],'raw_row':row,'matrix_model':'double-precision Rz*Ry*Rx; audit approximation, not captured native matrix',
                'matrix':[rotation(row)[i]+[row['pos_'+'xyz'[i]]] for i in range(3)]})
matrices = {a['zone_name']:a['matrix'] for a in attachments}
matrices['HeavensDoorGalaxy'] = [[1,0,0,0],[0,1,0,0],[0,0,1,0]]

def classify(entry):
    n,z,r = entry['creator_identifier'],entry['zone'],entry['raw_row']
    if n=='RestartCube':
        return 'registered_original_owner_validated','Exact original class already linked; original MarioCollision update dispatch and GameSequenceDirector restart owner now present. Generic Cube2 descriptor validated by actual-process120: all four authored checkpoint IDs1..4 dispatch into original sequence-owned state and restore; no start0 checkpoint.'
    if n in ['ChangeBgmCube','AudioEffectSphere']:
        return 'audio_area_omitted','Canonical actor changes BGM/audio effect controls. Audio output is user-deferred; absence remains an explicit scene setup difference, not a claim of complete reproduction.'
    if n=='SpinGuidanceCube' and z=='HeavensDoorMysteriousZone':
        return 'post_spawn_switch_gated','SW_APPEAR1016; Rosetta NPCActor::kill writes SW_DEAD1016 during later spin-get demo8 (sheet action29), after her first appearance. Missing native PlayerActionGuidance still prevents this original area from being created.'
    if n=='BlackHole':
        return 'post_spawn_switch_gated','SW_APPEAR1016 has the same later Rosetta death/spin-get writer. Original black-hole actor still absent; later hazard correctness is not established.'
    if n in ['SuperSpinDriver','SpinDriver']:
        return 'downstream_launch_gate','SW_APPEAR%d; downstream writers are CrystalCageM group2 (1017), YellowChipGroup (1009), or later SmallZone progression (1127). Not a direct rabbit catch/tower-sheet dependency; canonical bound-Mario launch behavior remains absent.' % r['SW_APPEAR']
    if 'RotateParts' in n:
        return 'omitted_map_geometry','Retail creator RotateMoveObj initializes SimpleMapObj/MapObjActor model, hit sensor, collision and MapPartsRotator. Two Middle rows are ungated; three Inside rows use local SW_APPEAR1. Fresh retail archives contain BDL and KCL for all five variants. Distant placement alone does not prove visually irrelevant.'
    if n=='KoopaJrNormalShipA':
        return 'omitted_map_geometry','Retail creator SimpleMapObj (already implemented), archive KoopaJrNormalShipA plus low-model registration. Fresh retail archive contains BDL plus main and MoveLimit KCL. Three ungated MiddleZone rows: missing scene geometry, not a demonstrated harmless omission.'
    if n=='RailCoin':
        return 'other_attached_zone_content_unproven','Normal rail coin path has current rail/coin owners, but canonical optional Mercator division is still explicitly unavailable; blanket shadow/area reason is partly stale. No direct initial-planet reveal/tower gate. Visibility and later collection unverified.'
    if n=='CrystalCageS':
        return 'other_attached_zone_content_unproven','Exact CrystalCage already imported for M variant, but S archive/collision/break model and makeArchiveListDummyDisplayModel callback require their own proof. Three BlackHoleZone rows publish local80/81/82; these connect chips/guidance, not the initial rabbit reveal switches.'
    if n in ['YellowChip','YellowChipGroup']:
        return 'other_attached_zone_content_unproven','BlackHoleZone chip collection/group0 controls global1009 for its launch star; one chip requires local80. Real missing collection/reward chain; no direct initial rabbit/tower-sheet input, no visual parity claim.'
    if n=='ExterminationKuriboKeySwitch':
        return 'other_attached_zone_content_unproven','SmallZone SW_APPEAR1124 and output SW_A1125 connect the local enemy/key/CapsuleCage chain. It does not use collector reveal1112/1113/1114/1118 or tower1015.'
    if n=='KuriboChief':
        return 'other_attached_zone_content_unproven','MiddleZone enemy writes global1100, paired with its CapsuleCage SW_B1100. Enemy AI/sensors/reward remain missing, including potential distant visibility.'
    if n=='CapsuleCage':
        return 'other_attached_zone_content_unproven','Small/Middle cages use SW_B1125/1100; these are separate later-zone enemy/key paths. Authored collision/model/contents absent; no initial collector gate reference.'
    if n=='MeteorCannon':
        return 'other_attached_zone_content_unproven','BlackHoleZone cannons require localSW21 and distinct authored rails0/1; original projectile/hazard ownership absent. No claim that current clipping excludes every relevant visual.'
    if n=='SpinGuidanceCube':
        return 'other_attached_zone_content_unproven','Small/BlackHole local guidance depends on original PlayerActionGuidance/SpinGuidance layout owner, absent natively. BlackHole areas use local80/81/82. Not proof of harmlessness before first Rosetta appearance.'
    return 'other_attached_zone_content_unproven','Authored enemy/benefit actor on a different attached planet. No direct initial collector/tower gate was found in placement switches; visual, sensor, collision and eventual progression effects remain unverified.'
rows = []
for old in inventory['unavailable_rows']:
    e = dict(old);raw=e['raw_row'];category,reason=classify(e)
    e.update(audit_category=category,audit_reason=reason,current_creator_registered=e['creator_identifier'] in explicit,
             canonical_creator=constructors.get(e['creator_identifier']),layer=e['table'].split('/')[3])
    m=matrices[e['zone']];v=[raw.get('pos_'+c,0) for c in 'xyz']
    e['approximate_world_origin']=[sum(m[i][j]*v[j] for j in range(3))+m[i][3] for i in range(3)]
    e['world_origin_scope']='Authored matrix evaluated in double precision; not captured actor position'
    rows.append(e)
assert len(rows)==63
assert sum(e['current_creator_registered'] for e in rows)==4
counts=collections.Counter(e['audit_category'] for e in rows)
result={'scope':'63 omitted rows in immutable prior original-queue baseline; four RestartCube rows now registered and actual-process120 owner/dispatch probe passed, leaving59. No whole-scene correctness or harmlessness inferred.',
 'baseline_report':'notes/compat-original-runtime-20260919/gateway-opening-placements.json',
 'baseline_report_sha256':hashlib.sha256(Path('notes/compat-original-runtime-20260919/gateway-opening-placements.json').read_bytes()).hexdigest(),
 'baseline_counts':{'supported':175,'known_unlinked':63,'unknown':0,'metadata':5,'total':243},
 'working_tree_expected_counts':{'supported':179,'known_unlinked':59,'unknown':0,'metadata':5,'total':243},
 'source_sha256':{p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in source_paths},
 'archive_provenance':provenance,'scenario':inventory['scenario'],'attached_zone_instances':attachments,
 'unattached_zone':'HeavensDoorLargeZone is in ZoneList/scenario columns but has no active StageObjInfo instance',
 'category_counts':dict(counts),'rows':rows}
(out/'omitted-placement-classification.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
print(dict(counts))
