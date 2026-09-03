from pathlib import Path
import json, subprocess
ROOT=Path(__file__).resolve().parents[3];BUILD=ROOT/'build/original-mario-animator-native-20260903';BUILD.mkdir(parents=True,exist_ok=True);STAGE=BUILD/'staged';INCLUDE=STAGE/'include'
for name in ('MarioAnimator','MarioAnimatorData'):
 p=INCLUDE/'Game/Player'/(name+'.hpp');p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((ROOT/'include/Game/Player'/(name+'.hpp')).read_bytes())
(STAGE/'MarioAnimator.baseline.cpp').write_bytes((ROOT/'src/Game/Player/MarioAnimator.cpp').read_bytes())
entries=json.loads((ROOT/'pc-port/compile_commands.json').read_text())
entry=next(e for e in entries if e['file'].endswith('/XanimePlayer.cpp'))
prefix=[];skip=False
for arg in entry['arguments']:
 if skip:skip=False;continue
 if arg=='-o':skip=True;continue
 if arg not in ('-c',entry['file']):prefix.append(arg)
prefix[1:1]=['-I'+str(INCLUDE),'-I'+str(ROOT/'build/original-model-manager-native-20260903'),'-I'+str(ROOT/'build/original-modelx-native-20260903/staged/include')]
for label in ('MarioAnimator.baseline',):
 cmd=prefix+['-fno-color-diagnostics','-ferror-limit=0','-c',str(STAGE/(label+'.cpp')),'-o',str(BUILD/(label+'.o'))]
 (BUILD/(label+'.command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 proc=subprocess.run(cmd,cwd=entry['directory'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 (BUILD/(label+'.compile.log')).write_text(proc.stdout)
 print(label,proc.returncode);print('\n'.join(x for x in proc.stdout.splitlines() if 'error:' in x))
source=(ROOT/'src/Game/Player/MarioAnimator.cpp').read_text()
source=source.replace('        J3DMtxCalc** jointMtxCalc = (J3DMtxCalc**)((u8*)joint + 0x54);\n        *jointMtxCalc = nullptr;', '        joint->mMtxCalc = nullptr;')
source=source.replace('    Mario* playerVec = getPlayer();\n    Mario* playerAngle', '    {\n    Mario* playerVec = getPlayer();\n    Mario* playerAngle')
source=source.replace('    goto afterRotate;\n\nsetIdentity:', '    goto afterRotate;\n    }\n\nsetIdentity:')
(STAGE/'MarioAnimator.cpp').write_text(source)
cmd=prefix+['-fno-color-diagnostics','-ferror-limit=0','-c',str(STAGE/'MarioAnimator.cpp'),'-o',str(BUILD/'MarioAnimator.o')]
(BUILD/'MarioAnimator.command.json').write_text(json.dumps(cmd,indent=2)+'\n')
proc=subprocess.run(cmd,cwd=entry['directory'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(BUILD/'MarioAnimator.compile.log').write_text(proc.stdout)
print('MarioAnimator',proc.returncode);print('\n'.join(x for x in proc.stdout.splitlines() if 'error:' in x));proc.check_returncode()
# The remaining two Animator methods live in MarioWait.cpp. Extract them
# verbatim for a class-completeness probe, without importing the unrelated
# MarioWait state or masking its existing declaration/definition mismatch.
def function(source, signature):
 start=source.index(signature);brace=source.index('{',start);end=brace+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
wait=(ROOT/'src/Game/Player/MarioWait.cpp').read_text()
wait_source=wait[:wait.index('void MarioAnimator::controlWaitAnimation')]
for name in ('controlWaitAnimation','stopWaitAnimation'):
 wait_source+=function(wait,'void MarioAnimator::'+name)+'\n\n'
(STAGE/'MarioAnimatorWait.cpp').write_text(wait_source)
for label,source in (('MarioAnimationEfx',ROOT/'src/Game/Player/MarioAnimationEfx.cpp'),('MarioAnimatorWait',STAGE/'MarioAnimatorWait.cpp'),('MarioWait',ROOT/'src/Game/Player/MarioWait.cpp')):
 cmd=prefix+['-fno-color-diagnostics','-ferror-limit=0','-c',str(source),'-o',str(BUILD/(label+'.o'))]
 (BUILD/(label+'.command.json')).write_text(json.dumps(cmd,indent=2)+'\n')
 proc=subprocess.run(cmd,cwd=entry['directory'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 (BUILD/(label+'.compile.log')).write_text(proc.stdout)
 print(label,proc.returncode);print('\n'.join(x for x in proc.stdout.splitlines() if 'error:' in x))
 if label!='MarioWait':proc.check_returncode()
