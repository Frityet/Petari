"""Read-only source/retail table inventory. Does not execute actor constructors."""
from pathlib import Path
import collections,hashlib,json,re
base=Path(__file__).parent
factory_path=Path('src/scene/nameobj/NameObjFactory.cpp');area_path=Path('src/scene/AreaObjRuntime.cpp');planet_path=Path('src/scene/nameobj/PlanetMapCatalog.cpp')
factory=factory_path.read_text();section=factory[factory.index('constexpr auto cSupportedCreateTable'):factory.index('constexpr auto cPlayerArchiveLoaderObjTable')]
creators=dict(re.findall(r'NameObjFactory::Name2CreateFunc\{\s*"([^"]+)"\s*,\s*([^,]+),',section))
areas=dict(re.findall(r'\.object_name\s*=\s*"([^"]+)"\s*,\s*\.object_creator\s*=\s*([^\n]+)',area_path.read_text()))
unique=dict(re.findall(r'\{"([^"]+)", "([^"]+)"\}',planet_path.read_text().split('struct UniqueArchiveName')[0]))
cat=json.loads((base/'planet-map-catalog.json').read_text());planets={r['PlanetName']:r for rows in cat['tables'].values() for r in rows}
scenario=json.loads((base/'HeavensDoorGalaxyScenario.json').read_text());chosen=next(r for r in scenario['tables']['/scenariodata.bcsv'] if r['ScenarioNo']==1)
zoneids={r['ZoneName']:r['row'] for r in scenario['tables']['/zonelist.bcsv']}
files={'HeavensDoorGalaxy':base/'root-zone.json','HeavensDoorMysteriousZone':Path('notes/original-rabbit-tower-chain-20260919/zone-metadata.json')}
for name in ['HeavensBlackHoleZone','HeavensDoorInsideZone','HeavensDoorMiddleZone','HeavensDoorSmallZone']:files[name]=base/(name+'.json')
archive_data={n:json.loads(p.read_text()) for n,p in files.items()}
def active(name,path):
 m=re.search(r'/(common|layer[a-p])/',path)
 if not m:return False
 if m[1]=='common':return True
 return bool(chosen[name]&(1<<(ord(m[1][-1])-ord('a'))))
# Follow actual StageObjInfo instances, not every entry in ZoneList.
queue=['HeavensDoorGalaxy'];seen=[]
while queue:
 n=queue.pop(0)
 if n in seen:continue
 seen.append(n)
 for path,rows in archive_data[n]['tables'].items():
  if '/stageobjinfo' in path and active(n,path):queue.extend(r['name'] for r in rows)
records=[]
for zone in seen:
 for path,rows in archive_data[zone]['tables'].items():
  if not active(zone,path) or not any('/'+cat+'/' in path for cat in ['placement','mapparts','childobj']):continue
  for r in rows:
   name=r.get('type',r.get('name',''));kind='unavailable_factory';ctor=None
   if '/stageobjinfo' in path:kind='zone_holder_metadata'
   elif '/demoobjinfo' in path:kind='demo_loader_metadata';ctor='DemoExecutor' if name=='DemoGroup' else None
   elif zone=='HeavensDoorMysteriousZone' and '/childobj/' in path and r.get('ParentID')==4 and name in ('RunawayRabbit','RunawayTico'):
    kind='original_parent_owned_child';ctor=name+' via RunawayRabbitCollect::init'
   elif '/childobj/' in path:
    kind='child_metadata_unverified_parent';ctor='Original parent must construct this child; not a top-level factory lookup'
   elif name in planets:
    cls=unique.get(name,'PlanetMap');kind='planet_catalog_constructor' if cls in ('PlanetMap','SimpleMapObj') else 'unavailable_unique_planet_constructor';ctor=cls
   elif name in creators:kind='ordinary_factory';ctor=creators[name].strip()
   elif name in areas:kind='ordinary_area_descriptor';ctor=areas[name].strip().rstrip(',')
   records.append({'zone':zone,'zone_id':zoneids[zone],'table':path,'row':r['row'],'l_id':r.get('l_id'),'name':r.get('name'),'creator_identifier':name,'source_status':kind,'original_constructor':ctor,'raw_row':r})
result={'scope':'Read-only source/table comparison; no runtime constructor/link/test outcome asserted. CrystalCageM and StarPiece are published in 066d765bed38f95270b4ea85434d87d0750fd663 with separate passing 120-frame ownership probes and 18000-frame integrated survival; see validation-update.json for the precise runtime scope.',
 'scenario':chosen,'instantiated_zone_names':seen,'uninstantiated_zone_names':[n for n in zoneids if n not in seen],
 'source_sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [factory_path,area_path,planet_path]},
 'archive_provenance':{n:{'json_path':str(files[n]),'archive':archive_data[n]['archive'],'archive_sha256':archive_data[n]['archive_sha256']} for n in seen},
 'status_counts':dict(collections.Counter(r['source_status'] for r in records)),
 'rows':records,'unavailable_rows':[r for r in records if r['source_status'].startswith('unavailable')]}
(base/'source-placement-inventory.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
print(result['status_counts']);print('zones',seen)
for n in seen:
 absent=[r for r in result['unavailable_rows'] if r['zone']==n]
 print(n,len(absent),dict(collections.Counter(r['creator_identifier'] for r in absent)))
