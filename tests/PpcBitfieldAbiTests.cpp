#include "Game/Player/Mario.hpp"
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
    check_field<Mario::MovementStates>(0, 0x80000000u, 31, [](const auto& f) { return f.jumping; }, [](auto& f, auto v) { f.jumping = v; });
    check_field<Mario::MovementStates>(0, 0x40000000u, 30, [](const auto& f) { return f._1; }, [](auto& f, auto v) { f._1 = v; });
    check_field<Mario::MovementStates>(0, 0x20000000u, 29, [](const auto& f) { return f._2; }, [](auto& f, auto v) { f._2 = v; });
    check_field<Mario::MovementStates>(0, 0x10000000u, 28, [](const auto& f) { return f.turning; }, [](auto& f, auto v) { f.turning = v; });
    check_field<Mario::MovementStates>(0, 0x08000000u, 27, [](const auto& f) { return f._4; }, [](auto& f, auto v) { f._4 = v; });
    check_field<Mario::MovementStates>(0, 0x04000000u, 26, [](const auto& f) { return f._5; }, [](auto& f, auto v) { f._5 = v; });
    check_field<Mario::MovementStates>(0, 0x02000000u, 25, [](const auto& f) { return f._6; }, [](auto& f, auto v) { f._6 = v; });
    check_field<Mario::MovementStates>(0, 0x01000000u, 24, [](const auto& f) { return f._7; }, [](auto& f, auto v) { f._7 = v; });
    check_field<Mario::MovementStates>(0, 0x00800000u, 23, [](const auto& f) { return f._8; }, [](auto& f, auto v) { f._8 = v; });
    check_field<Mario::MovementStates>(0, 0x00400000u, 22, [](const auto& f) { return f._9; }, [](auto& f, auto v) { f._9 = v; });
    check_field<Mario::MovementStates>(0, 0x00200000u, 21, [](const auto& f) { return f._A; }, [](auto& f, auto v) { f._A = v; });
    check_field<Mario::MovementStates>(0, 0x00100000u, 20, [](const auto& f) { return f._B; }, [](auto& f, auto v) { f._B = v; });
    check_field<Mario::MovementStates>(0, 0x00080000u, 19, [](const auto& f) { return f._C; }, [](auto& f, auto v) { f._C = v; });
    check_field<Mario::MovementStates>(0, 0x00040000u, 18, [](const auto& f) { return f._D; }, [](auto& f, auto v) { f._D = v; });
    check_field<Mario::MovementStates>(0, 0x00020000u, 17, [](const auto& f) { return f._E; }, [](auto& f, auto v) { f._E = v; });
    check_field<Mario::MovementStates>(0, 0x00010000u, 16, [](const auto& f) { return f._F; }, [](auto& f, auto v) { f._F = v; });
    check_field<Mario::MovementStates>(0, 0x00008000u, 15, [](const auto& f) { return f._10; }, [](auto& f, auto v) { f._10 = v; });
    check_field<Mario::MovementStates>(0, 0x00004000u, 14, [](const auto& f) { return f._11; }, [](auto& f, auto v) { f._11 = v; });
    check_field<Mario::MovementStates>(0, 0x00002000u, 13, [](const auto& f) { return f._12; }, [](auto& f, auto v) { f._12 = v; });
    check_field<Mario::MovementStates>(0, 0x00001000u, 12, [](const auto& f) { return f._13; }, [](auto& f, auto v) { f._13 = v; });
    check_field<Mario::MovementStates>(0, 0x00000800u, 11, [](const auto& f) { return f._14; }, [](auto& f, auto v) { f._14 = v; });
    check_field<Mario::MovementStates>(0, 0x00000400u, 10, [](const auto& f) { return f._15; }, [](auto& f, auto v) { f._15 = v; });
    check_field<Mario::MovementStates>(0, 0x00000200u, 9, [](const auto& f) { return f.debugMode; }, [](auto& f, auto v) { f.debugMode = v; });
    check_field<Mario::MovementStates>(0, 0x00000100u, 8, [](const auto& f) { return f._17; }, [](auto& f, auto v) { f._17 = v; });
    check_field<Mario::MovementStates>(0, 0x00000080u, 7, [](const auto& f) { return f._18; }, [](auto& f, auto v) { f._18 = v; });
    check_field<Mario::MovementStates>(0, 0x00000040u, 6, [](const auto& f) { return f._19; }, [](auto& f, auto v) { f._19 = v; });
    check_field<Mario::MovementStates>(0, 0x00000020u, 5, [](const auto& f) { return f._1A; }, [](auto& f, auto v) { f._1A = v; });
    check_field<Mario::MovementStates>(0, 0x00000010u, 4, [](const auto& f) { return f._1B; }, [](auto& f, auto v) { f._1B = v; });
    check_field<Mario::MovementStates>(0, 0x00000008u, 3, [](const auto& f) { return f._1C; }, [](auto& f, auto v) { f._1C = v; });
    check_field<Mario::MovementStates>(0, 0x00000004u, 2, [](const auto& f) { return f._1D; }, [](auto& f, auto v) { f._1D = v; });
    check_field<Mario::MovementStates>(0, 0x00000002u, 1, [](const auto& f) { return f.digitalJump; }, [](auto& f, auto v) { f.digitalJump = v; });
    check_field<Mario::MovementStates>(0, 0x00000001u, 0, [](const auto& f) { return f._1F; }, [](auto& f, auto v) { f._1F = v; });
    check_field<Mario::MovementStates>(1, 0x80000000u, 31, [](const auto& f) { return f._20; }, [](auto& f, auto v) { f._20 = v; });
    check_field<Mario::MovementStates>(1, 0x40000000u, 30, [](const auto& f) { return f._21; }, [](auto& f, auto v) { f._21 = v; });
    check_field<Mario::MovementStates>(1, 0x20000000u, 29, [](const auto& f) { return f._22; }, [](auto& f, auto v) { f._22 = v; });
    check_field<Mario::MovementStates>(1, 0x10000000u, 28, [](const auto& f) { return f._23; }, [](auto& f, auto v) { f._23 = v; });
    check_field<Mario::MovementStates>(1, 0x08000000u, 27, [](const auto& f) { return f._24; }, [](auto& f, auto v) { f._24 = v; });
    check_field<Mario::MovementStates>(1, 0x04000000u, 26, [](const auto& f) { return f._25; }, [](auto& f, auto v) { f._25 = v; });
    check_field<Mario::MovementStates>(1, 0x02000000u, 25, [](const auto& f) { return f._26; }, [](auto& f, auto v) { f._26 = v; });
    check_field<Mario::MovementStates>(1, 0x01000000u, 24, [](const auto& f) { return f._27; }, [](auto& f, auto v) { f._27 = v; });
    check_field<Mario::MovementStates>(1, 0x00800000u, 23, [](const auto& f) { return f._28; }, [](auto& f, auto v) { f._28 = v; });
    check_field<Mario::MovementStates>(1, 0x00400000u, 22, [](const auto& f) { return f._29; }, [](auto& f, auto v) { f._29 = v; });
    check_field<Mario::MovementStates>(1, 0x00200000u, 21, [](const auto& f) { return f._2A; }, [](auto& f, auto v) { f._2A = v; });
    check_field<Mario::MovementStates>(1, 0x00100000u, 20, [](const auto& f) { return f._2B; }, [](auto& f, auto v) { f._2B = v; });
    check_field<Mario::MovementStates>(1, 0x00080000u, 19, [](const auto& f) { return f._2C; }, [](auto& f, auto v) { f._2C = v; });
    check_field<Mario::MovementStates>(1, 0x00040000u, 18, [](const auto& f) { return f._2D; }, [](auto& f, auto v) { f._2D = v; });
    check_field<Mario::MovementStates>(1, 0x00020000u, 17, [](const auto& f) { return f._2E; }, [](auto& f, auto v) { f._2E = v; });
    check_field<Mario::MovementStates>(1, 0x00010000u, 16, [](const auto& f) { return f._2F; }, [](auto& f, auto v) { f._2F = v; });
    check_field<Mario::MovementStates>(1, 0x00008000u, 15, [](const auto& f) { return f._30; }, [](auto& f, auto v) { f._30 = v; });
    check_field<Mario::MovementStates>(1, 0x00004000u, 14, [](const auto& f) { return f._31; }, [](auto& f, auto v) { f._31 = v; });
    check_field<Mario::MovementStates>(1, 0x00002000u, 13, [](const auto& f) { return f._32; }, [](auto& f, auto v) { f._32 = v; });
    check_field<Mario::MovementStates>(1, 0x00001000u, 12, [](const auto& f) { return f._33; }, [](auto& f, auto v) { f._33 = v; });
    check_field<Mario::MovementStates>(1, 0x00000800u, 11, [](const auto& f) { return f._34; }, [](auto& f, auto v) { f._34 = v; });
    check_field<Mario::MovementStates>(1, 0x00000400u, 10, [](const auto& f) { return f._35; }, [](auto& f, auto v) { f._35 = v; });
    check_field<Mario::MovementStates>(1, 0x00000200u, 9, [](const auto& f) { return f._36; }, [](auto& f, auto v) { f._36 = v; });
    check_field<Mario::MovementStates>(1, 0x00000100u, 8, [](const auto& f) { return f._37; }, [](auto& f, auto v) { f._37 = v; });
    check_field<Mario::MovementStates>(1, 0x00000080u, 7, [](const auto& f) { return f._38; }, [](auto& f, auto v) { f._38 = v; });
    check_field<Mario::MovementStates>(1, 0x00000040u, 6, [](const auto& f) { return f._39; }, [](auto& f, auto v) { f._39 = v; });
    check_field<Mario::MovementStates>(1, 0x00000020u, 5, [](const auto& f) { return f._3A; }, [](auto& f, auto v) { f._3A = v; });
    check_field<Mario::MovementStates>(1, 0x00000010u, 4, [](const auto& f) { return f._3B; }, [](auto& f, auto v) { f._3B = v; });
    check_field<Mario::MovementStates>(1, 0x00000008u, 3, [](const auto& f) { return f._3C; }, [](auto& f, auto v) { f._3C = v; });
    check_field<Mario::MovementStates>(1, 0x00000004u, 2, [](const auto& f) { return f._3D; }, [](auto& f, auto v) { f._3D = v; });
    check_field<Mario::MovementStates>(1, 0x00000003u, 0, [](const auto& f) { return f._3E; }, [](auto& f, auto v) { f._3E = v; });
    check_field<Mario::DrawStates>(0, 0x80000000u, 31, [](const auto& f) { return f._0; }, [](auto& f, auto v) { f._0 = v; });
    check_field<Mario::DrawStates>(0, 0x40000000u, 30, [](const auto& f) { return f._1; }, [](auto& f, auto v) { f._1 = v; });
    check_field<Mario::DrawStates>(0, 0x20000000u, 29, [](const auto& f) { return f._2; }, [](auto& f, auto v) { f._2 = v; });
    check_field<Mario::DrawStates>(0, 0x10000000u, 28, [](const auto& f) { return f._3; }, [](auto& f, auto v) { f._3 = v; });
    check_field<Mario::DrawStates>(0, 0x08000000u, 27, [](const auto& f) { return f._4; }, [](auto& f, auto v) { f._4 = v; });
    check_field<Mario::DrawStates>(0, 0x04000000u, 26, [](const auto& f) { return f._5; }, [](auto& f, auto v) { f._5 = v; });
    check_field<Mario::DrawStates>(0, 0x02000000u, 25, [](const auto& f) { return f._6; }, [](auto& f, auto v) { f._6 = v; });
    check_field<Mario::DrawStates>(0, 0x01000000u, 24, [](const auto& f) { return f._7; }, [](auto& f, auto v) { f._7 = v; });
    check_field<Mario::DrawStates>(0, 0x00800000u, 23, [](const auto& f) { return f._8; }, [](auto& f, auto v) { f._8 = v; });
    check_field<Mario::DrawStates>(0, 0x00400000u, 22, [](const auto& f) { return f._9; }, [](auto& f, auto v) { f._9 = v; });
    check_field<Mario::DrawStates>(0, 0x00200000u, 21, [](const auto& f) { return f._A; }, [](auto& f, auto v) { f._A = v; });
    check_field<Mario::DrawStates>(0, 0x00100000u, 20, [](const auto& f) { return f._B; }, [](auto& f, auto v) { f._B = v; });
    check_field<Mario::DrawStates>(0, 0x00080000u, 19, [](const auto& f) { return f._C; }, [](auto& f, auto v) { f._C = v; });
    check_field<Mario::DrawStates>(0, 0x00040000u, 18, [](const auto& f) { return f._D; }, [](auto& f, auto v) { f._D = v; });
    check_field<Mario::DrawStates>(0, 0x00020000u, 17, [](const auto& f) { return f._E; }, [](auto& f, auto v) { f._E = v; });
    check_field<Mario::DrawStates>(0, 0x00010000u, 16, [](const auto& f) { return f._F; }, [](auto& f, auto v) { f._F = v; });
    check_field<Mario::DrawStates>(0, 0x00008000u, 15, [](const auto& f) { return f._10; }, [](auto& f, auto v) { f._10 = v; });
    check_field<Mario::DrawStates>(0, 0x00004000u, 14, [](const auto& f) { return f._11; }, [](auto& f, auto v) { f._11 = v; });
    check_field<Mario::DrawStates>(0, 0x00002000u, 13, [](const auto& f) { return f.mIsUnderwater; }, [](auto& f, auto v) { f.mIsUnderwater = v; });
    check_field<Mario::DrawStates>(0, 0x00001000u, 12, [](const auto& f) { return f._13; }, [](auto& f, auto v) { f._13 = v; });
    check_field<Mario::DrawStates>(0, 0x00000800u, 11, [](const auto& f) { return f._14; }, [](auto& f, auto v) { f._14 = v; });
    check_field<Mario::DrawStates>(0, 0x00000400u, 10, [](const auto& f) { return f._15; }, [](auto& f, auto v) { f._15 = v; });
    check_field<Mario::DrawStates>(0, 0x00000200u, 9, [](const auto& f) { return f._16; }, [](auto& f, auto v) { f._16 = v; });
    check_field<Mario::DrawStates>(0, 0x00000100u, 8, [](const auto& f) { return f._17; }, [](auto& f, auto v) { f._17 = v; });
    check_field<Mario::DrawStates>(0, 0x00000080u, 7, [](const auto& f) { return f._18; }, [](auto& f, auto v) { f._18 = v; });
    check_field<Mario::DrawStates>(0, 0x00000040u, 6, [](const auto& f) { return f._19; }, [](auto& f, auto v) { f._19 = v; });
    check_field<Mario::DrawStates>(0, 0x00000020u, 5, [](const auto& f) { return f._1A; }, [](auto& f, auto v) { f._1A = v; });
    check_field<Mario::DrawStates>(0, 0x00000010u, 4, [](const auto& f) { return f._1B; }, [](auto& f, auto v) { f._1B = v; });
    check_field<Mario::DrawStates>(0, 0x00000008u, 3, [](const auto& f) { return f._1C; }, [](auto& f, auto v) { f._1C = v; });
    check_field<Mario::DrawStates>(0, 0x00000004u, 2, [](const auto& f) { return f._1D; }, [](auto& f, auto v) { f._1D = v; });
    check_field<Mario::DrawStates>(0, 0x00000002u, 1, [](const auto& f) { return f._1E; }, [](auto& f, auto v) { f._1E = v; });
    check_field<Mario::DrawStates>(0, 0x00000001u, 0, [](const auto& f) { return f._1F; }, [](auto& f, auto v) { f._1F = v; });
    check_field<J3DModelX::Flags>(0, 0x80000000u, 31, [](const auto& f) { return f._0; }, [](auto& f, auto v) { f._0 = v; });
    check_field<J3DModelX::Flags>(0, 0x40000000u, 30, [](const auto& f) { return f._1; }, [](auto& f, auto v) { f._1 = v; });
    check_field<J3DModelX::Flags>(0, 0x20000000u, 29, [](const auto& f) { return f._2; }, [](auto& f, auto v) { f._2 = v; });
    check_field<J3DModelX::Flags>(0, 0x10000000u, 28, [](const auto& f) { return f._3; }, [](auto& f, auto v) { f._3 = v; });
    check_field<J3DModelX::Flags>(0, 0x08000000u, 27, [](const auto& f) { return f._4; }, [](auto& f, auto v) { f._4 = v; });
    check_field<J3DModelX::Flags>(0, 0x04000000u, 26, [](const auto& f) { return f._5; }, [](auto& f, auto v) { f._5 = v; });
    check_field<J3DModelX::Flags>(0, 0x02000000u, 25, [](const auto& f) { return f._6; }, [](auto& f, auto v) { f._6 = v; });
    check_field<J3DModelX::Flags>(0, 0x01000000u, 24, [](const auto& f) { return f._7; }, [](auto& f, auto v) { f._7 = v; });
    check_field<J3DModelX::Flags>(0, 0x00800000u, 23, [](const auto& f) { return f._8; }, [](auto& f, auto v) { f._8 = v; });
    check_field<J3DModelX::Flags>(0, 0x00400000u, 22, [](const auto& f) { return f._9; }, [](auto& f, auto v) { f._9 = v; });
    check_field<J3DModelX::Flags>(0, 0x00200000u, 21, [](const auto& f) { return f._A; }, [](auto& f, auto v) { f._A = v; });
    check_field<J3DModelX::Flags>(0, 0x00100000u, 20, [](const auto& f) { return f._B; }, [](auto& f, auto v) { f._B = v; });
    check_field<J3DModelX::Flags>(0, 0x00080000u, 19, [](const auto& f) { return f._C; }, [](auto& f, auto v) { f._C = v; });
    check_field<J3DModelX::Flags>(0, 0x00040000u, 18, [](const auto& f) { return f._D; }, [](auto& f, auto v) { f._D = v; });
    check_field<J3DModelX::Flags>(0, 0x00020000u, 17, [](const auto& f) { return f._E; }, [](auto& f, auto v) { f._E = v; });
    check_field<J3DModelX::Flags>(0, 0x00010000u, 16, [](const auto& f) { return f._F; }, [](auto& f, auto v) { f._F = v; });
    check_field<J3DModelX::Flags>(0, 0x00008000u, 15, [](const auto& f) { return f._10; }, [](auto& f, auto v) { f._10 = v; });
    check_field<J3DModelX::Flags>(0, 0x00004000u, 14, [](const auto& f) { return f._11; }, [](auto& f, auto v) { f._11 = v; });
    check_field<J3DModelX::Flags>(0, 0x00002000u, 13, [](const auto& f) { return f._12; }, [](auto& f, auto v) { f._12 = v; });
    check_field<J3DModelX::Flags>(0, 0x00001000u, 12, [](const auto& f) { return f._13; }, [](auto& f, auto v) { f._13 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000800u, 11, [](const auto& f) { return f._14; }, [](auto& f, auto v) { f._14 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000400u, 10, [](const auto& f) { return f._15; }, [](auto& f, auto v) { f._15 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000200u, 9, [](const auto& f) { return f._16; }, [](auto& f, auto v) { f._16 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000100u, 8, [](const auto& f) { return f._17; }, [](auto& f, auto v) { f._17 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000080u, 7, [](const auto& f) { return f._18; }, [](auto& f, auto v) { f._18 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000040u, 6, [](const auto& f) { return f._19; }, [](auto& f, auto v) { f._19 = v; });
    check_field<J3DModelX::Flags>(0, 0x00000020u, 5, [](const auto& f) { return f._1A; }, [](auto& f, auto v) { f._1A = v; });
    check_field<J3DModelX::Flags>(0, 0x00000010u, 4, [](const auto& f) { return f._1B; }, [](auto& f, auto v) { f._1B = v; });
    check_field<J3DModelX::Flags>(0, 0x00000008u, 3, [](const auto& f) { return f._1C; }, [](auto& f, auto v) { f._1C = v; });
    ByteFlags byte{};
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
