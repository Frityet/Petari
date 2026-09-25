from pathlib import Path
import re,json,hashlib,subprocess
base=Path('notes/compat-owner-removal-round20-20260925/std-prefix');base.mkdir(parents=True,exist_ok=True)
manifest={'owned_paths':[]};recorded=set()
def snapshot(path):
 p=Path(path)
 if path in recorded:return
 recorded.add(path);data=p.read_bytes() if p.exists() else None
 if data is not None:
  t=base/'before'/p;t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(data)
 if path.startswith('aurora/'):
  status=subprocess.check_output(['git','-C','aurora','status','--short','--',path[len('aurora/'):]],text=True).rstrip()
 else:status=subprocess.check_output(['git','status','--short','--',path],text=True).rstrip()
 manifest['owned_paths'].append({'path':path,'before_exists':data is not None,'before_sha256':hashlib.sha256(data).hexdigest() if data else None,'before_status':status})
 (base/'owned-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
def write(path,s):
 snapshot(path);p=Path(path);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s)
def add_include(path,header):
 p=Path(path);s=p.read_text()
 if re.search(r'#include [<"]'+re.escape(header)+r'[>"]',s):return
 include=f'#include <{header}>\n' if header in {'functional.hpp','macros.h'} else f'#include "{header}"\n'
 # Keep literal encoding adaptation first for exact donor-source checks.
 if s.startswith('#include "resource/TextEncoding.hpp"\n'):
  pos=s.index('\n')+1;s=s[:pos]+include+s[pos:]
 elif s.startswith('#pragma once\n'):
  pos=s.index('\n')+1;s=s[:pos]+'\n'+include+s[pos:]
 else:s=include+s
 write(path,s)
p='src/Game/Util/MathUtil.hpp';s=Path(p).read_text().replace('#include <cmath>','#include <cmath>\n#include <concepts>');needle='    /// @brief Restricts a number to the unit interval.'
extra='''#if defined(TARGET_PC)
    // Wii long is 32-bit; preserve the original integer overload on LP64 hosts.
    template <std::integral Value, std::integral Minimum, std::integral Maximum>
        requires (std::same_as<Value, long> || std::same_as<Minimum, long> || std::same_as<Maximum, long>)
    inline s32 clamp(Value value, Minimum min, Maximum max) {
        return clamp(static_cast<s32>(value), static_cast<s32>(min), static_cast<s32>(max));
    }
#endif

'''
s=s.replace(needle,extra+needle,1);write(p,s)
p='aurora/include/revolution/types.h';s=Path(p).read_text();s=s.replace('#include <stdint.h>','#include <stdint.h>\n#include <string.h>');s+='''
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) ((s32)(sizeof(array) / sizeof((array)[0])))
#endif

// Compiler intrinsics declared by the original Revolution types header.
// Bit operations preserve NaN payloads, clear negative zero, and avoid signed
// overflow for the two's-complement INT_MIN absolute-value result.
#if !defined(__MWERKS__)
#ifdef __cplusplus
extern "C" {
#define RVL_INTRINSIC_NOEXCEPT noexcept
#else
#define RVL_INTRINSIC_NOEXCEPT
#endif
inline f32 __fabsf(f32 value) RVL_INTRINSIC_NOEXCEPT {
  u32 bits;
  memcpy(&bits, &value, sizeof(bits));
  bits &= 0x7fffffffU;
  memcpy(&value, &bits, sizeof(value));
  return value;
}
inline f64 __fabs(f64 value) RVL_INTRINSIC_NOEXCEPT {
  u64 bits;
  memcpy(&bits, &value, sizeof(bits));
  bits &= 0x7fffffffffffffffULL;
  memcpy(&value, &bits, sizeof(value));
  return value;
}
inline s32 __abs(s32 value) RVL_INTRINSIC_NOEXCEPT {
  u32 magnitude = value < 0 ? -(u32)value : (u32)value;
  memcpy(&value, &magnitude, sizeof(value));
  return value;
}
#undef RVL_INTRINSIC_NOEXCEPT
#ifdef __cplusplus
}
#endif
#endif
''';write(p,s)
write('aurora/include/macros.h','''#pragma once

// Original RVL angle conversion constants and operation order.
#ifndef M_PI_F
#define M_PI_F 3.1415926f
#endif
#ifndef M_TAU
#define M_TAU 6.283185307179586
#endif
#ifndef DEG_TO_RAD_MULT_CONSTANT
#define DEG_TO_RAD_MULT_CONSTANT (M_PI_F / 180.0f)
#endif
#ifndef RAD_TO_DEG_MULT_CONSTANT
#define RAD_TO_DEG_MULT_CONSTANT (180.0f / M_PI_F)
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD(x) ((x) * DEG_TO_RAD_MULT_CONSTANT)
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG(x) ((x) * RAD_TO_DEG_MULT_CONSTANT)
#endif
''')
for p in ['src/Game/Gravity/ParallelGravity.cpp','src/Game/Map/RailPart.cpp']:
 write(p,Path(p).read_text().replace('void DUMMY() {','static void DUMMY() {'))
for root in ['src','tests']:
 for p in Path(root).rglob('*'):
  if p.suffix not in {'.cpp','.hpp'} or str(p)=='src/compat/MetrowerksStdCompat.hpp':continue
  s=p.read_text()
  if '#include "compat/MetrowerksStdCompat.hpp"' in s:
   s=s.replace('#include "compat/MetrowerksStdCompat.hpp"\n','')
   # Stdio aliases must appear before every native header in explicit clients.
   write(str(p),'#include <MSL_C/stdio.h>\n'+s)
for p in ['src/Game/xmake.lua','tests/xmake.lua']:
 write(p,Path(p).read_text().replace('src/compat/MetrowerksStdCompat.hpp','aurora/include/MSL_C/stdio.h'))
# Include the defining owner where a source directly names its API. Do not
# force any Game class, math umbrella, functional adapter or JKR declaration.
patterns=[(r'std::(?:mem_fun|mem_fun_ref|bind1st|bind2nd|unary_function|binary_function|ptr_fun)\b','functional.hpp'),(r'\bTDDraw::','Game/Util/DirectDraw.hpp'),(r'\b(?:DEG_TO_RAD|RAD_TO_DEG)\b','macros.h'),(r'\bnew\s*\([^\n)]*(?:[0-9]|[Hh]eap|,)','JSystem/JKernel/JKRHeap.hpp')]
for p in Path('src/Game').rglob('*'):
 if p.suffix not in {'.cpp','.hpp'}:continue
 s=p.read_text()
 for pattern,header in patterns:
  if re.search(pattern,s):add_include(str(p),header)
# Detect actual math constant users that previously relied on the forced header.
include_pattern=re.compile(r'^\s*#\s*include\s*[<"]([^>"\n]+)[>"]',re.M)
include_cache={}
def reaches(path,target,seen=None):
 if path==target:return True
 if seen is None:seen=set()
 if path in seen:return False
 seen.add(path)
 if path not in include_cache:
  p=Path(path);result=[]
  if p.is_file():
   for inc in include_pattern.findall(p.read_text(errors='replace')):
    for q in [p.parent/inc,Path('src')/inc,Path('aurora/include')/inc]:
     if q.is_file():result.append(str(q));break
  include_cache[path]=result
 return any(reaches(q,target,seen) for q in include_cache[path])
math_pattern=r'\b(?:HALF_PI|HALF_PI_D|PI|TWO_PI|TWO_PI_D|PI_180|_180_PI|DEGREE_TO_S16|FLOAT_MAX|FLOAT_ZERO|gZeroVec)\b'
for p in Path('src/Game').rglob('*'):
 if p.suffix not in {'.cpp','.hpp'}:continue
 s=p.read_text()
 if re.search(math_pattern,s) and not reaches(str(p),'src/math_types.hpp'):add_include(str(p),'math_types.hpp')
# LiveActor complete-type accesses must have an actual class include chain.
for p in Path('src/Game').rglob('*'):
 if p.suffix not in {'.cpp','.hpp'}:continue
 s=p.read_text()
 if re.search(r'(?:public\s+LiveActor\b|\bLiveActor\s*::|\bmHost\s*->|\bstatic_cast\s*<\s*LiveActor\s*\*)',s) and not reaches(str(p),'src/Game/LiveActor/LiveActor.hpp'):
  add_include(str(p),'Game/LiveActor/LiveActor.hpp')
snapshot('src/compat/MetrowerksStdCompat.hpp');Path('src/compat/MetrowerksStdCompat.hpp').unlink()
print('Edited/deleted',len(recorded),'owned paths; manifest',base/'owned-manifest.json')
