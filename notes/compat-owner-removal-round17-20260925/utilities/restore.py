from pathlib import Path
import re,json,hashlib,difflib,subprocess
base=Path('notes/compat-owner-removal-round17-20260925/utilities')
manifest=[]
requests={
 'ActorMovementUtil.cpp':['bool faceToVector(MtxPtr','bool makeMtxOnMapCollision(','bool calcVelocityRailMoveOnGround('],
 'LayoutUtil.cpp':['u8 getPaneAlpha(','void setLayoutAlpha(','void setPaneAlpha(','void copyLayoutDrawInfoWithAspect('],
 'MessageUtil.cpp':['const wchar_t* getMessageLine('],
}
for name,prefixes in requests.items():
 path=Path('src/Game/Util')/name;donorpath=Path('decomp/src/Game/Util')/name
 before=path.read_text();donor=donorpath.read_text();after=before;blocks=[]
 dest=base/'before'/path;dest.parent.mkdir(parents=True,exist_ok=True);assert not dest.exists();dest.write_text(before)
 for prefix in prefixes:
  start=donor.index('    '+prefix);end=donor.index('\n    }',start)+len('\n    }');body=donor[start:end]
  assert ('    '+prefix) not in before, (name,prefix)
  blocks.append(body)
 if name=='MessageUtil.cpp':
  assert '    // getMessageLine\n' in after
  after=after.replace('    // getMessageLine\n',blocks[0]+'\n\n')
 else:
  pos=after.index('namespace MR {')+len('namespace MR {')
  after=after[:pos]+'\n'+ '\n\n'.join(blocks)+'\n'+after[pos:]
 status=subprocess.check_output(['git','status','--short','--',str(path)],text=True).strip()
 path.write_text(after)
 patch=base/'patches'/(str(path)+'.patch');patch.parent.mkdir(parents=True,exist_ok=True);patch.write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+str(path),tofile='b/'+str(path))))
 manifest.append(dict(path=str(path),action='modify',initial_git_status=status or 'clean',before=str(dest),before_sha256=hashlib.sha256(before.encode()).hexdigest(),after_sha256=hashlib.sha256(after.encode()).hexdigest(),donor=str(donorpath),donor_bodies=prefixes))
(base/'owned-manifest.json').write_text(json.dumps({'scope':'Eight missing exact donor utility bodies, existing native implementations retained','files':manifest},indent=2)+'\n')
