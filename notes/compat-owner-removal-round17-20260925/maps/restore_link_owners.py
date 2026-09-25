from pathlib import Path
import json,shutil,re,hashlib,difflib
base=Path('notes/compat-owner-removal-round17-20260925/maps')
manifest=json.loads((base/'owned-manifest.json').read_text());owned={e['path']:e for e in manifest['paths']}
# Reuse only the importer's tokenizer/functions, never execute its import loop.
ns={};exec(Path('notes/compat-owner-removal-round17-20260925/import_missing_game.py').read_text().split('entries=[]')[0],ns)
def write(path,text,reason):
 p=Path(path);old=p.read_text();assert text!=old,path
 if path not in owned:
  b=base/'before'/path;b.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,b)
  e={'path':path,'reason':reason,'initial_import_this_round':False,'lane_patch':'lane-only.patch'};owned[path]=e;manifest['paths'].append(e)
 else:owned[path]['reason']+=' '+reason
 p.write_text(text)
for name in ['MapObj/SpiderThread.cpp','MapObj/SpinDriverPathDrawer.cpp','MapObj/ChipGroup.cpp','MapObj/BlackHole.cpp','AreaObj/MercatorTransformCube.cpp','Map/OceanSphere.cpp']:
 s,count=ns['encode'](Path('decomp/src/Game',name).read_text())
 s=re.sub(r'#include <revolution/gx/[^>]+>','#include <revolution/gx.h>',s)
 if 'MR::Functor' in s and '#include "Game/Util/Functor.hpp"' not in s:
  s='#include "Game/Util/Functor.hpp"\n'+s
 if name=='MapObj/SpinDriverPathDrawer.cpp':
  start=s.index('        u32 result = 0;');end=s.index('        return result;',start)+len('        return result;')
  s=s[:start]+'''        const u32 red = static_cast< u8 >(static_cast< s32 >(255.0f * MR::abs(rColor.x)));
        const u32 green = static_cast< u8 >(static_cast< s32 >(255.0f * MR::abs(rColor.y)));
        const u32 blue = static_cast< u8 >(static_cast< s32 >(255.0f * MR::abs(rColor.z)));
        const u32 alpha = static_cast< u8 >(static_cast< s32 >(255.0f * MR::abs(rAlpha)));
        return (red << 24) | (green << 16) | (blue << 8) | alpha;'''+s[end:]
 write('src/Game/'+name,s,'Restore complete existing donor owner after native fragment left unresolved actual methods; preserve CP932 and SDK include boundaries.')
for name in ['MapObj/SpinDriverPathDrawer.hpp','MapObj/ChipGroup.hpp','MapObj/BlackHole.hpp']:
 write('src/Game/'+name,Path('decomp/include/Game',name).read_text(),'Restore coherent current donor declarations for complete owner source.')
p=Path('src/Game/MapObj/ChipHolder.cpp');write(str(p),p.read_text().replace('mChipGroups[i]->_4C','mChipGroups[i]->mStageSwitchArg'),'Use restored donor ChipGroup field name at existing holder lookup.')
(base/'owned-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Owned paths:',len(manifest['paths']))
