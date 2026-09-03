from pathlib import Path
import shutil,difflib,json,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-arithmetic-20260903';S=B/'staged';P=R/'build/original-jpa-draw-20260903/staged'
shutil.copytree(P,S,dirs_exist_ok=True)
p=S/'aurora/ppc_math.hpp';p.parent.mkdir(parents=True,exist_ok=True)
p.write_text('''#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace aurora::ppc {
// Gekko fctiwz's integer word result. Emulated FPSCR/CR state is outside this API.
inline std::int32_t truncate_s32(double value) {
  if (std::isnan(value) || value < -2147483648.0) {
    return std::numeric_limits<std::int32_t>::min();
  }
  if (value >= 2147483648.0) {
    return std::numeric_limits<std::int32_t>::max();
  }
  return static_cast<std::int32_t>(value);
}

// sth preserves only the low halfword; extsh interprets that halfword as signed.
inline std::int16_t narrow_s16(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));
}
inline std::uint16_t truncate_u16(double value) {
  return static_cast<std::uint16_t>(static_cast<std::uint32_t>(truncate_s32(value)));
}
inline std::int16_t truncate_s16(double value) {
  return narrow_s16(static_cast<std::uint32_t>(truncate_s32(value)));
}

// Gekko divw's result for zero divisors and signed overflow follows the dividend sign.
inline std::int32_t divide_s32(std::int32_t numerator, std::int32_t denominator) {
  if (denominator == 0 || (numerator == std::numeric_limits<std::int32_t>::min() && denominator == -1)) {
    return numerator < 0 ? -1 : 0;
  }
  return numerator / denominator;
}

// slw consumes the low six count bits; bit 5 clears the result.
inline std::int32_t shift_left_s32(std::int32_t value, std::uint32_t count) {
  const std::uint32_t shifted = (count & 0x20U) ? 0U : static_cast<std::uint32_t>(value) << (count & 0x1fU);
  return std::bit_cast<std::int32_t>(shifted);
}
} // namespace aurora::ppc
''')
changes={
 'JPAParticle.cpp':{
  'mRotateAngle = esp->getRotateInitAngle() + esp->getRotateRndmAngle() * emtr->get_r_zh();':'mRotateAngle = aurora::ppc::truncate_u16(esp->getRotateInitAngle() + esp->getRotateRndmAngle() * emtr->get_r_zh());',
  'mRotateSpeed = esp->getRotateInitSpeed() * (esp->getRotateRndmSpeed() * emtr->get_r_zp() + 1.0f);':'mRotateSpeed = aurora::ppc::truncate_s16(esp->getRotateInitSpeed() * (esp->getRotateRndmSpeed() * emtr->get_r_zp() + 1.0f));'
 },
 'JPADynamicsBlock.cpp':{
  'theta = (s16)((work->mVolumeEmitIdx << 16) / work->mEmitCount);':'theta = aurora::ppc::narrow_s16(aurora::ppc::divide_s32(aurora::ppc::shift_left_s32(work->mVolumeEmitIdx, 16), work->mEmitCount));',
  'theta = theta * work->mVolumeSweep;':'theta = aurora::ppc::truncate_s16(theta * work->mVolumeSweep);',
  'theta = work->mVolumeSweep * work->mpEmtr->get_r_ss();':'theta = aurora::ppc::truncate_s16(work->mVolumeSweep * work->mpEmtr->get_r_ss());',
  'phi = (u16)(work->mVolumeX * 0x8000 / (work->mDivNumber - 1) + 0x4000);':'phi = aurora::ppc::narrow_s16(static_cast<u32>(aurora::ppc::divide_s32(aurora::ppc::shift_left_s32(work->mVolumeX, 15), work->mDivNumber - 1)) + 0x4000U);',
  'f32 tmp = (u16)(work->mVolumeAngleNum * 0x10000 / (work->mVolumeAngleMax - 1));':'f32 tmp = static_cast<u16>(aurora::ppc::divide_s32(aurora::ppc::shift_left_s32(work->mVolumeAngleNum, 16), work->mVolumeAngleMax - 1));',
  'theta = tmp * work->mVolumeSweep + 0x8000;':'theta = aurora::ppc::truncate_s16(tmp * work->mVolumeSweep + 0x8000);'
 }
}
patches=[];native=[]
for name,subs in changes.items():
 p=S/'JSystem/JParticle'/name;old=p.read_text();new=old
 for a,b in subs.items():assert a in new;new=new.replace(a,b)
 new='#include <aurora/ppc_math.hpp>\n'+new;p.write_text(new)
 target='pc-port/src/JSystem/JParticle/'+name;patches.append(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/'+target,'b/'+target)))
 native.append({'target':target,'source':str(p.relative_to(R)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
old=(R/'pc-port/src/compat/J3DAnimationInterpolation.hpp').read_text();new=old.replace('#include <bit>','#include <aurora/ppc_math.hpp>\n#include <bit>');a=new.index('        if (std::isnan(value)');b=new.index('\n    }',a);new=new[:a]+'''        return aurora::ppc::truncate_s32(value);'''+new[b:];new=new.replace('return std::bit_cast<s16>(static_cast<u16>(value));','return aurora::ppc::narrow_s16(value);');new=new.replace('        const u32 amount = static_cast<u32>(shift);\n        const u32 shifted = (amount & 0x20U) != 0 ? 0U : static_cast<u32>(value) << (amount & 0x1FU);\n        return narrowPpcRotation(shifted);', '        return aurora::ppc::narrow_s16(aurora::ppc::shift_left_s32(value, static_cast<u32>(shift)));');new=new.replace('#include <bit>\n','').replace('#include <limits>\n','');p=S/'compat/J3DAnimationInterpolation.hpp';p.parent.mkdir(exist_ok=True);p.write_text(new)
target='pc-port/src/compat/J3DAnimationInterpolation.hpp';patches.append(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/'+target,'b/'+target)));native.append({'target':target,'source':str(p.relative_to(R)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
p=S/'aurora/ppc_math.hpp';target='pc-port/aurora/include/aurora/ppc_math.hpp';patches.append(''.join(difflib.unified_diff([],p.read_text().splitlines(True),'/dev/null','b/'+target)));native.append({'target':target,'source':str(p.relative_to(R)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(N/'native.patch').write_text(''.join(patches));(N/'native-manifest.json').write_text(json.dumps(native,indent=2)+'\n')
