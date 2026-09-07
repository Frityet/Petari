#!/usr/bin/env python3
"""Fresh isolated original/native compile and retail scores for frozen actor closure."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib,json,runpy,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent/'checkpoint'
OUT.mkdir(exist_ok=True)
WII=json.loads((ROOT/'notes/mario-actor-movement-restoration-20260907/wii-compile-command.json').read_text())['command']
NATIVE=json.loads((ROOT/'notes/mario-actor-movement-restoration-20260907/native-probe.command.json').read_text())['command']
COHORT=['Player/MarioActor','Player/MarioActorInit','Player/MarioActorPad','Player/MarioActorParts','Player/MarioActorRush','Player/MarioActorRushMsg','Player/MarioActorBlackHole','Player/MarioActorClap','Player/MarioMove','Player/MarioWalk','Player/MarioJump','MapObj/BlackHole','MapObj/CollectCounter','MapObj/ChipCounter','MapObj/ChipHolder']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def build(job):
 tu,kind=job;source=ROOT/('decomp/src/Game' if kind=='wii' else 'src/Game')/(tu+'.cpp');obj=OUT/(tu.replace('/','-')+'.'+kind+'.o');cmd=list(WII if kind=='wii' else NATIVE);cmd[cmd.index('-c')+1]=str(source);cmd[cmd.index('-o')+1]=str(obj)
 res=subprocess.run(cmd,cwd=ROOT/'decomp' if kind=='wii' else ROOT,capture_output=True,text=True);log=obj.with_suffix('.log');log.write_text(res.stdout+res.stderr)
 row={'tu':tu,'kind':kind,'source':str(source.relative_to(ROOT)),'source_sha256':sha(source),'exit_code':res.returncode,'command':cmd,'log':str(log.relative_to(ROOT))}
 if kind=='wii' and not res.returncode:
  retail=ROOT/'decomp/build/original-player-state-recovery-20260907/retail/obj/Game'/(tu+'.o');diff=obj.with_suffix('.objdiff.json');dc=['build/tools/objdiff-cli','diff','-1',str(retail),'-2',str(obj),'-o',str(diff),'--format','json-pretty'];dr=subprocess.run(dc,cwd=ROOT/'decomp',capture_output=True,text=True);row['objdiff_exit_code']=dr.returncode
  if not dr.returncode:
   d=json.loads(diff.read_text());row['retail_object_sha256']=sha(retail);row['object_sha256']=sha(obj);row['symbols']=[{k:s[k] for k in ['name','size','match_percent'] if k in s} for s in d['left']['symbols'] if '__' in s['name'] and 'match_percent' in s]
 return row
with ThreadPoolExecutor(max_workers=4) as pool:rows=list(pool.map(build,[(x,k) for x in COHORT for k in ['wii','native']]))
(OUT/'compile-and-scores.json').write_text(json.dumps(rows,indent=2)+'\n')
for row in rows:print(row['tu'],row['kind'],row['exit_code'],row.get('objdiff_exit_code',''))
assert all(x['exit_code']==0 and x.get('objdiff_exit_code',0)==0 for x in rows)
