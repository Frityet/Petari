from pathlib import Path
import json,subprocess,hashlib
root=Path.cwd(); n=root/'notes/compat-owner-removal-round6-20260925/nw4r'
removed=['src/compat/Nw4rDiagnostics.cpp','src/compat/Nw4rFontCompat.cpp','src/compat/Nw4rLayoutRecordsCompat.cpp','src/compat/PPCArchCompat.cpp']
added=['src/nw4r/db/db_assert.cpp','src/nw4r/ut/ut_ResFont.cpp','src/nw4r/lyt/lyt_pane.cpp','aurora/lib/dolphin/PPCArch.cpp']
modified=['src/nw4r/ut/ut_Font.cpp','aurora/xmake.lua','aurora/cmake/aurora_os.cmake']
files=removed+added+modified
before=[]
for f in files:
 p=root/f
 exists=p.exists()
 if exists:
  dst=n/'before'/f;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(p.read_bytes())
 before.append({'path':f,'exists':exists,'sha256':hashlib.sha256(p.read_bytes()).hexdigest() if exists else None})
(n/'manifest.json').write_text(json.dumps({'files':files,'before':before},indent=2)+'\n')
font=(root/removed[1]).read_text()
start=font.index('    Font::Font()')
end=font.index('    ResFont::ResFont()')
base=font[start:end]
p=root/'src/nw4r/ut/ut_Font.cpp'
s=p.read_text().replace('#include "nw4r/ut/Font.h"','#include "nw4r/ut/Font.h"\n#include <aurora/allocation.hpp>')
s+='\nnamespace nw4r::ut {\n'+base+'}\n';p.write_text(s)
(root/'src/nw4r/ut/ut_ResFont.cpp').write_text(font[:start]+font[end:])
panic=(root/removed[0]).read_text().replace('"compat/JkrAllocationDomain.hpp"','<aurora/allocation.hpp>').replace('smgpc::compat::JkrHostAllocationScope','aurora::allocation::HostAllocationScope')
(root/'src/nw4r/db/db_assert.cpp').write_text(panic)
pane=(root/'decomp/src/nw4r/lyt/lyt_pane.cpp').read_text()
pane='#include "layout/Nw4rLayoutRecords.hpp"\n'+pane
old=(root/removed[2]).read_text()
default=old[old.index('Pane::Pane()'):old.index('Pane::Pane(const res::Pane*')]
pane=pane.replace('        Pane::Pane(const res::Pane* pRes) {',default+'\n        Pane::Pane(const res::Pane* pRes) {')
edits={
 'void Pane::SetName(const char* pName) {':'\n            smgpc::layout::validate_native_pane_rename(this, pName);',
 'void Pane::InsertChild(PaneList::Iterator next, Pane* pChild) {':'\n            smgpc::layout::validate_native_pane_hierarchy_change(this, pChild);',
 'void Pane::RemoveChild(Pane* pChild) {':'\n            smgpc::layout::validate_native_pane_hierarchy_change(this, pChild);',
 'void Pane::CalculateMtx(const DrawInfo& rInfo) {':'\n            if (smgpc::layout::synchronize_native_pane(this)) return;',
 'void Pane::AnimateSelf(u32 option) {':'\n            smgpc::layout::animate_native_pane(this);'}
for a,b in edits.items():
 assert pane.count(a)==1;pane=pane.replace(a,a+b)
pane=pane.replace('const math::VEC2 Pane::GetVtxPos() const','math::VEC2 Pane::GetVtxPos() const')
(root/'src/nw4r/lyt/lyt_pane.cpp').write_text(pane)
(root/'aurora/lib/dolphin/PPCArch.cpp').write_bytes((root/removed[3]).read_bytes())
for f in removed:(root/f).unlink()
p=root/'aurora/xmake.lua';s=p.read_text();a='"lib/dolphin/AR.cpp", "lib/nand.cpp", "lib/sysconf.cpp",';assert s.count(a)==1;s=s.replace(a,'"lib/dolphin/AR.cpp", "lib/dolphin/PPCArch.cpp", "lib/nand.cpp", "lib/sysconf.cpp",');p.write_text(s)
p=root/'aurora/cmake/aurora_os.cmake';s=p.read_text();a='        lib/dolphin/AR.cpp\n';assert s.count(a)==1;s=s.replace(a,a+'        lib/dolphin/PPCArch.cpp\n');p.write_text(s)
print('Consolidated complete current font/pane interfaces and native CPU sync into SDK owners')
