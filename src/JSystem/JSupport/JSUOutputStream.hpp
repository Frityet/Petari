#pragma once

#include "JSystem/JSupport/JSUIosBase.hpp"
#include <aurora/endian.hpp>
#include <array>

class JSUOutputStream : public JSUIosBase {
public:
    JSUOutputStream() {
    }

    virtual ~JSUOutputStream();
    virtual s32 skip(s32, s8);
    virtual s32 writeData(const void*, s32) = 0;

    s32 write(const void*, s32);

    inline void writeU8(u8 val) {
        write(&val, sizeof(u8));
    }

    inline void writeU16(u16 val) {
        write(&val, sizeof(u16));
    }

    inline void writeU32(u32 val) {
        write(&val, sizeof(u32));
    }

    template <aurora::endian::Scalar T>
    s32 writeBig(T value) {
        auto bytes = std::bit_cast<std::array<u8, sizeof(T)>>(value);
        if constexpr (std::endian::native == std::endian::little) {
            for (unsigned i = 0; i < sizeof(T) / 2; ++i) {
                auto byte = bytes[i];
                bytes[i] = bytes[sizeof(T) - i - 1];
                bytes[sizeof(T) - i - 1] = byte;
            }
        }
        return write(bytes.data(), bytes.size());
    }

    // TODO: probably a lot of other helpers for different types
};
