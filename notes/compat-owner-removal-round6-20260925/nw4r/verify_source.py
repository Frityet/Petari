from pathlib import Path
import json, importlib.util, hashlib, re
root=Path(__file__).resolve().parents[3]; n=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('source_parser',root/'notes/compat-owner-removal-round5-20260925/draw/source_parser.py')
parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
norm=parser.normalize;checks=[]
def check(name,value):
 checks.append({'check':name,'passed':bool(value)})
def read(p):return (root/p).read_text()
def before(p):return (n/'before'/p).read_text()
font=before('src/compat/Nw4rFontCompat.cpp')
a=font.index('    Font::Font()');b=font.index('    ResFont::ResFont()')
def without_includes(s):return re.sub(r'^#include[^\n]*','',s,flags=re.M)
actual=read('src/nw4r/ut/ut_ResFont.cpp');expected=font[:a]+font[b:]
check('Complete ResFont implementation and helpers unchanged',norm(without_includes(actual))==norm(without_includes(expected)) and sorted(re.findall(r'^#include[^\n]*',actual,re.M))==sorted(re.findall(r'^#include[^\n]*',expected,re.M)))
check('Font native ownership methods preserved',norm(font[a:b]) in norm(read('src/nw4r/ut/ut_Font.cpp')))
check('Original Font character reader unchanged',norm(before('src/nw4r/ut/ut_Font.cpp').replace('#include "nw4r/ut/Font.h"','')) in norm(read('src/nw4r/ut/ut_Font.cpp')))
panic=before('src/compat/Nw4rDiagnostics.cpp').replace('"compat/JkrAllocationDomain.hpp"','<aurora/allocation.hpp>').replace('smgpc::compat::JkrHostAllocationScope','aurora::allocation::HostAllocationScope')
# Formatting sorts includes; compare complete function bodies independently.
check('Native Panic keeps identical behavior and direct Aurora allocation', [x['normalized'] for x in parser.functions(panic)]==[x['normalized'] for x in parser.functions(read('src/nw4r/db/db_assert.cpp'))])
check('PPCSync memory fence unchanged',norm(before('src/compat/PPCArchCompat.cpp'))==norm(read('aurora/lib/dolphin/PPCArch.cpp')))
pane=read('src/nw4r/lyt/lyt_pane.cpp')
hooks=['smgpc::layout::validate_native_pane_rename(this, pName);','smgpc::layout::validate_native_pane_hierarchy_change(this, pChild);','if (smgpc::layout::synchronize_native_pane(this)) return;','smgpc::layout::animate_native_pane(this);']
normalized=norm(pane.replace('GXLoadPosMtxImm(pMtx->m, GX_PNMTX0);', 'GXLoadPosMtxImm(*pMtx, GX_PNMTX0);'))
check('Original NW4R matrix copy helper restored', norm('inline MTX34* MTX34Copy(MTX34* pOut, const MTX34* pIn) { PSMTXCopy(*pIn, *pOut); return pOut; }') in norm(read('src/nw4r/math/types.h')))
for hook in hooks:
 h=norm(hook);check('Native pane hook preserved: '+hook,h in normalized);normalized=normalized.replace(h,'')
donor=read('decomp/src/nw4r/lyt/lyt_pane.cpp').replace('const math::VEC2 Pane::GetVtxPos() const','math::VEC2 Pane::GetVtxPos() const')
for f in parser.functions(donor):
 check('Complete donor body: '+f['symbol'],f['normalized'] in normalized)
old=before('src/compat/Nw4rLayoutRecordsCompat.cpp')
default=old[old.index('Pane::Pane()'):old.index('Pane::Pane(const res::Pane*')]
check('Native default Pane constructor preserved',norm(default) in norm(pane))
for p in ['src/compat/Nw4rDiagnostics.cpp','src/compat/Nw4rFontCompat.cpp','src/compat/Nw4rLayoutRecordsCompat.cpp','src/compat/PPCArchCompat.cpp']:
 check('Retired provider removed: '+p,not(root/p).exists())
result={'passed':all(x['passed'] for x in checks),'checks':checks,'donor_functions':len(parser.functions(donor)),'files':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in ['src/nw4r/db/db_assert.cpp','src/nw4r/ut/ut_Font.cpp','src/nw4r/ut/ut_ResFont.cpp','src/nw4r/lyt/lyt_pane.cpp','aurora/lib/dolphin/PPCArch.cpp']}}
(n/'source-validation.json').write_text(json.dumps(result,indent=2)+'\n')
print(f"{sum(x['passed'] for x in checks)}/{len(checks)} source checks passed")
assert result['passed'],[x for x in checks if not x['passed']]
