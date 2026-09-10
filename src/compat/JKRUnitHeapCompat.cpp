#include "JSystem/JKernel/JKRUnitHeap.hpp"
#include "compat/JkrDiagnostics.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>

namespace {
    constexpr std::uintptr_t align_up(std::uintptr_t value, std::uintptr_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
    constexpr u8 unit_mask(u32 index) { return static_cast<u8>(0x80U >> (index & 7)); }
    u32 native_alignment(u32 alignment) {
        alignment = std::max<u32>(alignment == 0 ? 4 : alignment, alignof(void*));
        return (alignment & (alignment - 1)) == 0 ? alignment : 0;
    }
    struct HeapLock {
        JKRUnitHeap& heap;
        explicit HeapLock(JKRUnitHeap& value) : heap(value) { heap.lock(); }
        ~HeapLock() { heap.unlock(); }
    };
}

u32 JKRUnitHeap::calcHeapSize(u32 unitSize, u32 unitCount, u32 alignment) {
    alignment = native_alignment(alignment);
    if (alignment == 0 || unitCount == 0) return 0;
    const auto unit = align_up(std::max(unitSize, alignment), alignment);
    const auto bitmap = align_up((std::uint64_t(unitCount) + 7) / 8, 4);
    const auto size = align_up(sizeof(JKRUnitHeap) + bitmap, alignment) + unit * unitCount;
    return size <= std::numeric_limits<s32>::max() ? static_cast<u32>(size) : 0;
}

JKRUnitHeap* JKRUnitHeap::create(u32 unitSize, u32 size, u32 alignment, JKRHeap* parent, bool error) {
    alignment = native_alignment(alignment);
    if (alignment == 0) return nullptr;
    if (parent == nullptr) parent = sRootHeap;
    if (parent == nullptr) return nullptr;
    const auto unit = align_up(std::max(unitSize, alignment), alignment);
    if (unit > std::numeric_limits<s32>::max()) return nullptr;
    if (size == UINT32_MAX) size = parent->getMaxAllocatableSize(alignment);
    if (size <= sizeof(JKRUnitHeap) || size > std::numeric_limits<s32>::max()) return nullptr;
    auto* memory = static_cast<u8*>(JKRHeap::alloc(size, alignment, parent));
    if (memory == nullptr) return nullptr;
    // Retail stores one MSB-first occupancy bit per unit before the data.
    // Account for the native header size and host pointer alignment.
    auto* bitmap = memory + sizeof(JKRUnitHeap);
    const auto estimatedCount = (std::uint64_t(size - sizeof(JKRUnitHeap)) * 8) / (unit * 8 + 1);
    const auto bitmapSize = align_up((estimatedCount + 7) / 8, 4);
    auto* data = reinterpret_cast<u8*>(align_up(reinterpret_cast<std::uintptr_t>(bitmap) + bitmapSize, alignment));
    if (data >= memory + size || std::size_t(memory + size - data) < unit) {
        JKRHeap::free(memory, parent);
        return nullptr;
    }
    const u32 count = static_cast<u32>((memory + size - data) / unit);
    return new (memory) JKRUnitHeap(bitmap, data, static_cast<u32>(unit), count,
                                  static_cast<u32>(memory + size - data), alignment, parent, error);
}

JKRUnitHeap::JKRUnitHeap(u8* bitmap, u8* data, u32 unitSize, u32 unitCount, u32,
                       u32 alignment, JKRHeap* parent, bool error)
    : JKRHeap(data, unitSize * unitCount, parent, error), _6C(unitSize), _70(unitCount),
      _74(alignment), _78(bitmap), _7C(data), mTotalFreeSize(unitSize * unitCount), _84(1) {
    clearBatArea();
}
JKRUnitHeap::~JKRUnitHeap() { dispose(); }
void JKRUnitHeap::do_destroy() {
    if (auto* parent = getParent()) {
        this->~JKRUnitHeap();
        JKRHeap::free(this, parent);
    }
}
void JKRUnitHeap::clearBatArea() {
    // Padding bits are occupied, including any native alignment padding.
    std::memset(_78, 0xff, _7C - _78);
    for (u32 index = 0; index < _70; ++index) _78[index / 8] &= ~unit_mask(index);
}
s32 JKRUnitHeap::find1FreeBlock(int direction) {
    if (direction >= 0) {
        for (u32 index = 0; index < _70; ++index) if (!isUnitUsed(index)) return index;
    } else {
        for (u32 index = _70; index != 0; --index) if (!isUnitUsed(index - 1)) return index - 1;
    }
    return -1;
}
s32 JKRUnitHeap::findFreeBlock(int direction, u32 count) {
    if (count == 1) return find1FreeBlock(direction);
    return direction >= 0 ? findFreeBlock_fromHead(count) : findFreeBlock_fromTail(count);
}
s32 JKRUnitHeap::findFreeBlock_fromHead(u32 count) {
    s32 bestStart = -1;
    u32 bestLength = count == 0 ? 0 : UINT32_MAX;
    for (u32 index = 0; index < _70;) {
        if (isUnitUsed(index)) { ++index; continue; }
        const u32 start = index;
        while (index < _70 && !isUnitUsed(index)) ++index;
        const u32 length = index - start;
        if (count == 0) {
            if (length > bestLength) { bestLength = length; bestStart = start; }
        } else if (length == count || (length > count && _84 == 1)) {
            return start;
        } else if (length > count && length < bestLength) {
            bestLength = length;
            bestStart = start;
        }
    }
    // This SDK method returns a length for the zero-count query.
    return count == 0 ? static_cast<s32>(bestLength) : bestStart;
}
s32 JKRUnitHeap::findFreeBlock_fromTail(u32 count) {
    s32 bestStart = -1;
    u32 bestLength = UINT32_MAX;
    for (u32 index = _70; index != 0;) {
        if (isUnitUsed(index - 1)) { --index; continue; }
        const u32 end = index;
        while (index != 0 && !isUnitUsed(index - 1)) --index;
        const u32 length = end - index;
        if (length == count || (length > count && _84 == 1)) return end - count;
        if (length > count && length < bestLength) {
            bestLength = length;
            bestStart = end - count;
        }
    }
    return bestStart;
}
void* JKRUnitHeap::do_alloc(u32 size, int direction) {
    HeapLock lock(*this);
    const u32 count = size <= _6C ? 1 : static_cast<u32>((std::uint64_t(size) + _6C - 1) / _6C);
    const s32 index = findFreeBlock(direction, count);
    if (index < 0) return nullptr;
    for (u32 offset = 0; offset < count; ++offset) setUnitUsed(index + offset);
    mTotalFreeSize -= _6C * count;
    return indexToAddress(index);
}
void JKRUnitHeap::do_free(void* memory) {
    HeapLock lock(*this);
    const s32 index = addressToIndex(memory);
    if (index >= 0 && isUnitUsed(index)) {
        _78[index / 8] &= ~unit_mask(index);
        // Galaxy's Overwrite.cpp frees one unit, even for a multi-unit alloc.
        mTotalFreeSize += _6C;
    }
}
void JKRUnitHeap::do_freeAll() {
    HeapLock lock(*this);
    clearBatArea();
    mTotalFreeSize = _6C * _70;
}
void JKRUnitHeap::do_freeTail() {}
void JKRUnitHeap::do_fillFreeArea() {}
s32 JKRUnitHeap::do_resize(void*, u32) { return -1; }
s32 JKRUnitHeap::do_getSize(void* memory) { return isUnitUsed(addressToIndex(memory)) ? _6C : 0; }
s32 JKRUnitHeap::do_getFreeSize() { return _6C * findFreeBlock_fromHead(0); }
void* JKRUnitHeap::do_getMaxFreeBlock() { return indexToAddress(findFreeBlock_fromHead(0)); }
s32 JKRUnitHeap::do_getTotalFreeSize() { return mTotalFreeSize; }
void* JKRUnitHeap::indexToAddress(int index) {
    return index < 0 || static_cast<u32>(index) >= _70 ? nullptr : _7C + std::size_t(_6C) * index;
}
s32 JKRUnitHeap::addressToIndex(void* memory) {
    const auto address = reinterpret_cast<std::uintptr_t>(memory);
    const auto start = reinterpret_cast<std::uintptr_t>(_7C);
    if (address < start || address - start >= std::size_t(_6C) * _70 || (address - start) % _6C != 0) return -1;
    return static_cast<s32>((address - start) / _6C);
}
bool JKRUnitHeap::isUnitUsed(int index) const {
    return index >= 0 && static_cast<u32>(index) < _70 && (_78[index / 8] & unit_mask(index)) != 0;
}
void JKRUnitHeap::setUnitUsed(int index) {
    if (index >= 0 && static_cast<u32>(index) < _70) _78[index / 8] |= unit_mask(index);
}
bool JKRUnitHeap::check() {
    HeapLock lock(*this);
    u32 freeUnits = 0;
    for (u32 index = 0; index < _70; ++index) if (!isUnitUsed(index)) ++freeUnits;
    return mTotalFreeSize == freeUnits * _6C && mSize == _6C * _70;
}
bool JKRUnitHeap::dump() {
    const bool valid = check();
    smgpc::compat::jkr_report_f("JKRUnitHeap: %u units of %u bytes, %d free bytes\n", _70, _6C, mTotalFreeSize);
    return valid;
}
void JKRUnitHeap::state_register(TState* state, u32 id) const {
    state->mId = id;
    state->mUsedSize = mSize - mTotalFreeSize;
    state->mCheckCode = 0;
    for (auto* word = _78; word < _7C; word += 4)
        state->mCheckCode += (u32(word[0]) << 24) | (u32(word[1]) << 16) | (u32(word[2]) << 8) | word[3];
}
bool JKRUnitHeap::state_compare(const TState& left, const TState& right) const {
    return left.mCheckCode == right.mCheckCode && left.mUsedSize == right.mUsedSize;
}
u32 JKRUnitHeap::getHeapType() { return 0x554e4954; }
