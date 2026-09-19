"""Compile only the focused probe; link pre-existing archives without Xmake builds."""
import hashlib,json,pathlib,shlex,subprocess,sys
root=pathlib.Path(__file__).resolve().parents[2];here=pathlib.Path(__file__).resolve().parent
phase=sys.argv[1] if len(sys.argv)>1 else 'baseline'
source=root/'tests/OriginalActorUtilityTests.cpp'; obj=here/(phase+'.o');binary=here/(phase+'-probe')
entry=next(e for e in json.loads((root/'compile_commands.json').read_text()) if e['file']=='tests/GravityRealOrAbsentTests.cpp')
args=entry['arguments'].copy();args[args.index('-o')+1]=str(obj);args[-1]=str(source);args+=['-Itests']
def run(args,name):
 p=subprocess.run(args,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 (here/(name+'.log')).write_text(p.stdout);print(name,p.returncode,p.stdout[-1500:]);return p.returncode
lua='import("core.project.project"); import("core.project.config"); config.load(); local t = project.target("smg-pc-gravity-real-or-absent-tests"); '
link=shlex.split(subprocess.check_output(['xmake','lua','-c',lua+'print(t:linkcmd())'],cwd=root,text=True).strip())
libs=subprocess.check_output(['xmake','lua','-c',lua+'for _, d in ipairs(t:orderdeps()) do if d:is_static() then print(d:targetfile()) end end'],cwd=root,text=True).splitlines()
# The baseline Aurora archive still uses its recorded Abseil package even if
# the working build graph has since removed that package.
recorded=json.loads((root/'compile_commands.json').read_text())
cp=next(e for e in recorded if e['file'].endswith('command_processor.cpp'))
absl=next(pathlib.Path(a).parent/'lib' for a in cp['arguments'] if '/abseil/' in a)
link += [str(p) for p in sorted(absl.glob('*.a'))]
link[link.index('-o')+1]=str(binary)
link=[a for a in link if not a.endswith('.cpp.o')];link[3:3]=[str(obj),'build/.objs/smg-pc-gravity-real-or-absent-tests/macosx/arm64/debug/aurora/lib/compat.cpp.o']+list(reversed(libs))
record={'scope':'isolated focused test object/link against existing archives; no shared build','compile':args,'link':link,'archives':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in libs}}
record['compile_exit']=run(args,phase+'-compile')
if record['compile_exit']==0:
 record['link_exit']=run(link,phase+'-link')
 if record['link_exit']==0:record['probe_exit']=run([str(binary)],phase+'-run')
(here/(phase+'.json')).write_text(json.dumps(record,indent=2)+'\n')
