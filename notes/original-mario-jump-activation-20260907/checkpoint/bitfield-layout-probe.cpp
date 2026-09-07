#include "Game/Player/Mario.hpp"
#include <cstdio>
#include <cstring>

int main() {
    Mario::MovementStates movement = {};
    movement.jumping = 1;
    u32 word = 0;
    std::memcpy(&word, &movement, sizeof(word));
    std::printf("jumping word = 0x%08x; retail mask = 0x80000000\n", word);
    Mario::DrawStates draw = {};
    draw._5 = 1;
    std::memcpy(&word, &draw, sizeof(word));
    std::printf("draw._5 word = 0x%08x; retail mask = 0x04000000\n", word);
    return 0;
}
