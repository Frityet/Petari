#!/usr/bin/env python3
from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-arithmetic-20260903';S=B/'staged'
result=[]
cmd=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++23','-O2','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-ffp-contract=off','-I'+str(S),str(N/'arithmetic-probe.cpp'),'-o',str(B/'arithmetic-probe')]
p=subprocess.run(cmd,capture_output=True,text=True);result.append({'command':cmd,'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr});p.check_returncode()
cmd=[str(B/'arithmetic-probe'),str(B/'oracle-cases.txt')];p=subprocess.run(cmd,capture_output=True,text=True);result.append({'command':cmd,'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr});print(p.stdout,p.stderr);p.check_returncode();assert not p.stderr
cmd=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
cmd=[a.replace(str(R/'build/original-jpa-resource-loader-20260903/staged'),str(S)) for a in cmd]
for name in ['J3DTransformAnimationCompat','J3DMaterialAnimationCompat','J3DAdditionalAnimationCompat']:
 source=R/'pc-port/src/compat'/(name+'.cpp');c=cmd+['-c',str(source),'-o',str(B/(name+'.o'))];p=subprocess.run(c,cwd=R/'pc-port',capture_output=True,text=True);(B/(name+'-compile.log')).write_text(p.stdout+p.stderr);result.append({'command':c,'returncode':p.returncode});print(name,p.returncode);p.check_returncode()
(N/'arithmetic-verification.json').write_text(json.dumps(result,indent=2)+'\n')
