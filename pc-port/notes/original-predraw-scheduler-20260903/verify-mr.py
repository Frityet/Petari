#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-predraw-scheduler-20260903'
s=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
source=(N/'OriginalPreDrawRegistration-root.cpp').read_text()
assert source[source.index('    void registerPreDrawFunction'):source.rindex('}')] in (R/'src/Game/Util/ObjUtil.cpp').read_text()
src=B/'OriginalPreDrawRegistration.cpp';src.write_text(source)
cmd=h.compiler('cflags_game')+['-c',str(src),'-o',str(B/'OriginalPreDrawRegistration.o')]
p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/'OriginalPreDrawRegistration-compile.log').write_text(p.stdout+p.stderr);p.check_returncode()
retail=R/'build/mario-update-restoration-20260903/retail/obj/Game/Util/ObjUtil.o'
cmd_diff=[str(R/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(B/'OriginalPreDrawRegistration.o'),'-o',str(B/'OriginalPreDrawRegistration.json'),'--format','json-pretty'];subprocess.run(cmd_diff,check=True,capture_output=True)
rows=[{k:row[k] for k in ['name','size','match_percent']} for row in json.loads((B/'OriginalPreDrawRegistration.json').read_text())['left']['symbols'] if row.get('name')=='registerPreDrawFunction__2MRFRCQ22MR11FunctorBasei']
assert rows[0]['match_percent']==100.0
(N/'mr-registration-evidence.json').write_text(json.dumps({'literal_root_source':source,'command':cmd,'comparison':rows},indent=2)+'\n')
print(rows)
