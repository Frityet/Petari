#include "Game/Util/HashUtil.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>

int main() {
    static_assert(sizeof(HashSortTable::Value) == sizeof(void*));
    HashSortTable table(5);
    // HashSortTable is arena-owned in Game; this standalone probe releases its arrays.
    std::unique_ptr<u32[]> hashes(table.mHashCodes);
    std::unique_ptr<HashSortTable::Value[]> values(table._8);
    std::unique_ptr<u16[]> ranges(table._C), lengths(table._10);
    int first = 19, second = 29;
    const auto firstValue = reinterpret_cast<HashSortTable::Value>(&first);
    const auto secondValue = reinterpret_cast<HashSortTable::Value>(&second);
    assert(firstValue > UINT32_MAX && secondValue > UINT32_MAX);
    table.add(0xff000010u, firstValue);
    table.add(0x03000020u, secondValue);
    table.add(0x03000010u, static_cast<HashSortTable::Value>(0xfedcba9876543210ULL));
    table.add(0x10000000u, 7);
    assert(!table.addOrSkip(0xff000010u, 0));
    assert(table.addOrSkip(0x00000010u, 0));
    table.sort();
    HashSortTable::Value result = 0;
    assert(table.search(0xff000010u, &result) && result == firstValue);
    assert(*reinterpret_cast<int*>(result) == 19);
    assert(table.search(0x03000020u, &result) && result == secondValue);
    assert(*reinterpret_cast<int*>(result) == 29);
    assert(table.search(0x03000010u, &result) && result == 0xfedcba9876543210ULL);
    assert(table.search(0x10000000u, &result) && result == 7);
    assert(table.search(0x00000010u, &result) && result == 0);
    assert(!table.search(0x03000030u, &result) && result == 0);
    assert(table.search(0xff000010u, nullptr));
    std::puts("HashSortTable: actual 64-bit pointers, full-width scalar, zero/index payloads, sorting, duplicate skip, missing lookup passed");
}
