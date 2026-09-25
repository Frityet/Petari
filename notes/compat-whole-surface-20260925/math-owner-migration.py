from pathlib import Path

root = Path(__file__).resolve().parents[2]
def read(p): return (root / p).read_text()
def span(source, signature):
    assert source.count(signature) == 1, (signature, source.count(signature))
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return start, opening, end

def body(source, signature):
    start, opening, end = span(source, signature)
    return source[opening+1:end-1]

def replace_body(source, signature, replacement):
    start, opening, end = span(source, signature)
    return source[:opening+1] + replacement + source[end-1:]

source = read('decomp/src/Game/Util/MathUtil.cpp')
native = read('src/compat/GameMathCompat.cpp')
sqrt = read('src/compat/OriginalJMathSqrt.cpp')
source = source.replace('#include <revolution/mtx.h>\n', '#include <revolution/mtx.h>\n\n#if defined(TARGET_PC)\n#include <aurora/exception.hpp>\n#include <aurora/ppc_math.hpp>\n#include <dolphin/ppc_math.h>\n#include <bit>\n#include <cmath>\n#include <limits>\n#include <stdexcept>\n#endif\n')
# Game random state remains solely in the original GameSystemObjHolder.
source = source.replace('        return reinterpret_cast< f32& >(value) - 1.0f;', '#if defined(TARGET_PC)\n        return std::bit_cast< f32 >(value) - 1.0f;\n#else\n        return reinterpret_cast< f32& >(value) - 1.0f;\n#endif')
source = source.replace('    f32 acosEx(f32 x) {\n', '    f32 acosEx(f32 x) {\n#if defined(TARGET_PC)\n        // Avoid an undefined host float-to-integer conversion outside the domain.\n        if (!std::isfinite(x) || x < -1.0f || x > 1.0f) {\n            return std::numeric_limits< f32 >::quiet_NaN();\n        }\n#endif\n')
# Keep only native representation/ABI adaptations; donor algorithms remain intact.
for donor_sig, native_sig in [
    ('void setNan(TVec3f& rDst)', 'void setNan(TVec3f& rDst)'),
    ('bool isNan(const TVec3f& rVec)', 'bool isNan(const TVec3f& vector)'),
    ('void floatToFixed16(TVec3s* pDst, const TVec3f& rPSrc, u8 q)', 'void floatToFixed16(TVec3s* pDst, const TVec3f& pSrc, u8 q)'),
    ('void fixed16ToFloat(TVec3f* pDst, const TVec3s& rPSrc, u8 q)', 'void fixed16ToFloat(TVec3f* pDst, const TVec3s& pSrc, u8 q)'),
    ('f32 getMaxElement(const TVec3f& rVec)', 'f32 getMaxElement(const TVec3f &rVec)'),
]:
    original_body = body(source, donor_sig)
    native_body = body(native, native_sig).replace('vector.', 'rVec.').replace('pSrc.', 'rPSrc.')
    source = replace_body(source, donor_sig, '\n#if defined(TARGET_PC)' + native_body + '#else' + original_body + '#endif\n    ')
# The same explicit member access avoids indexing beyond a scalar subobject.
sig = 'f32 getMaxAbsElement(const TVec3f& rVec)'
original_body = body(source, sig)
source = replace_body(source, sig, '\n#if defined(TARGET_PC)\n        const u32 index = getMaxAbsElementIndex(rVec);\n        return index == 0 ? rVec.x : index == 1 ? rVec.y : rVec.z;\n#else' + original_body + '#endif\n    ')
# Preserve the previously compiled paired-single instruction translation bodies.
for donor_sig, native_sig in [
    ('f32 PSVECKillElement(__REGISTER const Vec* pSrc, __REGISTER const Vec* pKill, __REGISTER const Vec* pDst)', 'f32 PSVECKillElement(const Vec* pSrc, const Vec* pKill, const Vec* pDst)'),
    ('void vecScaleAdd(const register TVec3f* pA1, const register TVec3f* pA2, register f32 a3)', 'void vecScaleAdd(const TVec3f* destination, const TVec3f* add, f32 scale)'),
    ('void PSvecBlend(const register TVec3f* pA1, const register TVec3f* pA2, register TVec3f* pA3, register f32 a4, register f32 a5)', 'void PSvecBlend(const register TVec3f* a1, const register TVec3f* a2, register TVec3f* a3, register f32 a4, register f32 a5)'),
]:
    original_body = body(source, donor_sig)
    native_body = body(native, native_sig)
    if native_sig.startswith('void PSvecBlend'):
        native_body = native_body.split('#else', 1)[1].split('#endif', 1)[0]
        import re
        for name in ('a1', 'a2', 'a3'):
            native_body = re.sub(r'\b'+name+r'\b', 'pA'+name[1:], native_body)
    elif native_sig.startswith('void vecScaleAdd'):
        import re
        for name, target in [('destination', 'pA1'), ('add', 'pA2'), ('scale', 'a3')]:
            native_body = re.sub(r'\b'+name+r'\b', target, native_body)
    original_body = original_body.replace('#endif', '#else'+native_body+'#endif', 1)
    source = replace_body(source, donor_sig, original_body)
# JMASqrt is a MathUtil owner function, not a separate JSystem replacement.
sig = 'f32 JMASqrt(__REGISTER f32 value)'
original_body = body(source, sig)
native_body = body(sqrt, 'f32 JMASqrt(f32 x)').replace('x >', 'value >').replace('(x)', '(value)').replace('return x;', 'return value;')
source = replace_body(source, sig, '\n#if defined(__MWERKS__)' + original_body + '#else' + native_body + '#endif\n')
# The non-MW definitions correspond to declarations already present in MathUtil.hpp.
extra = '\n#if defined(TARGET_PC)\nnamespace MR {\n'
for sig in ('s32 getRandom(long min, long max)', 'f32 frsqrte(f32 x)', 'f32 fastSqrtf(f32 x)'):
    start, opening, end = span(native, sig)
    extra += '    ' + native[start:end] + '\n\n'
extra += '}  // namespace MR\n#endif\n'
source += extra
(root / 'src/Game/Util/MathUtil.cpp').write_text(source)
# Ensure LP64 declaration is owned by the actual utility header when forced compat includes retire.
p = root / 'src/Game/Util/MathUtil.hpp'
s = p.read_text(); anchor = '    s32 getRandom(s32 min, s32 max);\n'
assert s.count(anchor) == 1
s = s.replace(anchor, anchor+'\n#if defined(TARGET_PC)\n    s32 getRandom(long min, long max);\n#endif\n')
p.write_text(s)
# Remove exactly the overlapping canonical MathUtil functions; keep unrelated providers intact.
trims = {
'src/compat/OriginalMtxGeometry.cpp': [
    'f32 diffAngleAbs(f32 angleA, f32 angleB)',
    'f32 diffAngleAbs(const TVec2f& rA, const TVec2f& rB)',
    'bool isNormalize(const TVec3f& rVec, f32 tolerance)',
    'void getRotatedAxisZ(TVec3f* pDst, const TVec3f& pSrc)',
],
'src/compat/OriginalImageEffectUtil.cpp': [
    'u8 lerp(u8 start, u8 end, f32 t)',
    'GXColor lerp(GXColor start, GXColor end, f32 t)',
],
'src/compat/OriginalCollisionGeometry.cpp': [
    'bool checkHitSegmentSphere(const TVec3f& rSpherePos, const TVec3f& rPointA, const TVec3f& rPointB, f32 radius, TVec3f* pDir)',
],
}
for path, signatures in trims.items():
    p = root / path
    s = p.read_text()
    for sig in signatures:
        start, opening, end = span(s, sig)
        line_start = s.rfind('\n', 0, start) + 1
        s = s[:line_start] + s[end:].lstrip('\n')
    s = s.replace('\nnamespace MR {\n}\n', '\n').replace('\nnamespace MR {\n\n}\n', '\n')
    if path.endswith('OriginalCollisionGeometry.cpp'):
        s = s.replace('// Original segment/sphere broad-phase used by CollisionCategorizedKeeper.\n', '')
    p.write_text(s)
for path in ['src/compat/GameMathCompat.cpp', 'src/compat/OriginalJMathSqrt.cpp', 'src/compat/OriginalVectorOrientation.cpp', 'src/compat/OriginalPointerVectorQueries.cpp']:
    (root / path).unlink()
print('Restored complete MathUtil donor with native numeric/ABI branches; removed four providers and seven overlapping methods.')
