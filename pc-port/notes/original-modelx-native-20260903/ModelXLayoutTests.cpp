#include "Game/Player/J3DModelX.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

static_assert(sizeof(J3DModelX::Flags) == 4);
static_assert(std::is_same_v<decltype(J3DModelX::_128), void*>);
static_assert(std::is_same_v<decltype(J3DModelX::_12C), J3DModel*>);
static_assert(std::is_same_v<decltype(J3DModelX::_1B4), u8*>);
static_assert(std::is_same_v<decltype(J3DModelX::_1C4), u8**>);
int main() {
    J3DModelX::Flags flags;
    std::uint32_t word;
    flags.clear(); flags._0 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x80000000U);
    flags.clear(); flags._1 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x40000000U);
    flags.clear(); flags._2 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x20000000U);
    flags.clear(); flags._3 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x10000000U);
    flags.clear(); flags._4 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x08000000U);
    flags.clear(); flags._5 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x04000000U);
    flags.clear(); flags._6 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x02000000U);
    flags.clear(); flags._7 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x01000000U);
    flags.clear(); flags._8 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00800000U);
    flags.clear(); flags._9 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00400000U);
    flags.clear(); flags._A = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00200000U);
    flags.clear(); flags._B = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00100000U);
    flags.clear(); flags._C = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00080000U);
    flags.clear(); flags._D = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00040000U);
    flags.clear(); flags._E = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00020000U);
    flags.clear(); flags._F = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00010000U);
    flags.clear(); flags._10 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00008000U);
    flags.clear(); flags._11 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00004000U);
    flags.clear(); flags._12 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00002000U);
    flags.clear(); flags._13 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00001000U);
    flags.clear(); flags._14 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000800U);
    flags.clear(); flags._15 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000400U);
    flags.clear(); flags._16 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000200U);
    flags.clear(); flags._17 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000100U);
    flags.clear(); flags._18 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000080U);
    flags.clear(); flags._19 = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000040U);
    flags.clear(); flags._1A = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000020U);
    flags.clear(); flags._1B = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000010U);
    flags.clear(); flags._1C = true; std::memcpy(&word, &flags, sizeof(word)); assert(word == 0x00000008U);
    std::puts("ModelX: all 29 retail bitfield masks and four pointer types pass");
}
