#include <aurora/endian.hpp>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
int main() {
    using Word = aurora::endian::BigEndian<std::uint16_t>;
    static_assert(std::is_trivially_copyable_v<Word>);
    static_assert(std::is_trivially_default_constructible_v<Word>);
    std::array<std::uint8_t, 5> unaligned{0xDC, 0, 0, 0xBA, 0x98};
    for (std::uint32_t value = 0; value <= 65535; ++value) {
        Word word;
        word = static_cast<std::uint16_t>(value);
        assert(word.bytes[0] == value / 256 && word.bytes[1] == value % 256);
        assert(static_cast<std::uint16_t>(word) == value);
        aurora::endian::write_u16(unaligned.data() + 1, value);
        assert(unaligned[0] == 0xDC && unaligned[3] == 0xBA && unaligned[4] == 0x98);
        assert(aurora::endian::read_u16(unaligned.data() + 1) == value);
        assert(aurora::endian::read_u32(unaligned.data() + 1) == (value << 16 | 0xBA98));
    }
    Word texture[2];
    texture[0] = 255;
    texture[1] = 0x832A;
    const std::uint8_t expected[] = {0, 255, 0x83, 0x2A};
    assert(std::memcmp(texture, expected, sizeof(expected)) == 0);
}
