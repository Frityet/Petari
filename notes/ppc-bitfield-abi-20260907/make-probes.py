#!/usr/bin/env python3
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
records=json.loads((OUT/'field-groups.json').read_text())
fields=[]
for name,groups in records.items():
 for word,group in enumerate(groups):
  offset=0
  for field,width in group:
   shift=32-offset-width
   if field:fields.append({'type':{'MovementStates':'Mario::MovementStates','DrawStates':'Mario::DrawStates'}.get(name,name),'field':field,'word':word,'width':width,'shift':shift,'mask':f'0x{((1<<width)-1)<<shift:08x}','name':name.replace('::','_')+field})
   offset+=width
(OUT/'canonical-field-masks.json').write_text(json.dumps(fields,indent=2)+'\n')
# These functions are compiled twice with the original Wii compiler. The baseline
# uses untouched reference declarations; the comparison uses the shared helper.
source=['#include "Game/Player/Mario.hpp"\n#include "Game/Player/J3DModelX.hpp"\n']
for f in fields:
 kind=f['type'];count=2 if kind=='Mario::MovementStates' else 1
 source += [f'''extern "C" u32 set_{f['name']}(u32 input) {{
    union {{ {kind} fields; u32 words[{count}]; }} value;
''']
 for i in range(count):source += [f'    value.words[{i}] = 0;\n']
 source += [f"    value.fields.{f['field']} = input;\n    return value.words[{f['word']}];\n}}\n"]
 source += [f'''extern "C" u32 get_{f['name']}(u32 input) {{
    union {{ {kind} fields; u32 words[{count}]; }} value;
''']
 for i in range(count):source += [f"    value.words[{i}] = {'input' if i==f['word'] else '0'};\n"]
 source += [f"    return value.fields.{f['field']};\n}}\n"]
(OUT/'wii-fields.cpp').write_text(''.join(source))
# Only the two record declarations differ in the temporary reference overlay.
for filename,record_names in [('Mario.hpp',['MovementStates','DrawStates']),('J3DModelX.hpp',['Flags'])]:
 original=(ROOT/'decomp/include/Game/Player'/filename).read_text();native=(ROOT/'src/Game/Player'/filename).read_text()
 for record in record_names:
  pattern=r'    struct '+record+r' \{.*?\n    \};'
  replacement=re.search(pattern,native,re.S).group(0);m=re.search(pattern,original,re.S);original=original[:m.start()]+replacement+original[m.end():]
 original=original.replace('#pragma once','#pragma once\n#include <aurora/ppc_bitfield.hpp>',1)
 dest=OUT/'wii-overlay/Game/Player'/filename;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(original)
# Native regression tests use the actual production types, fixed retail masks,
# and opaque whole-word copies in both directions.
source=['''#include "Game/Player/Mario.hpp"
#include "Game/Player/J3DModelX.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace {
template <typename T, typename Getter, typename Setter>
void check_field(std::size_t word, std::uint32_t mask, unsigned shift, Getter get, Setter set) {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) % sizeof(std::uint32_t) == 0);
    std::array<std::uint32_t, sizeof(T) / sizeof(std::uint32_t)> words{};
    const std::uint32_t max = mask >> shift;
    for (std::uint32_t input = 0; input <= max; ++input) {
        T flags{};
        set(flags, input);
        std::memcpy(words.data(), &flags, sizeof(flags));
        for (std::size_t i = 0; i < words.size(); ++i)
            assert(words[i] == (i == word ? input << shift : 0));

        words.fill(0xA5963C5Au);
        words[word] = (words[word] & ~mask) | (input << shift);
        std::memcpy(&flags, words.data(), sizeof(flags));
        assert(get(flags) == input);

        const auto before = words;
        set(flags, max - input);
        std::memcpy(words.data(), &flags, sizeof(flags));
        for (std::size_t i = 0; i < words.size(); ++i) {
            const auto expected = i == word ? (before[i] & ~mask) | ((max - input) << shift) : before[i];
            assert(words[i] == expected);
        }
    }
}

struct ByteFlags {
    AURORA_PPC_BITFIELD_GROUP(unsigned char, (first, 1), (mode, 2), (, 3), (last, 2))
};
struct HalfFlags {
    AURORA_PPC_BITFIELD_GROUP(unsigned short, (first, 1), (mode, 3), (, 11), (last, 1))
};
static_assert(sizeof(ByteFlags) == 1 && alignof(ByteFlags) == 1);
static_assert(sizeof(HalfFlags) == 2 && alignof(HalfFlags) == 2);
static_assert(sizeof(Mario::MovementStates) == 8 && alignof(Mario::MovementStates) == 4);
static_assert(sizeof(Mario::DrawStates) == 4 && alignof(Mario::DrawStates) == 4);
static_assert(sizeof(J3DModelX::Flags) == 4 && alignof(J3DModelX::Flags) == 4);
} // namespace

int main() {
''']
for f in fields:
 t=f['type'];field=f['field'];source += [f"    check_field<{t}>({f['word']}, {f['mask']}u, {f['shift']}, [](const auto& f) {{ return f.{field}; }}, [](auto& f, auto v) {{ f.{field} = v; }});\n"]
source += ['''    ByteFlags byte{};
    byte.first = 1;
    byte.mode = 2;
    byte.last = 3;
    std::uint8_t byte_raw;
    std::memcpy(&byte_raw, &byte, sizeof(byte));
    assert(byte_raw == 0xC3);
    byte_raw = 0x62;
    std::memcpy(&byte, &byte_raw, sizeof(byte));
    assert(byte.first == 0 && byte.mode == 3 && byte.last == 2);
    HalfFlags half{};
    half.first = 1;
    half.mode = 5;
    half.last = 1;
    std::uint16_t half_raw;
    std::memcpy(&half_raw, &half, sizeof(half));
    assert(half_raw == 0xD001);
    half_raw = 0x6000;
    std::memcpy(&half, &half_raw, sizeof(half));
    assert(half.first == 0 && half.mode == 6 && half.last == 0);
    std::puts("PASS: all Mario Movement/Draw and J3DModelX fields, bidirectional raw masks and multibit values");
    std::puts("PASS: generic 8/16/32-bit storage, padding, sizes and alignment");
}
''']
(ROOT/'tests/PpcBitfieldAbiTests.cpp').write_text(''.join(source))
