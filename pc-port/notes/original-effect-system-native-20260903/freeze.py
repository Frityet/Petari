from pathlib import Path
import difflib,hashlib,json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
for tree,prefix,filename in [('native','pc-port/src','native.patch'),('aurora','pc-port/aurora','aurora.patch'),('integration','pc-port/src','integration.patch')]:
 parts=[];manifest=[]
 for p in sorted((N/tree).rglob('*')):
  if not p.is_file():continue
  rel=prefix+'/'+str(p.relative_to(N/tree));base=R/rel
  manifest.append({'path':rel,'base_sha256':sha(base),'final_sha256':sha(p)})
  before=base.read_text().splitlines(keepends=True) if base.exists() else []
  parts.extend(line if line.endswith('\n') else line+'\n\\ No newline at end of file\n' for line in difflib.unified_diff(before,p.read_text().splitlines(keepends=True),fromfile='a/'+rel if base.exists() else '/dev/null',tofile='b/'+rel))
 (N/filename).write_text(''.join(parts));(N/(tree+'-manifest.json')).write_text(json.dumps(manifest,indent=2)+'\n')
 p=subprocess.run(['git','apply','--check',str(N/filename)],cwd=R,capture_output=True,text=True);print(filename,len(manifest),'files','check',p.returncode, p.stderr)
 assert p.returncode==0
 (N/(tree+'-apply-check.log')).write_text('git apply --check: success\n')
cohort=json.loads((N/'cohort.json').read_text());literal=[]
for name in cohort:
 for ext,directory in [('cpp','src'),('hpp','include')]:
  root=R/directory/'Game'/(name+'.'+ext);native=N/'native/Game'/(name+'.'+ext)
  assert root.read_bytes()==native.read_bytes(),root
  literal.append({'original':str(root.relative_to(R)),'native':str(native.relative_to(N)),'sha256':sha(root)})
(N/'literal-root-sources.json').write_text(json.dumps(literal,indent=2)+'\n')
deps=[]
for p in B.glob('*.d'):
 text=p.read_text();assert str(R/'include/') not in text and str(R/'libs/JSystem/include/') not in text,p
 deps.append({'file':p.name,'sha256':sha(p),'root_fallback_references':0})
(N/'native-header-audit.json').write_text(json.dumps(deps,indent=2)+'\n')
manifest=json.loads((N/'native-manifest.json').read_text());general={'pc-port/src/Game/Scene/SceneFunction.cpp','pc-port/src/Game/Util/Color.hpp'}
for name,selected in [('general-native',[x for x in manifest if x['path'] in general]),('effect-cohort',[x for x in manifest if x['path'] not in general])]:
 parts=[]
 for record in selected:
  rel=record['path'];base=R/rel;p=N/'native'/Path(rel).relative_to('pc-port/src');before=base.read_text().splitlines(keepends=True) if base.exists() else []
  parts.extend(line if line.endswith('\n') else line+'\n\\ No newline at end of file\n' for line in difflib.unified_diff(before,p.read_text().splitlines(keepends=True),fromfile='a/'+rel if base.exists() else '/dev/null',tofile='b/'+rel))
 (N/(name+'.patch')).write_text(''.join(parts));(N/(name+'-manifest.json')).write_text(json.dumps(selected,indent=2)+'\n')
 p=subprocess.run(['git','apply','--check',str(N/(name+'.patch'))],cwd=R,capture_output=True,text=True);assert p.returncode==0,p.stderr
 print(name,len(selected),'files','apply check 0')
