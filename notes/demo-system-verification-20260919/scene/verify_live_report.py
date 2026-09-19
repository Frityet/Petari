"""Compare the actual original-process report with hash-revalidated disc metadata.
Read-only for the process/game; writes only this audit's result JSON.
"""
from pathlib import Path
import collections, hashlib, json, math
base=Path(__file__).parent
report_path=base.parent/'final-route-placements.json'
report=json.loads(report_path.read_text())
inventory_path=Path('notes/gateway-content-inventory-20260919/source-placement-inventory.json')
inventory=json.loads(inventory_path.read_text())
audit=json.loads((base/'omitted-placement-classification.json').read_text())
expected={}
for row in inventory['rows']:
    if row['source_status'] in ['original_parent_owned_child','child_metadata_unverified_parent']:continue
    key=(row['zone_id'],row['table'],row['row'])
    assert key not in expected,key
    expected[key]=row
root_data=json.loads(Path('notes/gateway-content-inventory-20260919/root-zone.json').read_text())
start=root_data['tables']['/jmp/start/layera/startinfo'][0]
expected[(0,'/jmp/start/layera/startinfo',0)]={'zone_id':0,'zone':'HeavensDoorGalaxy','table':'/jmp/start/layera/startinfo','row':0,'l_id':None,'creator_identifier':'Mario','source_status':'ordinary_factory','raw_row':start}
actual={}
for entry in report['entries']:
    key=(entry['zone'],entry['table_path'],entry['row'])
    assert key not in actual,('duplicate report row',key)
    actual[key]=entry
assert set(actual)==set(expected),('row set mismatch',set(actual)-set(expected),set(expected)-set(actual))
classification={ (e['zone_id'],e['table'],e['row']):e for e in audit['rows'] }
for key,entry in actual.items():
    raw=expected[key]
    assert entry['zone_name']==raw['zone'],key
    assert entry['object']==raw['creator_identifier'],(key,entry['object'],raw['creator_identifier'])
    assert entry['layer']==raw['table'].split('/')[3],key
    assert entry['table']==raw['table'].rsplit('/',1)[1],key
    assert entry['l_id']==(raw['l_id'] if raw['l_id'] is not None else -1),key
    if key in classification:
        availability='supported' if classification[key]['current_creator_registered'] else 'known_unlinked'
    else:
        availability='metadata' if raw['source_status']=='zone_holder_metadata' else 'supported'
    assert entry['availability']==availability,(key,entry['availability'],availability)
zone_counts=dict(sorted(collections.Counter(e['zone'] for e in actual.values()).items()))
assert zone_counts=={0:38,1:24,2:44,4:30,5:84,6:23},zone_counts
layers=dict(collections.Counter(e['layer'] for e in actual.values()))
assert layers=={'common':205,'layera':38},layers
counts=collections.Counter(e['availability'] for e in actual.values())
assert counts=={'supported':179,'known_unlinked':59,'metadata':5},counts
assert (report['supported'],report['known_unlinked'],report['unknown'],report['metadata'])==(179,59,0,5)
matrices={0: [[1,0,0,0],[0,1,0,0],[0,0,1,0]]}
paths={0:[]}
for i,a in enumerate(audit['attached_zone_instances']):
    matrices[a['zone_id']]=a['matrix'];paths[a['zone_id']]=[i]
errors=collections.defaultdict(float)
for entry in actual.values():
    z=entry['zone'];matrix=entry['zone_placement_matrix']
    assert len(matrix)==12 and all(math.isfinite(x) for x in matrix),entry
    assert entry['holder_path']==paths[z],entry
    for i in range(3):
        assert matrix[i*4+3]==matrices[z][i][3],(z,'translation')
        for j in range(3):
            errors[z]=max(errors[z],abs(matrix[i*4+j]-matrices[z][i][j]))
# Float trigonometric/argument rounding, not an actor-position tolerance. This
# independently checks the reported matrix against authored Rz*Ry*Rx values.
assert max(errors.values())<1e-5,dict(errors)
result={'status':'PASS','scope':'Exact retained placement identities, source availability and owner provenance; does not establish actor behavior, physics parity, rendered visibility or route completion.',
 'report':str(report_path),'report_sha256':hashlib.sha256(report_path.read_bytes()).hexdigest(),
 'raw_inventory':str(inventory_path),'raw_inventory_sha256':hashlib.sha256(inventory_path.read_bytes()).hexdigest(),
 'row_count':len(actual),'availability_counts':dict(counts),'zone_counts':zone_counts,'layer_counts':layers,
 'holder_paths':paths,'max_rotation_matrix_absolute_error_by_zone':dict(sorted(errors.items())),
 'translations':'Every reported translation equals the authored float exactly',
 'selected_start':actual[(0,'/jmp/start/layera/startinfo',0)],
 'restart_rows':[e for e in actual.values() if e['object']=='RestartCube'],
 'known_unlinked_by_name':dict(sorted(collections.Counter(e['object'] for e in actual.values() if e['availability']=='known_unlinked').items()))}
(base/'final-route-report-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(actual),'exact row identities;',dict(counts),'; matrix max abs error',max(errors.values()))
