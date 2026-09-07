#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, hashlib
R=Path(__file__).resolve().parents[2]; N=Path(__file__).resolve().parent; D=R/'decomp'
OVERLAY=N/'overlay'; h=OVERLAY/'Game/Player/Mario.hpp';h.parent.mkdir(parents=True,exist_ok=True)
h.write_text((D/'include/Game/Player/Mario.hpp').read_text().replace('void createCorrectionMtx(MtxPtr, TVec3f*);','bool createCorrectionMtx(MtxPtr, TVec3f*);'))
s=(N/'Mario.baseline.cpp').read_text();s=s.replace('#include "Game/Player/Mario.hpp"','#include "Game/Player/Mario.hpp"\n#include "Game/LiveActor/HitSensor.hpp"');s+='\n'+(N/'methods.cpp').read_text();p=N/'Mario.candidate.cpp';p.write_text(s)
cmd=json.loads((R/'notes/mario-actor-movement-restoration-20260907/wii-compile-command.json').read_text())['command'].copy();cmd[cmd.index('-c')+1]=str(p);cmd[cmd.index('-o')+1]=str(N/'Mario.candidate.o');k=cmd.index('-i');cmd[k:k]=['-i',str(OVERLAY)]
res=subprocess.run(cmd,cwd=D,capture_output=True,text=True);(N/'compile.log').write_text(res.stdout+res.stderr);(N/'compile-command.json').write_text(json.dumps({'command':cmd,'exit_code':res.returncode},indent=2)+'\n');print('compile',res.returncode)
if res.returncode:print(res.stdout,res.stderr);raise SystemExit(res.returncode)
cmd=['build/tools/objdiff-cli','diff','-1',str(D/'build/original-player-state-recovery-20260907/retail/obj/Game/Player/Mario.o'),'-2',str(N/'Mario.candidate.o'),'-o',str(N/'Mario.candidate.objdiff.json'),'--format','json-pretty'];res=subprocess.run(cmd,cwd=D,capture_output=True,text=True);res.check_returncode();(N/'objdiff-command.json').write_text(json.dumps({'command':cmd,'exit_code':res.returncode},indent=2)+'\n')
x=json.loads((N/'Mario.candidate.objdiff.json').read_text());right={s['name']:s for s in x['right']['symbols']};prefixes=('createDirectionMtx','createCorrectionMtx','createAngleMtx','fixHeadFrontVecByGravity');proof=[]
for s in x['left']['symbols']:
 if s['name'].startswith(prefixes):
  v={'symbol':s['name'],'score':s.get('match_percent'),'retail_bytes':s['size'],'candidate_bytes':right[s['name']]['size']};proof.append(v);print(v)
(N/'function-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
