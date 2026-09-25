from pathlib import Path
import hashlib,json,re
root=Path(__file__).resolve().parents[2]
native=root/'src/JSystem/J3DGraphBase/J3DPacket.cpp'
donor=root/'decomp/src/JSystem/J3DGraphBase/J3DPacket.cpp'
header=root/'src/JSystem/J3DGraphBase/J3DPacket.hpp'
s=native.read_text();d=donor.read_text()
checks={
 'entire_current_donor_except_standard_cstring_include': s==d.replace('#include <mem.h>','#include <cstring>'),
 'compat_provider_deleted': not (root/'src/compat/J3DPacketCompat.cpp').exists(),
 'public_return_type_matches_donor': 'J3DError newDifferedDisplayList(u32);' in header.read_text(),
 'original_failed_allocation_propagates': re.search(r'J3DError J3DShapePacket::newDifferedDisplayList[\s\S]*?if \(ret != kJ3DError_Success\) \{\s*return ret;',s) is not None,
 'seven_register_table': 'static u32 sDifferedRegister[7]' in s and 'static s32 sSizeOfDiffered[7]' in s,
 'original_indirect_stage_budget': 'J3DDiffFlag_TevStageIndirect' in s.split('static u32 sDifferedRegister[7]')[1].split('};',1)[0],
 'no_added_ambient_budget': 'J3DDiffFlag_AmbColor' not in s.split('static u32 sDifferedRegister[7]')[1].split('};',1)[0],
 'original_device_sdk_boundary_preserved': all(x in s for x in ['OSDisableInterrupts()', 'OSRestoreInterrupts(sInterruptFlag)', 'GDInitGDLObj(', 'GDFlushCurrToMem()', 'GXCallDisplayList(', 'DCStoreRange(']),
}
# Provider scan includes every native CPP. References and declarations are not definitions.
pattern=re.compile(r'^\s*(?:[\w:*&<>]+\s+)*(J3D(?:DisplayListObj|Packet|DrawPacket|MatPacket|ShapePacket)::[\w~]+)\s*\([^;{}]*\)\s*(?:const\s*)?\{',re.M)
providers={}
for p in (root/'src').rglob('*.cpp'):
 text=p.read_text(errors='replace')
 for symbol in pattern.findall(text):providers.setdefault(symbol,[]).append(str(p.relative_to(root)))
checks['all_32_packet_definitions_have_single_canonical_owner']=len(providers)==32 and all(p==['src/JSystem/J3DGraphBase/J3DPacket.cpp'] for p in providers.values())
static_providers={}
for symbol in ('sGDLObj','sInterruptFlag'):
 locations=[]
 rx=re.compile(r'^\s*\w+\s+J3DDisplayListObj::'+symbol+r'\s*;',re.M)
 for p in (root/'src').rglob('*.cpp'):
  if rx.search(p.read_text(errors='replace')):locations.append(str(p.relative_to(root)))
 static_providers[symbol]=locations
checks['static_state_has_single_canonical_owner']=all(p==['src/JSystem/J3DGraphBase/J3DPacket.cpp'] for p in static_providers.values())
report={'checks':checks,'providers':providers,'static_providers':static_providers,'native_sha256':hashlib.sha256(native.read_bytes()).hexdigest(),'donor_sha256':hashlib.sha256(donor.read_bytes()).hexdigest(),'scope':'Source ownership, donor equality, public signature, original error branch/register table and unchanged device boundaries. Not a compile, heap exhaustion, GPU or gameplay test.'}
(root/'notes/compat-whole-surface-20260925/j3d-packet-source-validation.json').write_text(json.dumps(report,indent=2)+'\n')
for name,passed in checks.items():print(('PASS' if passed else 'FAIL'),name)
assert all(checks.values())
