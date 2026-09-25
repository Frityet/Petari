#pragma once

#include "JSystem/JSupport/JSUIosBase.hpp"
#include <aurora/endian.hpp>
#include <array>

class JSUInputStream : public JSUIosBase {
public:
    JSUInputStream() : JSUIosBase() {
    }

    virtual ~JSUInputStream();
    virtual s32 getAvailable() const = 0;
    virtual s32 skip(s32);
    virtual u32 readData(void*, s32) = 0;

    s32 read(void*, s32);

    // The native stream defines otherwise-indeterminate unread scalar bytes.
    // Raw read() still leaves the caller's unread bytes untouched.
    inline u8 readU8() {
        u8 ret = 0;
        read(&ret, sizeof(u8));
        return ret;
    }

    inline u16 readU16() {
        u16 ret = 0;
        read(&ret, sizeof(u16));
        return ret;
    }

    inline u32 readU32() {
        u32 ret = 0;
        read(&ret, sizeof(u32));
        return ret;
    }

    // Partial reads replace the most significant bytes and retain the caller's
    // unread bytes, matching raw reads on the original big-endian CPU.
    template <aurora::endian::Scalar T>
    s32 readBig(T& value) {
        std::array<u8, sizeof(T)> bytes = std::bit_cast<std::array<u8, sizeof(T)>>(value);
        if constexpr (std::endian::native == std::endian::little) {
            for (unsigned i = 0; i < sizeof(T) / 2; ++i) {
                auto byte = bytes[i];
                bytes[i] = bytes[sizeof(T) - i - 1];
                bytes[sizeof(T) - i - 1] = byte;
            }
        }
        const auto count = read(bytes.data(), bytes.size());
        value = aurora::endian::read_big<T>(bytes.data());
        return count;
    }

    template <aurora::endian::Scalar T>
    T readBig() {
        T value = 0;
        readBig(value);
        return value;
    }

    // TODO: probably a lot of other helpers for different types
};
