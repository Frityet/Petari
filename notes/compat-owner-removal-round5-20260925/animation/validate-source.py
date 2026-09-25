"""Read-only source checks; this does not compile or execute the port."""
from pathlib import Path
import difflib
import hashlib
import json
import re

N = Path(__file__).parent
B = N / 'before'
PATTERN = re.compile(r'^([^\n{};]*?\b((?:J3D\w+::[~\w]+)|(?:smgpc::resource::detail::load_native_animation))\([^;{]*\)[^{;]*?)\{', re.M)

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

def normal(text):
    text = re.sub(r'//[^\n]*', '', text)
    return re.sub(r'\s+', '', text)

def body(text):
    return text[text.index('{'):]

native = {}
locations = {}
for family in ['FrameCtrl', 'TransformAnimation', 'MaterialAnimation', 'AdditionalAnimation']:
    source = f'src/compat/J3D{family}Compat.cpp'
    found = methods((B / source).read_text())
    assert not set(native).intersection(found)
    native.update(found)
    locations.update({key: source for key in found})
source = 'src/JSystem/J3DGraphAnimator/J3DAnimation.cpp'
current_text = Path(source).read_text()
current = methods(current_text)
donor = methods((B / ('decomp/' + source)).read_text())
assert len(donor) == 35 and len(native) == 36
assert set(current) == set(native)
adapted = {
    'J3DAnmTransformFull::getTransform', 'J3DAnmTransformFullWithLerp::getTransform',
    'J3DAnmTransformKey::calcTransform', 'J3DAnmTextureSRTKey::calcTransform',
    'J3DAnmClusterFull::getWeight', 'J3DAnmVtxColorFull::getColor',
    'J3DAnmColorFull::getColor', 'J3DAnmTexPattern::getTexNo',
    'J3DAnmVisibilityFull::getVisibility', 'J3DAnmTransformFull::~J3DAnmTransformFull',
}
rows = []
for key, definition in current.items():
    retain_native = key[0] in adapted
    expected = native[key] if retain_native else donor[key]
    assert normal(definition) == normal(expected), key
    rows.append({'method':key[0], 'overload':key[1], 'owner':source,
                 'previous_provider':locations[key],
                 'result':'native-definition-preserved' if retain_native else 'donor-definition-restored'})

helper = (B / 'src/compat/J3DAnimationInterpolation.hpp').read_text()
conversion = helper[helper.index('namespace {'):helper.index('inline f32 J3DHermiteInterpolation')]
interpolation = helper[helper.index('inline f32 J3DHermiteInterpolation'):]
assert normal(conversion) in normal(current_text)
assert normal(interpolation) in normal(current_text)
for pragma in ['#pragma clang fp contract(off)', '#pragma GCC optimize("fp-contract=off")', '#pragma fp_contract(off)']:
    assert pragma in current_text
assert current_text.count('template <typename T>') == 1

source = 'src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp'
current = methods(Path(source).read_text())
native = methods((B / 'src/compat/J3DAnmLoaderCompat.cpp').read_text())
donor = methods((B / ('decomp/' + source)).read_text())
assert set(current) == set(native)
assert set(donor) <= set(current)
for key, definition in current.items():
    if key[0] == 'smgpc::resource::detail::load_native_animation':
        expected = native[key]
        result = 'native-dispatch-definition-preserved'
    elif key[0] == 'J3DAnmLoaderDataBase::load':
        expected = native[key]
        result = 'bounded-public-entry-preserved'
    else:
        expected = donor[key]
        expected = expected.replace('&header->mFirstBlock', 'smgpc::resource::detail::first_animation_block(header)')
        expected = expected.replace('block->getNext()', 'smgpc::resource::detail::next_animation_block(header, block)')
        for index in range(2):
            expected = expected.replace(
                f'(void*)((s32)indexPtr{index} + (s32)dst->mAnmVtxColorIndexData[{index}][i].mpData * 2)',
                f'(void*)((std::uintptr_t)indexPtr{index} + (std::uintptr_t)dst->mAnmVtxColorIndexData[{index}][i].mpData * 2)')
        result = 'donor-definition-with-existing-native-traversal-and-width-adaptations'
    assert normal(definition) == normal(expected), key
    rows.append({'method':key[0], 'overload':key[1], 'owner':source,
                 'previous_provider':'src/compat/J3DAnmLoaderCompat.cpp', 'result':result})

unchanged = ['src/JSystem/J3DGraphAnimator/J3DAnimation.hpp', 'src/JSystem/J3DGraphLoader/J3DAnmLoader.hpp',
             'src/resource/J3dAnimationResource.cpp', 'src/resource/J3dAnimationResource.hpp']
for path in unchanged:
    assert Path(path).read_bytes() == (B / path).read_bytes(), path
retired = [f'src/compat/{file}' for file in ['J3DFrameCtrlCompat.cpp', 'J3DTransformAnimationCompat.cpp',
           'J3DMaterialAnimationCompat.cpp', 'J3DAdditionalAnimationCompat.cpp', 'J3DAnimationInterpolation.hpp', 'J3DAnmLoaderCompat.cpp']]
assert all(not Path(path).exists() for path in retired)
for path in ['src/JSystem/J3DGraphAnimator/J3DAnimation.cpp', 'src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp']:
    assert not re.search(r'#include ["<](?:compat|Game)/', Path(path).read_text())

result = {'status':'passed-static-source-only', 'sampler_class_methods':36, 'sampler_donor_methods':35,
          'sampler_donor_definitions_restored':26, 'sampler_native_adapted_definitions_preserved':9,
          'native_transform_destructor_preserved':True, 'loader_definitions':len(current),
          'interpolation_and_ppc_conversions_preserved':True, 'fp_contract_controls_preserved':True,
          'unchanged_headers_and_resource_bridge':unchanged, 'retired_files':retired, 'methods':rows}
(N / 'source-validation.json').write_text(json.dumps(result, indent=2) + '\n')
(N / 'method-inventory.json').write_text(json.dumps(rows, indent=2) + '\n')
print('Verified 36 sampler class methods, all interpolation/conversion helpers, and ' + str(len(current)) + ' loader definitions; headers/resource decoder unchanged.')
