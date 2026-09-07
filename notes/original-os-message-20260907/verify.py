from pathlib import Path
import subprocess,json,hashlib
r=Path.cwd();o=r/'notes/original-os-message-20260907';g=r/'decomp/build/aurora-upstream-merge-tests'
assert (r/'aurora/lib/dolphin/os/OSMessage.cpp').read_bytes()==(r/'decomp/src/RVL_SDK/os/OSMessage.c').read_bytes()
results=[]
for label,flags in [('native',[]),('tsan',['-fsanitize=thread'])]:
 cmd=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++20','-g',*flags,'-DAURORA','-DTARGET_PC','-Iaurora/include',f'-I{g}/_deps/googletest-src/googletest/include',*[f'aurora/lib/dolphin/os/{n}.cpp'for n in ['OSExecution','OSMutex','OSMessage']],*[f'aurora/tests/{n}_test.cpp'for n in ['os_execution','os_mutex','os_message']],str(g/'lib/libgtest_main.a'),str(g/'lib/libgtest.a'),'-pthread','-o',str(o/label)]
 p=subprocess.run(cmd,capture_output=True,text=True);(o/f'{label}-build.log').write_text(p.stdout+p.stderr);res={'command':cmd,'build_exit':p.returncode}
 if not p.returncode:
  p=subprocess.run([str(o/label),f'--gtest_output=xml:{o}/{label}.xml'],capture_output=True,text=True,timeout=60);(o/f'{label}-run.log').write_text(p.stdout+p.stderr);res['run_exit']=p.returncode
 results.append(res);print(label,res['build_exit'],res.get('run_exit'),flush=True)
(o/'native-results.json').write_text(json.dumps(results,indent=2)+'\n')
