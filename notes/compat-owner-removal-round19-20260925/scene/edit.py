from pathlib import Path
import hashlib, json, subprocess, difflib
ROOT=Path.cwd()
BASE=ROOT/'notes/compat-owner-removal-round19-20260925/scene'
MANIFEST=BASE/'owned-manifest.json'

def write(path, content):
    file=ROOT/path
    manifest=json.loads(MANIFEST.read_text()) if MANIFEST.exists() else {'scope':'Actual Scene lifetime and execution ownership; Scene.hpp/.cpp only','files':[]}
    entry=next((e for e in manifest['files'] if e['path']==path),None)
    if entry is None:
        before=BASE/'before'/path
        before.parent.mkdir(parents=True,exist_ok=True)
        before.write_bytes(file.read_bytes())
        entry={'path':path,'before':str(before.relative_to(ROOT)),'before_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'initial_git_status':subprocess.check_output(['git','status','--short','--',path],text=True).strip() or 'clean','action':'modify'}
        manifest['files'].append(entry)
    file.write_text(content)
    entry['after_sha256']=hashlib.sha256(file.read_bytes()).hexdigest()
    MANIFEST.write_text(json.dumps(manifest,indent=2)+'\n')
    before=ROOT/entry['before']
    patch=BASE/'patches'/(path+'.patch')
    patch.parent.mkdir(parents=True,exist_ok=True)
    patch.write_text(''.join(difflib.unified_diff(before.read_text().splitlines(True),content.splitlines(True),fromfile='a/'+path,tofile='b/'+path)))
