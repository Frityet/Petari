from pathlib import Path
import hashlib, json, subprocess, difflib
ROOT=Path.cwd()
BASE=ROOT/'notes/compat-owner-removal-round21-20260925/audio-owner'
MANIFEST=BASE/'owned-manifest.json'

def write(path, content):
    file=ROOT/path
    manifest=json.loads(MANIFEST.read_text()) if MANIFEST.exists() else {'scope':'Actual AudSystemWrapper/name converter/scene manager/nonowning sound holder; remove eight audio compat files','files':[]}
    entry=next((e for e in manifest['files'] if e['path']==path),None)
    if entry is None:
        before=BASE/'before'/path
        before.parent.mkdir(parents=True,exist_ok=True)
        before.write_bytes(file.read_bytes())
        gitroot=Path(subprocess.check_output(['git','-C',str(file.parent),'rev-parse','--show-toplevel'],text=True).strip())
        status=subprocess.check_output(['git','-C',str(gitroot),'status','--short','--',str(file.relative_to(gitroot))],text=True).strip()
        entry={'path':path,'before':str(before.relative_to(ROOT)),'before_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'initial_git_status':status or 'clean','git_root':str(gitroot.relative_to(ROOT))}
        manifest['files'].append(entry)
    if content is None:
        file.unlink()
        entry.update(action='delete',after_sha256=None)
    else:
        file.write_text(content)
        entry.update(action='modify',after_sha256=hashlib.sha256(file.read_bytes()).hexdigest())
    MANIFEST.write_text(json.dumps(manifest,indent=2)+'\n')
    before=ROOT/entry['before']
    patch=BASE/'patches'/(path+'.patch')
    patch.parent.mkdir(parents=True,exist_ok=True)
    patch.write_text(''.join(difflib.unified_diff(before.read_text().splitlines(True),(content or '').splitlines(True),fromfile='a/'+path,tofile=('b/'+path if content is not None else '/dev/null'))))
