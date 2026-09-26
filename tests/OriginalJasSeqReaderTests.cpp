#include "JSystem/JAudio2/JASSeqReader.hpp"

#include <array>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void verify_byte_order() {
    std::array<u8, 9> bytes{0xa5, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    JASSeqReader reader;
    reader.init(bytes.data());
    require(reader.get16(1) == 0x1234 && reader.get32(1) == 0x12345678,
            "Sequence operands keep Wii byte order at unaligned offsets");
    require(reader.get24(0) == 0xa51234 && reader.get24(6) == 0xbcdef0,
            "24-bit operands use exactly their three bytes at either boundary");
    require(reader.readByte() == 0xa5 && reader.read16() == 0x1234 && reader.read24() == 0x56789a &&
                reader.read24() == 0xbcdef0 && reader.getCur() == bytes.data() + bytes.size(),
            "Mixed-width sequence operands advance by their encoded widths");
    std::array<u8, 3> exact{0xab, 0xcd, 0xef};
    reader.init(exact.data());
    require(reader.read24() == 0xabcdef && reader.getCur() == exact.data() + exact.size(),
            "A first 24-bit operand does not read before its resource");
}

void verify_control_flow() {
    std::array<u8, 64> bytes{};
    JASSeqReader reader;
    require(!reader.mSeqBuff && !reader.getCur() && !reader.ret() && !reader.loopEnd(),
            "An unused reader has no cursor or stack");
    reader.init(bytes.data());
    reader.jump(5U);
    require(reader.call(20) && reader.getCur() == bytes.data() + 20 && reader.getStackPtr(0) == bytes.data() + 5,
            "Calls retain the actual return cursor");
    require(reader.loopStart(2), "An authored loop shares the sequence stack");
    reader.readByte();
    require(reader.loopEnd() && reader.getCur() == bytes.data() + 20 && reader.getLoopCount() == 1,
            "The first loop end rewinds and decrements");
    reader.readByte();
    require(reader.loopEnd() && reader.getCur() == bytes.data() + 21 && reader.mNumStacks == 1,
            "The final loop end pops without rewinding");
    require(reader.ret() && reader.getCur() == bytes.data() + 5 && !reader.ret(),
            "Returning after a nested loop restores the caller");
    for (u32 index = 0; index < 8; ++index) require(reader.call(index), "Original stack capacity accepts eight calls");
    auto* cursor = reader.getCur();
    require(!reader.call(30) && !reader.loopStart(2) && reader.getCur() == cursor && !reader.getStackPtr(8),
            "Stack overflow fails without changing the cursor");
    reader.init();
    require(reader.mNumStacks == 0 && !reader.getCur(), "Reinitialization discards the prior resource and stack");
}

void verify_midi_values() {
    std::array<u8, 12> bytes{0, 0x7f, 0x81, 0, 0xff, 0xff, 0xff, 0x7f, 0x80, 0x80, 0x80, 0x80};
    JASSeqReader reader;
    reader.init(bytes.data());
    require(reader.readMidiValue() == 0 && reader.readMidiValue() == 127 && reader.readMidiValue() == 128 &&
                reader.readMidiValue() == 0x0fffffff,
            "Original variable-length operands cover one through four bytes");
    require(reader.readMidiValue() == 0 && reader.getCur() == bytes.data() + bytes.size(),
            "An overlong MIDI operand stops at the original four-byte limit");
}
}

int main() try {
    verify_byte_order();
    verify_control_flow();
    verify_midi_values();
    std::puts("PASS original JAS sequence reader: packed operands, call/loop stack and MIDI values");
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL JAS sequence reader: %s\n", error.what());
    return 1;
}
