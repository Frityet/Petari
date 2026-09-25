#include "resource/TextEncoding.hpp"

#include <cstddef>
#include <cstdio>
#include <type_traits>

template <std::size_t N, std::size_t M>
consteval bool same_bytes(const char (&actual)[N], const char (&expected)[M]) {
    if constexpr (N != M) return false;
    else {
        for (std::size_t i = 0; i != N; ++i)
            if (static_cast<unsigned char>(actual[i]) != static_cast<unsigned char>(expected[i])) return false;
        return true;
    }
}

static_assert(std::is_same_v<decltype(CP932("abc")), const char (&)[4]>);
static_assert(sizeof(CP932("")) == 1);
static_assert(same_bytes(CP932("ASCII\\\n\t\""), "ASCII\\\n\t\""));
static_assert(same_bytes(CP932("マリオ" "A"), "\x83\x7d\x83\x8a\x83\x49" "A"));
static_assert(same_bytes(CP932(u8"日本語"), "\x93\xfa\x96\x7b\x8c\xea"));
static_assert(same_bytes(CP932("\u65E5\u672C\u8A9E"), "\x93\xfa\x96\x7b\x8c\xea"));
static_assert(same_bytes(CP932(R"(日本語\n)"), "\x93\xfa\x96\x7b\x8c\xea\\n"));
static_assert(same_bytes(CP932("日\0A"), "\x93\xfa\0A"));
static_assert(sizeof(CP932("日\0A")) == 5);
static_assert(same_bytes(CP932("\0\0"), "\0\0"));
static_assert(same_bytes(CP932("\xE3\x83\x9E"), "\x83\x7d"));
static_assert(same_bytes(CP932("ｱｲｳ"), "\xb1\xb2\xb3"));
static_assert(same_bytes(CP932("①髙﨑"), "\x87\x40\xee\xe0\xed\x95"));
static_assert(same_bytes(CP932("～〜∥‖−－￠¢￡£￢¬"),
                         "\x81\x60\x81\x60\x81\x61\x81\x61\x81\x7c\x81\x7c"
                         "\x81\x91\x81\x91\x81\x92\x81\x92\x81\xca\x81\xca"));
static_assert(same_bytes(CP932("\ue000\uf8f0"), "\xf0\x40\xa0"));

// Raw numeric escaped bytes stay ordinary C++ bytes when not wrapped. A CP932
// macro sees resolved literal bytes, so its input must itself be valid UTF-8.
constexpr char raw_bytes[] = "\x82\xa0";
static_assert(static_cast<unsigned char>(raw_bytes[0]) == 0x82);
static_assert(static_cast<unsigned char>(raw_bytes[1]) == 0xa0);

const char* cp932_pointer_from_second_translation_unit();
constexpr const char* global_pointer = CP932("日本語");

int main() {
    const char* saved = nullptr;
    {
        const auto& scoped = CP932("日本語");
        saved = scoped;
    }
    if (saved != global_pointer || saved != cp932_pointer_from_second_translation_unit()) return 1;
    std::puts("CP932 literals: exact bytes, array extent, embedded NULs and shared static storage passed");
}
