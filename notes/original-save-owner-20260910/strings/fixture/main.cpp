#include "Game/Literals.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>
#pragma pack(push, 1)
struct Packed { char a; int b; };
#pragma pack(pop)
int main() {
    assert(std::strcmp(game_plain(), "\213\244") == 0);
    assert(std::strcmp(game_stringized(), "\"\213\244\"") == 0);
    assert(game_wide_prefix()[0] == 0x5171);
    assert(game_wide_concat()[0] == 0x65e5 && game_wide_concat()[1] == 0x5171);
    assert(game_utf8_prefix()[0] == char8_t(0xe5));
    assert(std::strcmp(GAME_NARROW, "\213\244") == 0);
    assert(std::strcmp(game_macro(), "\213\244") == 0);
    assert(static_cast<unsigned char>("共"[0]) == 0xe5);
    assert(static_cast<unsigned char>(STRINGIFY("共")[1]) == 0xe5);
    assert(sizeof(Packed) == 5);
    std::printf("CP932=%02x%02x macro_stringize=pass prefixes=pass host_UTF8=pass path=%s\n",
                static_cast<unsigned char>(game_plain()[0]), static_cast<unsigned char>(game_plain()[1]), game_path());
}
