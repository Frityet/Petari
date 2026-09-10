from pathlib import Path
import json,subprocess,concurrent.futures
root=Path.cwd();note=root/'notes/original-demo-director-20260910/talk-nodes';overlay=note/'headers';overlay.mkdir(exist_ok=True)
headers=['ut/TagProcessorBase.h','ut/TextWriterBase.h','ut/CharWriter.h','ut/Rect.h','ut/Color.h','ut/CharStrmReader.h','math/constant.h']
for h in headers:
 p=overlay/'nw4r'/h;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((root/'decomp/libs/nw4r/include/nw4r'/h).read_bytes())
p=overlay/'Game/Screen/MessageTagSkipTagProcessor.hpp';p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((root/'decomp/include/Game/Screen/MessageTagSkipTagProcessor.hpp').read_bytes())
a=json.loads((root/'notes/original-input-owner-20260910/rumble/native-compile.json').read_text())['command'];a=a[:a.index('-o')];a.remove('-c');a.insert(1,'-I'+str(overlay));a+=['-fsyntax-only','-ferror-limit=20']
srcs=['decomp/src/nw4r/ut/ut_TagProcessorBase.cpp','decomp/src/nw4r/ut/ut_TextWriterBase.cpp','decomp/src/nw4r/ut/ut_CharWriter.cpp','decomp/src/nw4r/ut/ut_CharStrmReader.cpp','decomp/src/nw4r/ut/ut_Font.cpp','decomp/src/Game/NPC/TalkNodeCtrl.cpp','decomp/src/Game/Screen/MessageTagSkipTagProcessor.cpp']
rows=[]
def run(s):
 r=subprocess.run(a+[s],capture_output=True,text=True);(note/(Path(s).stem+'.probe.log')).write_text(r.stdout+r.stderr);return {'source':s,'command':a+[s],'exit':r.returncode,'errors':[x for x in r.stderr.splitlines() if 'error:' in x]}
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:rows=list(pool.map(run,srcs))
(note/'probe.json').write_text(json.dumps(rows,indent=2)+'\n')
for r in rows:print(r['source'],r['exit'],'\n'+'\n'.join(r['errors']))
