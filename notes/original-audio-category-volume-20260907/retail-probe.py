import pathlib,json,subprocess,os,hashlib
r=pathlib.Path.cwd();n=r/'notes/original-audio-category-volume-20260907'
base=json.loads((r/'notes/mario-actor-movement-restoration-20260907/native-probe.command.json').read_text())['command']
objects=[];results=[]
for path in ['aurora/lib/j_audio_sound_archive.cpp','aurora/lib/j_audio_stream.cpp','tests/JAudioPlaybackTests.cpp','notes/original-audio-category-volume-20260907/extract-audio.cpp']:
 cmd=base.copy();o=n/(pathlib.Path(path).stem+'.native.o');cmd[cmd.index('-c')+1]=str(r/path);cmd[cmd.index('-o')+1]=str(o)
 p=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(n/(pathlib.Path(path).stem+'.native.log')).write_text(p.stdout+p.stderr);results.append({'source':path,'compile_exit':p.returncode,'command':cmd});print(path,p.returncode,flush=True)
 if p.returncode:print(p.stderr[-3000:]);raise SystemExit(1)
 objects.append(str(o))
old=json.loads((n/'isolated-result.json').read_text())['command'];args=old[old.index('-target'):];i=args.index('-o');del args[i:i+2]
for name,obj in [('extract-audio',objects[-1]),('retail-playback-probe',objects[-2])]:
 fresh=[str(n/(x+'.native.o'))for x in ['audio','j_audio_sound_params','JAudioCategoryVolumeOwnership','OriginalAudioVolumeController','JAudioPlaybackService']]
 cmd=[base[0],obj]+objects[:2]+fresh+args+['-o',str(n/name)]
 p=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(n/(name+'-link.log')).write_text(p.stdout+p.stderr);results.append({'name':name,'link_exit':p.returncode,'command':cmd});print(name,'LINK',p.returncode,flush=True)
 if p.returncode:print(p.stderr[-4000:]);raise SystemExit(1)
p=subprocess.run([str(n/'extract-audio'),str(r/'Super Mario Wii - Galaxy Adventure (Korea).rvz'),str(n/'fixture')],cwd=r,capture_output=True,text=True);(n/'extract-audio-run.log').write_text(p.stdout+p.stderr);results.append({'extract_exit':p.returncode});print('EXTRACT',p.returncode,p.stdout[-2000:],p.stderr[-1000:],flush=True)
manifest=[{'path':str(p.relative_to(n/'fixture')),'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}for p in sorted((n/'fixture').rglob('*')) if p.is_file()]
(n/'fixture-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
if p.returncode==0:
 env=os.environ.copy();env['SMGPC_RETAIL_FILES_ROOT']=str(n/'fixture')
 p=subprocess.run([str(n/'retail-playback-probe'),'--playback-service-probe'],cwd=r,env=env,capture_output=True,text=True);(n/'retail-playback-run.log').write_text(p.stdout+p.stderr);results.append({'retail_run_exit':p.returncode});print('RETAIL',p.returncode,p.stdout[-2000:],p.stderr[-2000:])
(n/'retail-result.json').write_text(json.dumps(results,indent=2)+'\n')
