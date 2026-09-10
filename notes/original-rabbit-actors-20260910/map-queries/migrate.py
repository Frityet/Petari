from pathlib import Path
import json,hashlib,re
root=Path.cwd();out=root/'notes/original-rabbit-actors-20260910/map-queries'
ref=(root/'decomp/src/Game/Util/MapUtil.cpp').read_text()
def functions(text,name,start=0,end=None):
 pattern=re.compile(r'^    (?:\[\[nodiscard\]\] )?(?:[\w:<>*&]+(?:\s+|\s*\*\s*))+?'+re.escape(name)+r'\(',re.M)
 rows=[]
 for m in pattern.finditer(text,start,len(text) if end is None else end):
  p=text.find('{',m.start());depth=1;q=p+1
  while depth:
   if text[q]=='{':depth+=1
   if text[q]=='}':depth-=1
   q+=1
  rows.append((m.start(),q,text[m.start():q]))
 return rows
mr_begin=ref.index('namespace MR {');col_begin=ref.index('namespace Collision {')
anon_names=['getStrikeInfoNumCategory','getFirstPolyOnLineCategory']
mr_names=['getFirstPolyOnLineToMap','getFirstPolyOnLineToWaterSurface','getNearPolyOnLineSort','getSortedPoly','isExistMapCollision','isExistMoveLimitCollision','isExistMapCollisionExceptActor','trySetMoveLimitCollision']
col_names=['checkStrikeLineToMap','checkStrikeLineToSunshade','getStrikeInfoMap','getStrikeInfoNumMap']
records=[]
def collect(names,start,end):
 result=[]
 for name in names:
  rows=functions(ref,name,start,end);assert rows,name
  for a,b,body in rows:
   result.append((a,body));records.append({'name':name,'reference_line':ref[:a].count('\n')+1,'body_sha256':hashlib.sha256(body.encode()).hexdigest()})
 return '\n\n'.join(body for _,body in sorted(result))
head='''// Original MapUtil entry points using the actual scene CollisionDirector.
#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/TriangleFilter.hpp"

static HitInfo mSortBuffer[32];
static u32 mSortCount;

'''
new=head+'namespace {\n'+collect(anon_names,0,mr_begin)+'\n}\n\nnamespace MR {\n'+collect(mr_names,mr_begin,col_begin)+'\n}\n\nnamespace Collision {\n'+collect(col_names,col_begin,len(ref))+'\n}\n'
compat=(root/'src/compat/GameMapCollisionCompat.cpp').read_text()
remove_mr=set(mr_names)-{'trySetMoveLimitCollision','isExistMoveLimitCollision'}
col_start=compat.index('namespace Collision {')
spans=[]
for name in remove_mr:
 rows=functions(compat,name,0,col_start);assert rows,name
 spans.extend(rows)
for name in col_names:
 rows=functions(compat,name,col_start);assert rows,name
 spans.extend(rows)
for name in ['first_line_hit','strike_infos','sorted_line_hits','store_line_hits']:
 rows=functions(compat,name);assert len(rows)==1,(name,len(rows))
 if name=='first_line_hit':
  a,b,body=rows[0];p=body.index('{');replacement=body[:p]+'''{
        return MR::getFirstPolyOnLineToMap(position, triangle, start, offset, parts_filter, triangle_filter);
    }''';spans.append((a,b,body,replacement))
 else:spans.extend(rows)
# Remove the now-obsolete native sorted snapshot, including its semicolon.
a=compat.index('    struct SortedLineHits {');b=compat.index('    };',a)+len('    };');spans.append((a,b,compat[a:b]))
for row in sorted(spans,key=lambda r:r[0],reverse=True):
 a,b,body=row[:3];assert compat[a:b]==body
 compat=compat[:a]+(row[3] if len(row)>3 else '')+compat[b:]
old='''        auto& infos = strike_infos();
        infos.clear();
        infos.reserve(contacts.size());
        for (const auto& contact : contacts) {
            infos.push_back(make_hit_info(contact));
        }
        return static_cast<s32>(infos.size());'''
replacement='''        auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(0);
        keeper->_10 = 0;
        for (const auto& contact : contacts) {
            keeper->mHitInfoArray[keeper->_10++] = make_hit_info(contact);
        }
        return keeper->_10;'''
assert old in compat;compat=compat.replace(old,replacement,1)
old='''        auto& infos = strike_infos();
        infos.clear();
        if (contacts.empty()) {
            return 0;
        }
        infos.push_back(make_hit_info(contacts.front()));
        if (output != nullptr) {
            *output = infos.front();
        }
        return 1;'''
replacement='''        auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(0);
        keeper->_10 = 0;
        if (contacts.empty()) {
            return 0;
        }
        keeper->mHitInfoArray[0] = make_hit_info(contacts.front());
        keeper->_10 = 1;
        if (output != nullptr) {
            *output = keeper->mHitInfoArray[0];
        }
        return 1;'''
assert old in compat;compat=compat.replace(old,replacement,1)
compat=compat.replace('#include "compat/CollisionDirectorOwnership.hpp"\n','#include "Game/Map/CollisionCategorizedKeeper.hpp"\n#include "Game/Map/CollisionDirector.hpp"\n')
compat=compat.replace('#include "scene/SceneObjHolderRuntime.hpp"\n','').replace('#include <vector>\n','')
compat=re.sub(r'\n(?:[ \t]*\n){2,}','\n\n',compat)
assert 'strike_infos()' not in compat and 'store_line_hits' not in compat
(root/'src/compat/OriginalMapQueries.cpp').write_text(new)
(root/'src/compat/GameMapCollisionCompat.cpp').write_text(compat)
(root/'src/Game/Util/MapUtil.cpp').write_text(ref)
(out/'extracted-functions.json').write_text(json.dumps(records,indent=2)+'\n')
print('Exact original functions:',len(records),'retired native function bodies:',len(spans)-1)
