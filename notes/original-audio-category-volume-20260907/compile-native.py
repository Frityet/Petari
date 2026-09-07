import json,pathlib,subprocess
r=pathlib.Path.cwd();n=r/'notes/original-audio-category-volume-20260907'
cmd=json.loads((r/'notes/mario-actor-movement-restoration-20260907/native-probe.command.json').read_text())['command']
paths=['aurora/lib/audio.cpp','aurora/lib/j_audio_sound_params.cpp','src/compat/JAudioCategoryVolumeOwnership.cpp','src/compat/OriginalAudioVolumeController.cpp','src/runtime/JAudioPlaybackService.cpp','src/compat/SoundUtilCompat.cpp','tests/OriginalAudioCategoryVolumeTests.cpp']
results=[]
for path in paths:
 c=cmd.copy();o=n/(pathlib.Path(path).stem+'.native.o');c[c.index('-c')+1]=str(r/path);c[c.index('-o')+1]=str(o)
 p=subprocess.run(c,cwd=r,capture_output=True,text=True);(n/(pathlib.Path(path).stem+'.native.log')).write_text(p.stdout+p.stderr)
 results.append({'path':path,'exit':p.returncode,'command':c});print(path,p.returncode,flush=True)
 if p.returncode: print((p.stdout+p.stderr)[-3000:])
(n/'native-proof.json').write_text(json.dumps(results,indent=2)+'\n')
