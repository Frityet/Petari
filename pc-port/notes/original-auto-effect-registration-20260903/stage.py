#!/usr/bin/env python3
from pathlib import Path
import shutil,json,hashlib
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-auto-effect-registration-20260903'
headers=['Game/Effect/EffectSystemUtil.hpp','Game/Effect/AutoEffectInfo.hpp','Game/Effect/MultiEmitter.hpp','Game/Effect/AutoEffectGroupHolder.hpp','Game/Effect/AutoEffectGroup.hpp','Game/Effect/EffectSystem.hpp','Game/Effect/ParticleResourceHolder.hpp','Game/LiveActor/EffectKeeper.hpp','Game/Scene/MultiSceneEffectKeeper.hpp','Game/Scene/MultiSceneActor.hpp','Game/Screen/PaneEffectKeeper.hpp']
sources=['Game/Effect/EffectSystemUtil.cpp','Game/Effect/AutoEffectInfo.cpp','Game/Effect/AutoEffectGroup.cpp']
rows=[]
for prefix,paths in [('include',headers),('src',sources)]:
 for path in paths:
  source=ROOT/prefix/path;target=BUILD/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
  rows.append({'source':str(source.relative_to(ROOT)),'staged':str(target.relative_to(ROOT)),'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'root_identical':True})
# Previously documented general Color8 architecture correction, still staged.
source=ROOT/'pc-port/src/Game/Util/Color.hpp';target=BUILD/'staged/Game/Util/Color.hpp';target.parent.mkdir(parents=True,exist_ok=True)
text=source.read_text().replace('        mColor = color;', '        set(static_cast<u8>(color >> 24), static_cast<u8>(color >> 16), static_cast<u8>(color >> 8), static_cast<u8>(color));',1).replace('        return mColor;', '        return (static_cast<u32>(r) << 24) | (static_cast<u32>(g) << 16) | (static_cast<u32>(b) << 8) | a;',1);target.write_text(text)
rows.append({'source':str(source.relative_to(ROOT)),'staged':str(target.relative_to(ROOT)),'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'root_identical':False,'adaptation':'Existing original-auto-effect-20260903/color-native.patch integer RGBA endian correction'})
(NOTES/'native-manifest.json').write_text(json.dumps(rows,indent=2)+'\n')
