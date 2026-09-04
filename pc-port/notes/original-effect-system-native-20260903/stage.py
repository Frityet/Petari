from pathlib import Path
import shutil,re,json
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903';S=B/'staged'
names=['Effect/EffectSystem','Effect/ParticleEmitter','Effect/ParticleEmitterHolder','Effect/ParticleDrawExecutor','Effect/ParticleCalcExecutor','Effect/AutoEffectGroup','Effect/AutoEffectGroupHolder','Effect/AutoEffectInfo','NameObj/NameObjAdaptor','Effect/SingleEmitter','Effect/EffectSystemUtil']
for name in names:
 for ext,root in [('cpp',R/'src'),('hpp',R/'include')]:
  rel='Game/'+name+'.'+ext;p=S/rel;p.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/rel,p)
# Headers required by these complete classes. Keep already native SDK/utility
# providers; collect missing declared headers as explicit reviewed files below.
for p in (N/'native').rglob('*'):
 if p.is_file():
  out=S/p.relative_to(N/'native');out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,out)
(N/'cohort.json').write_text(json.dumps(names,indent=2)+'\n')

if (N/'aurora/include/functional.hpp').exists():shutil.copyfile(N/'aurora/include/functional.hpp',S/'functional.hpp')
