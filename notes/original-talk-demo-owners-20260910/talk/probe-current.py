from pathlib import Path
import json,subprocess,concurrent.futures,re,hashlib
root=Path.cwd();out=root/'notes/original-talk-demo-owners-20260910/talk';overlay=out/'headers';overlay.mkdir(exist_ok=True)
sources=['NPC/TalkDirector.cpp','NPC/TalkBalloon.cpp','NPC/TalkState.cpp','NPC/TalkTextFormer.cpp','NPC/TalkSupportPlayerWatcher.cpp','NPC/TalkMessageCtrl.cpp','NPC/TalkNodeCtrl.cpp','System/MessageHolder.cpp','System/DrawSyncManager.cpp']
cmd=json.load(open('notes/original-input-owner-20260910/rumble/native-compile.json'))['command'];cmd=cmd[:cmd.index('-o')];cmd.remove('-c');cmd+=['-fsyntax-only','-ferror-limit=16','-I'+str(overlay)]
# Native headers remain first. Only missing original Game declarations are staged.
rows=[];imported=set()
for iteration in range(12):
 def probe(source):
  source_path=root/'decomp/src/Game'/source;args=cmd+[str(source_path)];p=subprocess.run(args,text=True,capture_output=True);return {'source':str(source_path.relative_to(root)),'exit_code':p.returncode,'command':args,'output':p.stdout+p.stderr,'sha256':hashlib.sha256(source_path.read_bytes()).hexdigest()}
 with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool: rows=list(pool.map(probe,sources))
 missing=set()
 for row in rows:
  for header in re.findall(r"fatal error: '([^']+)' file not found",row['output']):
   original=root/'decomp/include'/header
   if header.startswith('Game/') and original.exists(): missing.add(header)
 if not missing-imported:break
 for header in missing-imported:
  dst=overlay/header;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes((root/'decomp/include'/header).read_bytes())
 imported.update(missing)
for row in rows:
 p=Path(row['source']);(out/(p.stem+'.current-native.log')).write_text(row.pop('output'))
(out/'current-native-probes.json').write_text(json.dumps({'probes':rows,'original_headers_staged':sorted(imported),'iterations':iteration+1},indent=2)+'\n')
for row in rows:
 log=(out/(Path(row['source']).stem+'.current-native.log')).read_text();print(row['source'],row['exit_code']);print('\n'.join(x for x in log.splitlines() if 'error:' in x)[:2500])
