from pathlib import Path
import json,shutil,subprocess,hashlib,difflib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-auto-effect-20260903';S=B/'staged'
owned=['src/Game/Effect/AutoEffectGroup.cpp','src/Game/Effect/AutoEffectInfo.cpp','include/Game/Effect/AutoEffectInfo.hpp','libs/MSL_C/include/cstdlib']
headers=['include/Game/Effect/AutoEffectGroup.hpp']
manifest=[];patch=[]
for name in owned+headers:
 src=R/name;relative=Path(*Path(name).parts[1:]);dst=S/relative;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
 manifest.append({'source':name,'staged':str(dst.relative_to(R)),'owned':name in owned,'sha256':hashlib.sha256(src.read_bytes()).hexdigest()})
 if name in owned:
  before=subprocess.run(['git','show','HEAD:'+name],cwd=R,capture_output=True,text=True);old=before.stdout if before.returncode==0 else '';new=src.read_text();patch.append(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/'+name if before.returncode==0 else '/dev/null','b/'+name)))
  out=N/'root'/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,out)
(N/'root.patch').write_text(''.join(patch));(N/'native-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
# Color8's integer API represents 0xRRGGBBAA independently of host byte order.
color=(R/'pc-port/src/Game/Util/Color.hpp').read_text().replace('        mColor = color;', '        set(static_cast<u8>(color >> 24), static_cast<u8>(color >> 16), static_cast<u8>(color >> 8), static_cast<u8>(color));').replace('        return mColor;', '        return (static_cast<u32>(r) << 24) | (static_cast<u32>(g) << 16) | (static_cast<u32>(b) << 8) | a;')
p=S/'Game/Util/Color.hpp';p.parent.mkdir(parents=True,exist_ok=True);p.write_text(color)
(N/'color-native.patch').write_text(''.join(difflib.unified_diff((R/'pc-port/src/Game/Util/Color.hpp').read_text().splitlines(True),color.splitlines(True),'a/pc-port/src/Game/Util/Color.hpp','b/pc-port/src/Game/Util/Color.hpp')))
cmd=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
cmd=[a.replace(str(R/'build/original-jpa-resource-loader-20260903/staged'),str(S)) for a in cmd]
rows=[]
for name in ['AutoEffectGroup','AutoEffectInfo']:
 src=S/'Game/Effect'/(name+'.cpp');c=cmd+['-c',str(src),'-o',str(B/(name+'-native.o'))];p=subprocess.run(c,cwd=R/'pc-port',capture_output=True,text=True);(B/(name+'-native.log')).write_text(p.stdout+p.stderr);rows.append({'command':c,'returncode':p.returncode});print(name,p.returncode)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 c=['/opt/homebrew/opt/llvm/bin/llvm-nm','-u','--demangle',str(B/(name+'-native.o'))];p=subprocess.run(c,capture_output=True,text=True);(N/(name+'-undefined.txt')).write_text(p.stdout)
(N/'native-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')
