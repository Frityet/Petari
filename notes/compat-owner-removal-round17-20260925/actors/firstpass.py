from ownership_edits import *
imports={e['destination'] for e in json.loads(Path('notes/compat-owner-removal-round17-20260925/missing-game-source-imports.json').read_text())}
for p in imports:status[p]='new central donor import before this lane'
# Exact compiled originals now supply every method in the former query fragment.
remove('src/compat/OriginalTripodBossQuery.cpp')
for p in ['src/Game/Boss/TripodBossAccesser.hpp','src/Game/Boss/TripodBossAccesser.cpp']:
 s=Path(p).read_text().replace('u32 getTripodBossGravityHostID()', 'uintptr_t getTripodBossGravityHostID()').replace('reinterpret_cast< u32 >(getTripodBossAccesser())','reinterpret_cast< uintptr_t >(getTripodBossAccesser())')
 if p.endswith('.hpp'):s=s.replace('#include "Game/Boss/TripodBoss.hpp"','#include "Game/Boss/TripodBoss.hpp"\n#include <cstdint>')
 write(p,s)
# Restore actual donor reaction helpers missing from the older native class declaration.
p='src/Game/NPC/NPCActor.hpp';s=Path(p).read_text();donor=Path('decomp/include/Game/NPC/NPCActor.hpp').read_text()
a=donor.index('    bool isTrampledStart() const {');b=donor.index('    bool tryPushNullNerve()',a)
s=s.replace('    bool tryPushNullNerve();',donor[a:b]+'    bool tryPushNullNerve();')
a=donor.index('    inline void setDefaults(const char*');b=donor.index('    inline void setDefaultsParam()',a)
s=s.replace('    inline void setDefaultsParam()',donor[a:b]+'    inline void setDefaultsParam()')
a=donor.index('    inline void setTalkAction(const char* pActionName,');b=donor.index('    TalkMessageCtrl* getMsgCtrl()',a)
s=s.replace('    TalkMessageCtrl* getMsgCtrl()',donor[a:b]+'    TalkMessageCtrl* getMsgCtrl()')
s=s.replace('    u8 _5D;', '    bool mUseShadow;  // 0x5D')
write(p,s)
for p in ['src/Game/NPC/NPCActor.cpp','src/Game/NPC/Tico.cpp','src/Game/NPC/Rosetta.cpp']:
 s=Path(p).read_text();s=re.sub(r'\b_5D\b','mUseShadow',s);write(p,s)
for e in entries:
 p=Path(e['path']);old=Path(e['before']).read_text() if e['before'] else '';new=p.read_text() if p.exists() else ''
 dest=base/'patches'/(e['path']+'.patch');dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+e['path'],tofile='b/'+e['path'])))
save()
