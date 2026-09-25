from pathlib import Path
import json,hashlib,re,difflib
base=Path('notes/compat-owner-removal-round17-20260925/actors')
manifest=base/'owned-manifest.json'
entries=json.loads(manifest.read_text())['files'] if manifest.exists() else []
status={s[3:]:s[:2] for s in Path('notes/compat-owner-removal-round17-20260925/before-status.txt').read_text().splitlines() if len(s)>3}
def snap(path):
 p=Path(path)
 if not any(e['path']==path for e in entries):
  dest=base/'before'/path
  if p.exists(): dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(p.read_bytes())
  entries.append(dict(path=path,before=str(dest) if p.exists() else None,before_sha256=hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None,initial_git_status=status.get(path,'clean (parent round17 baseline)')))
  save()
def save():
 for e in entries:
  p=Path(e['path']);e['action']='modify' if p.exists() and e['before'] else ('add' if p.exists() else 'delete');e['after_sha256']=hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
 manifest.write_text(json.dumps({'scope':'Boss Enemy NPC donor portability and exact provider consolidation','files':entries},indent=2)+'\n')
def write(path,text):snap(path);Path(path).write_text(text);save()
def remove(path):snap(path);Path(path).unlink();save()
