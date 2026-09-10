#include "JSystem/JSupport/JSUMemoryInputStream.hpp"
#include "JSystem/JSupport/JSUMemoryOutputStream.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    static_assert(std::is_same_v<decltype(JSUIosBase::mState), bool>);
    static_assert(std::is_base_of_v<JSURandomInputStream, JSUMemoryInputStream>);
    static_assert(std::is_base_of_v<JSURandomOutputStream, JSUMemoryOutputStream>);
    static_assert(std::has_virtual_destructor_v<JSUIosBase>);
    static_assert(std::is_same_v<decltype(JSUMemoryInputStream::mBuffer), const void*>);
    static_assert(std::is_same_v<decltype(JSUMemoryOutputStream::mBuffer), void*>);

    void test_retail_boolean_state() {
        JSUIosBase stream;
        require(stream.isGood(), "new stream must be good");
        stream.setState(JSUIosBase::IO_MEMORY_ERROR);
        require(!stream.isGood() && stream.mState, "retail state normalizes nonzero bits to bool");
        require(stream.getState(JSUIosBase::IO_MEMORY_ERROR) == 0,
                "retail bool state does not retain a separate memory-error bit");
        require(stream.getState(JSUIosBase::IO_ERROR) == 1, "normalized error must be observable as bit zero");
        stream.clearState(JSUIosBase::IO_OK);
        require(!stream.isGood(), "clearing zero must preserve the state");
        stream.clearState(JSUIosBase::IO_ERROR);
        require(stream.isGood(), "clearState must remove the requested bit");
    }

    void test_partial_input_preserves_destination() {
        const std::array<u8, 4> bytes{0x12, 0x34, 0x56, 0x78};
        JSUMemoryInputStream memory(bytes.data(), bytes.size());
        JSUInputStream& stream = memory;
        std::array<u8, 6> destination{0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
        require(stream.read(destination.data(), destination.size()) == 4, "short read must return actual byte count");
        require(std::memcmp(destination.data(), bytes.data(), bytes.size()) == 0, "read must copy raw bytes");
        require(destination[4] == 0xAA && destination[5] == 0xAA, "short read must preserve unread destination suffix");
        require(memory.getPosition() == 4 && memory.getAvailable() == 0 && !stream.isGood(), "short read must advance and set error");
        destination.fill(0xBB);
        require(stream.read(destination.data(), 2) == 0 && destination[0] == 0xBB && destination[1] == 0xBB,
                "EOF must preserve the caller's previous bytes");
        require(memory.seek(0, SEEK_FROM_START) == -4 && stream.isGood(), "seek clears read error and returns displacement");
        require(memory.readData(destination.data(), 6) == 4 && stream.isGood(), "raw readData must not set wrapper error state");
    }

    void test_partial_output_preserves_guards() {
        const std::array<u8, 6> source{0xF1, 0xE2, 0xD3, 0xC4, 0xB5, 0xA6};
        std::array<u8, 8> buffer{0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
        JSUMemoryOutputStream memory(buffer.data() + 2, 4);
        JSUOutputStream& stream = memory;
        require(stream.write(source.data(), source.size()) == 4, "short write must return actual byte count");
        require(std::memcmp(buffer.data() + 2, source.data(), 4) == 0, "write must copy raw bytes");
        require(buffer[0] == 0xAA && buffer[1] == 0xAA && buffer[6] == 0xAA && buffer[7] == 0xAA,
                "write must respect both buffer boundaries");
        require(memory.getPosition() == 4 && memory.getAvailable() == 0 && !stream.isGood(), "short write must advance and set error");
        require(memory.seek(0, SEEK_FROM_START) == -4 && stream.isGood(), "output seek must clear IO_ERROR");
        require(memory.writeData(source.data(), 6) == 4 && stream.isGood(), "raw writeData must not set wrapper error state");
    }

    void test_scalar_helpers_are_host_native() {
        const u8 first = 0xA5;
        const u16 second = 0x1234;
        const u32 third = 0x89ABCDEF;
        std::array<u8, 7> expected{};
        std::memcpy(expected.data(), &first, sizeof(first));
        std::memcpy(expected.data() + 1, &second, sizeof(second));
        std::memcpy(expected.data() + 3, &third, sizeof(third));
        std::array<u8, 7> storage{};
        JSUMemoryOutputStream output(storage.data(), storage.size());
        output.writeU8(first);
        output.writeU16(second);
        output.writeU32(third);
        require(output.isGood() && storage == expected, "typed writes must retain host scalar representation");
        JSUMemoryInputStream input(storage.data(), storage.size());
        require(input.readU8() == first && input.readU16() == second && input.readU32() == third,
                "typed reads must decode host representation without implicit serialized-format swaps");
        require(input.isGood() && input.getAvailable() == 0, "exact scalar reads should stay good");
    }

    void test_partial_scalar_and_raw_eof_are_distinct() {
        const u8 byte = 0x42;
        JSUMemoryInputStream input(&byte, 1);
        u16 expected = 0;
        std::memcpy(&expected, &byte, 1);
        require(input.readU16() == expected && !input.isGood(), "typed partial read defines only its own unread temporary bytes as zero");
        require(input.readU32() == 0 && !input.isGood(), "typed EOF return must be defined and retain error");
        u16 retained = 0xCAFE;
        require(input.read(&retained, sizeof(retained)) == 0 && retained == 0xCAFE,
                "raw EOF must not zero a caller's persistent value");
    }

    template <typename Stream>
    void check_seek_contract(Stream& stream) {
        require(stream.seek(4, SEEK_FROM_START) == 4 && stream.getPosition() == 4, "start seek displacement");
        require(stream.seek(-2, SEEK_FROM_POSITION) == -2 && stream.getPosition() == 2, "relative backward seek");
        require(stream.seek(2, SEEK_FROM_END) == 4 && stream.getPosition() == 6, "end seek uses length minus offset");
        require(stream.seek(-2, SEEK_FROM_END) == 2 && stream.getPosition() == 8, "end seek clamps above length");
        require(stream.seek(99, SEEK_FROM_END) == -8 && stream.getPosition() == 0, "end seek clamps below zero");
        stream.setState(JSUIosBase::IO_ERROR);
        require(stream.seek(99, SEEK_FROM_START) == 8 && stream.isGood(), "clamped seek still clears IO_ERROR");
        require(stream.seek(3, static_cast<JSUStreamSeekFrom>(3)) == 0 && stream.getPosition() == 8,
                "unknown origin preserves current position as retail switch does");
    }

    void test_seek_displacements_and_end_origin() {
        std::array<u8, 8> buffer{};
        JSUMemoryInputStream input(buffer.data(), buffer.size());
        JSUMemoryOutputStream output(buffer.data(), buffer.size());
        check_seek_contract(input);
        check_seek_contract(output);
    }

    void test_memory_skip_behavior() {
        std::array<u8, 4> buffer{1, 2, 3, 4};
        JSUMemoryInputStream input(buffer.data(), buffer.size());
        require(input.skip(3) == 3 && input.isGood(), "random input skip should advance directly");
        require(input.skip(-2) == -2 && input.getPosition() == 1 && input.isGood(), "random input skip can rewind");
        require(input.skip(8) == 3 && input.getPosition() == 4 && !input.isGood(), "clamped random skip must set IO_ERROR");
        input.seek(0, SEEK_FROM_START);
        require(input.skip(-1) == 0 && !input.isGood(), "clamped backward skip must set IO_ERROR");
        JSUMemoryOutputStream output(buffer.data(), buffer.size());
        require(output.skip(6, -1) == 4 && !output.isGood(), "output skip must fill available bytes and flag a short transfer");
        require(buffer == std::array<u8, 4>{0xFF, 0xFF, 0xFF, 0xFF}, "output skip writes the fill byte instead of merely seeking");
    }

    class SequentialInput final : public JSUInputStream {
    public:
        SequentialInput(const void* bytes, s32 size) : memory(bytes, size) {}
        s32 getAvailable() const override { return memory.getAvailable(); }
        u32 readData(void* output, s32 size) override {
            ++calls;
            return memory.readData(output, size);
        }
        JSUMemoryInputStream memory;
        s32 calls = 0;
    };

    class SequentialOutput final : public JSUOutputStream {
    public:
        SequentialOutput(void* bytes, s32 size) : memory(bytes, size) {}
        s32 writeData(const void* input, s32 size) override {
            ++calls;
            return memory.writeData(input, size);
        }
        JSUMemoryOutputStream memory;
        s32 calls = 0;
    };

    void test_sequential_virtual_skip() {
        std::array<u8, 3> buffer{1, 2, 3};
        SequentialInput input(buffer.data(), buffer.size());
        require(input.skip(5) == 3 && input.calls == 4 && !input.isGood(), "base skip must use virtual byte reads until first short transfer");
        require(input.skip(-1) == 0 && input.calls == 4, "negative sequential skip must do no reads");
        SequentialOutput output(buffer.data(), buffer.size());
        require(output.skip(5, 0x5A) == 3 && output.calls == 4 && !output.isGood(), "base output skip must use virtual byte writes");
        require(buffer == std::array<u8, 3>{0x5A, 0x5A, 0x5A}, "base output fill must reach real memory provider");
    }

    void test_set_buffer_preserves_error() {
        const std::array<u8, 2> first{1, 2};
        const std::array<u8, 3> second{3, 4, 5};
        JSUMemoryInputStream input(first.data(), first.size());
        std::array<u8, 3> destination{};
        input.read(destination.data(), 3);
        input.setBuffer(second.data(), second.size());
        require(input.getPosition() == 0 && input.getLength() == 3 && !input.isGood(), "setBuffer resets position but does not clear IO_ERROR");
        require(input.read(destination.data(), 3) == 3 && !input.isGood(), "successful transfer must not clear sticky error");
        input.seek(0, SEEK_FROM_START);
        require(input.isGood(), "seek explicitly recovers error state");
        JSUMemoryOutputStream output(destination.data(), 1);
        output.write(second.data(), 3);
        output.setBuffer(destination.data(), 3);
        require(output.getPosition() == 0 && output.getAvailable() == 3 && !output.isGood(), "output setBuffer also preserves error");
    }

    void test_invalid_native_spans() {
        u8 value = 0xAA;
        JSUMemoryInputStream input(&value, 1);
        JSUMemoryOutputStream output(&value, 1);
        require(input.read(nullptr, 1) == 0 && !input.isGood(), "null destination must not be accessed");
        require(output.write(nullptr, 1) == 0 && !output.isGood(), "null source must not be accessed");
        input.seek(0, SEEK_FROM_START);
        output.seek(0, SEEK_FROM_START);
        require(input.read(&value, -1) == 0 && !input.isGood(), "negative read must transfer nothing and report error");
        require(output.write(&value, -1) == 0 && !output.isGood(), "negative write must transfer nothing and report error");
        input.setBuffer(nullptr, -1);
        output.setBuffer(nullptr, -1);
        require(input.getAvailable() == 0 && output.getAvailable() == 0, "negative native lengths have no available span");
        require(input.read(&value, 1) == 0 && output.write(&value, 1) == 0 && value == 0xAA, "invalid native buffers must not touch memory");
    }

    void test_large_seek_arithmetic() {
        // Only seek within a declared span; no byte access uses this address.
        u8 placeholder = 0;
        constexpr s32 limit = std::numeric_limits<s32>::max();
        JSUMemoryInputStream input(&placeholder, limit);
        require(input.seek(limit - 1, SEEK_FROM_START) == limit - 1, "large start position");
        require(input.seek(limit, SEEK_FROM_POSITION) == 1 && input.getPosition() == limit,
                "relative seek must clamp without signed overflow");
        require(input.seek(std::numeric_limits<s32>::min(), SEEK_FROM_END) == 0 && input.getPosition() == limit,
                "end-minus-offset must widen before arithmetic");
        require(input.seek(std::numeric_limits<s32>::min(), SEEK_FROM_POSITION) == -limit && input.getPosition() == 0,
                "large negative seek clamps safely");
    }

    void test_polymorphic_destruction() {
        class OwnedInput final : public JSUMemoryInputStream {
        public:
            OwnedInput(bool& destroyed, const void* bytes) : JSUMemoryInputStream(bytes, 1), flag(destroyed) {}
            ~OwnedInput() override { flag = true; }
            bool& flag;
        };
        u8 value = 0x5A;
        bool destroyed = false;
        std::unique_ptr<JSUIosBase> owner = std::make_unique<OwnedInput>(destroyed, &value);
        owner.reset();
        require(destroyed, "destruction through JSUIosBase must release the actual derived stream");
    }
}

int main() {
    try {
        test_retail_boolean_state();
        test_partial_input_preserves_destination();
        test_partial_output_preserves_guards();
        test_scalar_helpers_are_host_native();
        test_partial_scalar_and_raw_eof_are_distinct();
        test_seek_displacements_and_end_origin();
        test_memory_skip_behavior();
        test_sequential_virtual_skip();
        test_set_buffer_preserves_error();
        test_invalid_native_spans();
        test_large_seek_arithmetic();
        test_polymorphic_destruction();
        std::cout << "Original JSU stream tests passed: 12/12\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Original JSU stream tests failed: " << error.what() << '\n';
        return 1;
    }
}
