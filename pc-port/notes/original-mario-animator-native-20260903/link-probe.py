"""Link every compiled Animator method without running or constructing fake owners."""
from pathlib import Path
import json,re,subprocess,hashlib
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
BUILD=ROOT/'build/original-mario-animator-native-20260903'
base=json.loads((BUILD/'MarioAnimator.command.json').read_text());prefix=base[:base.index('-fno-color-diagnostics')]
(BUILD/'layout-native.cpp').write_text('#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"\n#include <stddef.h>\nstatic_assert(offsetof(J3DJoint,mMtxCalc)==0x68);\nint main(){return 0;}\n')
cmd=prefix+['-c',str(BUILD/'layout-native.cpp'),'-o',str(BUILD/'layout-native.o')]
proc=subprocess.run(cmd,cwd=ROOT/'pc-port',text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(BUILD/'layout-native.log').write_text(proc.stdout);proc.check_returncode()
data=(ROOT/'pc-port/build/.deps/smg-pc-showcase/macosx/arm64/debug/smg-pc-showcase.d').read_text()
flags=re.findall(r'"((?:\\.|[^"\\])*)"',re.search(r'values = \{(.*?)\n    \}',data,re.S)[1]);flags=[x for x in flags if x!='-Wl,-dead_strip']
objects=[BUILD/(name+'.o') for name in ('layout-native','MarioAnimator','MarioAnimationEfx','MarioAnimatorWait')]
cmd=[flags[0],*map(str,objects),str(ROOT/'pc-port/build/.objs/smg-pc-showcase/macosx/arm64/debug/aurora/lib/compat.cpp.o'),*flags[1:],'-o',str(BUILD/'whole-class-link-probe')]
proc=subprocess.run(cmd,cwd=ROOT/'pc-port',text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(BUILD/'complete-class.link.command.json').write_text(json.dumps(cmd,indent=2)+'\n');(BUILD/'complete-class.link.log').write_text(proc.stdout)
all_names=re.findall(r'^  "(.*?)", referenced from:',proc.stdout,re.M)
classes=[]
for block in re.findall(r'^  (".*?", referenced from:\n(?:.*\n)*?)(?=^  "|^ld:)',proc.stdout,re.M):
 lines=block.splitlines();callers=[l.strip() for l in lines[1:] if any(' in '+unit+'.o' in l for unit in ('MarioAnimator','MarioAnimationEfx','MarioAnimatorWait'))]
 if callers:classes.append({'symbol':lines[0].split('", referenced')[0].strip('"'),'callers':callers})
# No class method is absent after compiling its full TU, complete callback TU,
# and the exact two methods originally placed in MarioWait.cpp.
assert not any(n.startswith('MarioAnimator::') for n in all_names)
report={'scope':'All MarioAnimator methods retained at link, with complete original callback/Luigi table TU and exact original Animator Wait methods. No runtime execution. The link still requires real Game owners and providers.', 'link_exit':proc.returncode,'all_unresolved_count':len(all_names),'direct_class_dependency_count':len(classes),'direct_class_dependencies':classes,'all_unresolved':all_names,'command':cmd,'native_mMtxCalc_offset':104,'object_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in objects}}
(HERE/'complete-class-link-frontier.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Complete class retained: no unresolved MarioAnimator methods; {len(classes)} direct dependencies remain, {len(all_names)} including pre-existing transitive whole-object gaps')
