"""One-shot owner restoration from the recorded donor/native snapshots.

Historical mutation recipe; use validate-source.py for repeatable read-only checks.
"""
from pathlib import Path
import re

N = Path(__file__).parent
B = N / 'before'
PATTERN = re.compile(r'^([^\n{};]*?\b(J3D\w+::[~\w]+)\([^;{]*\)[^{;]*?)\{', re.M)

def methods(text):
    result = {}
    counts = {}
    for match in PATTERN.finditer(text):
        name = match[2]
        occurrence = counts.get(name, 0)
        counts[name] = occurrence + 1
        depth = 1
        end = match.end()
        while depth:
            depth += (text[end] == '{') - (text[end] == '}')
            end += 1
        result[(name, occurrence)] = text[match.start():end]
    return result

native = {}
for family in ['FrameCtrl', 'TransformAnimation', 'MaterialAnimation', 'AdditionalAnimation']:
    native.update(methods((B / f'src/compat/J3D{family}Compat.cpp').read_text()))
source = (B / 'decomp/src/JSystem/J3DGraphAnimator/J3DAnimation.cpp').read_text()
donor = methods(source)
assert len(donor) == 35 and len(native) == 36
assert set(native) - set(donor) == {('J3DAnmTransformFull::~J3DAnmTransformFull', 0)}

# Keep complete donor ordering and unmodified bodies. These nine samplers
# already contain the verified PPC conversion/store adaptations.
adapted = {
    'J3DAnmTransformFull::getTransform',
    'J3DAnmTransformFullWithLerp::getTransform',
    'J3DAnmTransformKey::calcTransform',
    'J3DAnmTextureSRTKey::calcTransform',
    'J3DAnmClusterFull::getWeight',
    'J3DAnmVtxColorFull::getColor',
    'J3DAnmColorFull::getColor',
    'J3DAnmTexPattern::getTexNo',
    'J3DAnmVisibilityFull::getVisibility',
}
for key, original in donor.items():
    if key[0] in adapted:
        source = source.replace(original, native[key], 1)

ctor = donor[('J3DAnmTransform::J3DAnmTransform', 0)]
source = source.replace(ctor, ctor + '\n\n' + native[('J3DAnmTransformFull::~J3DAnmTransformFull', 0)], 1)

helper = (B / 'src/compat/J3DAnimationInterpolation.hpp').read_text()
helper_start = helper.index('namespace {')
interpolation_start = helper.index('inline f32 J3DHermiteInterpolation')
conversions = helper[helper_start:interpolation_start].strip()
interpolation = helper[interpolation_start:].strip()
start = source.index('inline f32 J3DHermiteInterpolation')
end = source.index('void J3DAnmTransformKey::calcTransform', start)
source = source[:start] + interpolation + '\n\n' + source[end:]

first = source.index('void J3DFrameCtrl::init')
source = '''#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "JSystem/JMath/JMath.hpp"

#include <aurora/ppc_math.hpp>
#include <cmath>

// The original JSystem unit uses -fp_contract off. BCA lerp performs
// separate multiply and add operations; signed-16 Hermite uses explicit FMA.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

''' + conversions + '\n\n' + source[first:]
out = Path('src/JSystem/J3DGraphAnimator/J3DAnimation.cpp')
assert not out.exists()
out.write_text(source)

# Restore the complete donor loader and retain the bounded native resource
# boundary. Decoded pointer-bearing blocks cannot use guest contiguous offsets.
source = (B / 'decomp/src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp').read_text()
source = source.replace('#include "JSystem/JSupport/JSupport.hpp"', '#include "JSystem/JSupport/JSupport.hpp"\n#include "resource/J3dAnimationResource.hpp"\n\n#include <cstdint>')
source = source.replace('J3DAnmBase* J3DAnmLoaderDataBase::load(', '''// Called only by the bounded resource owner with a decoded native header.
J3DAnmBase* smgpc::resource::detail::load_native_animation(''', 1)
assert source.count('&header->mFirstBlock') == 4
assert source.count('block->getNext()') == 4
source = source.replace('&header->mFirstBlock', 'smgpc::resource::detail::first_animation_block(header)')
source = source.replace('block->getNext()', 'smgpc::resource::detail::next_animation_block(header, block)')
for index in range(2):
    original = f'(void*)((s32)indexPtr{index} + (s32)dst->mAnmVtxColorIndexData[{index}][i].mpData * 2)'
    replacement = f'(void*)((std::uintptr_t)indexPtr{index} + (std::uintptr_t)dst->mAnmVtxColorIndexData[{index}][i].mpData * 2)'
    assert source.count(original) == 2
    source = source.replace(original, replacement)
source += '''
J3DAnmBase* J3DAnmLoaderDataBase::load(const void* data, J3DAnmLoaderDataBaseFlag flag) {
    return smgpc::resource::load_registered_j3d_animation(data, flag);
}
'''
out = Path('src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp')
assert not out.exists()
out.write_text(source)

for name in ['J3DFrameCtrlCompat.cpp', 'J3DTransformAnimationCompat.cpp', 'J3DMaterialAnimationCompat.cpp', 'J3DAdditionalAnimationCompat.cpp', 'J3DAnimationInterpolation.hpp', 'J3DAnmLoaderCompat.cpp']:
    path = Path('src/compat') / name
    assert path.read_bytes() == (B / path).read_bytes()
    path.unlink()
print('Restored two complete canonical owners; retired six compat files.')
