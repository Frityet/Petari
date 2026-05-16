#pragma once

#include "compat/Types.hpp"

namespace MR {

class BitArray {
public:
    BitArray(int bitNum);
    ~BitArray();

    bool isOn(int bitIdx) const;
    void set(int bitIdx, bool isOn);

    int size() const {
        return mArraySize;
    }

private:
    /* 0x00 */ u8 *mArray;
    /* 0x08 */ s32 mArraySize;
};

}  // namespace MR
