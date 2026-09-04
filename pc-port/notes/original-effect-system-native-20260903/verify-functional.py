from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903'
cmd=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-DTARGET_PC','-I'+str(N/'aurora/include'),'-I'+str(R/'pc-port/aurora/include'),'-fsanitize=address,undefined','-fno-omit-frame-pointer',str(N/'functional-probe.cpp'),'-o',str(B/'functional-probe')]
p=subprocess.run(cmd,cwd=R,text=True,capture_output=True);(N/'functional-compile.log').write_text(p.stdout+p.stderr);(N/'functional-compile.json').write_text(json.dumps({'command':cmd,'returncode':p.returncode},indent=2)+'\n')
if p.returncode:print(p.stderr);raise SystemExit(p.returncode)
p=subprocess.run([str(B/'functional-probe')],cwd=R,text=True,capture_output=True);(N/'functional-runtime.log').write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);raise SystemExit(p.returncode)
