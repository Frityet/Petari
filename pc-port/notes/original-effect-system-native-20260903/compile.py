from pathlib import Path
import json,subprocess,concurrent.futures
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903';S=B/'staged'
base=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
base=[x.replace(str(R/'build/original-jpa-resource-loader-20260903/staged'),str(S)) for x in base]
base=[x for x in base if x not in ['-I'+str(R/'pc-port/../include'),'-I'+str(R/'pc-port/../libs/JSystem/include')]]
def compile(name):
 src=S/'Game'/(name+'.cpp');out=B/(src.stem+'.o');cmd=base+['-fsanitize=address,undefined','-fno-omit-frame-pointer','-MMD','-MF',str(B/(src.stem+'.d')),'-c',str(src),'-o',str(out)]
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/(src.stem+'-compile.log')).write_text(p.stdout+p.stderr)
 return {'source':str(src),'command':cmd,'returncode':p.returncode,'errors':[line for line in p.stderr.splitlines() if 'error:' in line]}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:rows=list(pool.map(compile,json.loads((N/'cohort.json').read_text())+['Scene/SceneFunction']))
(N/'native-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')
for row in rows:print(Path(row['source']).name,row['returncode'],*row['errors'],sep='\n')
