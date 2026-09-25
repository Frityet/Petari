from pathlib import Path
import json,hashlib,difflib,subprocess
base=Path('notes/compat-owner-removal-round18-20260925/executor')
manifest=base/'owned-manifest.json'
entries=json.loads(manifest.read_text())['files'] if manifest.exists() else []
def write(path,after):
 p=Path(path)
 if not any(e['path']==path for e in entries):
  before=p.read_text();dest=base/'before'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(before)
  status=subprocess.check_output(['git','status','--short','--',path],text=True).strip()
  entries.append(dict(path=path,before=str(dest),before_sha256=hashlib.sha256(before.encode()).hexdigest(),initial_git_status=status or 'clean',action='modify'))
 p.write_text(after);save()
def save():
 for e in entries:
  p=Path(e['path']);e['after_sha256']=hashlib.sha256(p.read_bytes()).hexdigest()
  patch=base/'patches'/(e['path']+'.patch');patch.parent.mkdir(parents=True,exist_ok=True)
  patch.write_text(''.join(difflib.unified_diff(Path(e['before']).read_text().splitlines(True),p.read_text().splitlines(True),fromfile='a/'+e['path'],tofile='b/'+e['path'])))
 manifest.write_text(json.dumps({'scope':'Actual category callback ownership and executor lifecycle; four files only','files':entries},indent=2)+'\n')
