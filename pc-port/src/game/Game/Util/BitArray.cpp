#include "Game/Util/BitArray.hpp"

#include "Game/Util/MemoryUtil.hpp"

namespace MR {

BitArray::BitArray(int bitNum) : mArray(nullptr), mArraySize(bitNum) {
    const int byteNum = (bitNum + 7 & ~7) / 8;
    mArray = new u8[byteNum];
    MR::zeroMemory(mArray, static_cast< u32 >(byteNum));
}

BitArray::~BitArray() {
    delete[] mArray;
}

bool BitArray::isOn(int bitIdx) const {
    const u8 byte = mArray[bitIdx / 8];
    return (byte & (1 << (bitIdx & 0x7))) != 0;
}

void BitArray::set(int bitIdx, bool isOn) {
    const int byteIdx = bitIdx / 8;

    if (isOn) {
        mArray[byteIdx] |= static_cast< u8 >(1 << (bitIdx & 0x7));
    } else {
        mArray[byteIdx] &= static_cast< u8 >(~(1 << (bitIdx & 0x7)));
    }
}

}  // namespace MR
