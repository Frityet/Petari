#!/usr/bin/env python3
from pathlib import Path
import difflib,hashlib,json,re,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/light-name-text-boundary-20260903';S=B/'staged';patch='';records=[]
for source in sorted(S.rglob('*')):
 if not source.is_file():continue
 rel=source.relative_to(S);dest=R/'pc-port'/rel;old=dest.read_text() if dest.exists() else '';new=source.read_text();patch+=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+str(dest.relative_to(R)) if dest.exists() else '/dev/null',tofile='b/'+str(dest.relative_to(R))))
 records.append(dict(staged=str(source.relative_to(R)),destination=str(dest.relative_to(R)),sha256=hashlib.sha256(source.read_bytes()).hexdigest(),production_before_sha256=hashlib.sha256(dest.read_bytes()).hexdigest() if dest.exists() else None))
assert len(records)==4
assert all('/Game/' not in r['destination'] for r in records)
(N/'native.patch').write_text(patch);(N/'native-manifest.json').write_text(json.dumps(records,indent=2)+'\n')
search='mAreaLightName|default_area_light_name|getDefaultAreaLightName|findAreaLight';r=subprocess.run(['rg','-n',search,'pc-port/src','--glob','*.cpp','--glob','*.hpp'],cwd=R,capture_output=True,text=True,check=True)
uses=[]
for line in r.stdout.splitlines():
 path,line_no,code=line.split(':',2);uses.append(dict(path=path,line=int(line_no),code=code))
(N/'consumer-audit.json').write_text(json.dumps(dict(search=search,scope='current native source tree; root original providers read separately',uses=uses,no_current_runtime_name_presentation_sink=True,raw_game_fields=['AreaLightInfo::mAreaLightName','ZoneAreaLight::area_light_name','StageLightData::_default_stage_area_light_name'],host_presentation_decoder='smgpc::resource::decode_cp932'),indent=2)+'\n')
subprocess.run(['git','apply','--check',str(N/'native.patch')],cwd=R,check=True);print('Frozen four-file text boundary patch, manifest, consumer audit.')
