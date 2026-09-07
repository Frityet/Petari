import json,pathlib,subprocess
r=pathlib.Path.cwd();n=r/'notes/original-audio-category-volume-20260907'
old=json.loads((r/'notes/mario-link-closure-20260907/link-command.json').read_text())
args=[a for a in old[old.index('-target'):] if 'keep_mario_dependencies' not in a]
if '-o' in args:
 i=args.index('-o');del args[i:i+2]
objs=[n/(name+'.native.o') for name in ['audio','j_audio_sound_params','JAudioCategoryVolumeOwnership','OriginalAudioVolumeController','JAudioPlaybackService','OriginalAudioCategoryVolumeTests']]
cmd=[old[0]]+list(map(str,objs))+args+['-o',str(n/'original-audio-category-volume-tests')]
p=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(n/'isolated-link.log').write_text(p.stdout+p.stderr)
out={'command':cmd,'link_exit':p.returncode};print('LINK',p.returncode,p.stderr[-4000:])
if p.returncode==0:
 p=subprocess.run([str(n/'original-audio-category-volume-tests')],cwd=r,capture_output=True,text=True);(n/'isolated-run.log').write_text(p.stdout+p.stderr);out['run_exit']=p.returncode;print('RUN',p.returncode,p.stdout,p.stderr)
(n/'isolated-result.json').write_text(json.dumps(out,indent=2)+'\n')
