#include "Game/Util/HashUtil.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    struct TableOwner {
        explicit TableOwner(u32 capacity) : table(capacity) {}

        // Retail tables live in an owning heap. The fixture owns the exact
        // constructor allocations individually and never copies their pointers.
        ~TableOwner() {
            delete[] table.mHashCodes;
            delete[] table._8;
            delete[] table._C;
            delete[] table._10;
        }

        TableOwner(const TableOwner&) = delete;
        TableOwner& operator=(const TableOwner&) = delete;

        HashSortTable table;
    };

    void test_unsigned_buckets_retain_values_after_sort() {
        struct Entry {
            u32 hash;
            u32 value;
        };
        const auto entries = std::array{
            Entry{0xFF000050, 501}, Entry{0x80000020, 802}, Entry{0x00000002, 3},
            Entry{0x7F000030, 734}, Entry{0xFFFFFFFF, 955}, Entry{0x00000000, 6},
            Entry{0x80000001, 87}, Entry{0x7F000010, 718}, Entry{0x01000000, 109},
        };
        TableOwner owner(entries.size());
        auto& table = owner.table;
        require(!table.mHasBeenSorted && table.mCurrentLength == 0,
                "the original constructor must start with no registered hashes");
        for (const auto& entry : entries) {
            require(table.add(entry.hash, entry.value), "each original insertion must succeed");
        }
        table.sort();
        require(table.mHasBeenSorted && table.mCurrentLength == entries.size(),
                "sorting must preserve the number of registered hashes");
        for (const auto& entry : entries) {
            u32 result = 0;
            require(table.search(entry.hash, &result) && result == entry.value,
                    "unsigned sorting must retain the original value for every hash");
            require(table.search(entry.hash, nullptr), "lookup must allow an omitted result pointer");
        }

        for (const u32 missing : {0x00000001U, 0x02000000U, 0x7F000020U, 0x80000021U, 0xFF000051U}) {
            u32 result = 42;
            require(!table.search(missing, &result) && result == 0,
                    "an absent hash must reset the result in populated and empty buckets");
        }
    }

    void test_duplicate_names_composite_lookup_and_resort() {
        TableOwner owner(8);
        auto& table = owner.table;
        require(table.add("Run", 31, true), "first named insertion must succeed");
        require(!table.add("Run", 99, true), "addOrSkip must reject a duplicate without replacing its value");
        require(table.add("Landing", 41, false), "a second named animation must be registered");
        require(table.add(MR::getHashCode("Lower") + MR::getHashCode("Upper"), 51),
                "a composite hash must be registered through the unsigned overload");
        table.sort();
        u32 result = 0;
        require(table.search("Run", &result) && result == 31, "a rejected duplicate must retain the first payload");
        require(table.search("Lower", "Upper", &result) && result == 51,
                "composite lookup must resolve the registered pair of names");

        table.swap("Landing", "LandingEnd");
        require(table.add("Wait", 61, true), "the owner may register another entry before its next sort");
        table.sort();
        require(!table.search("Landing", &result) && result == 0, "a replaced name must disappear after re-sorting");
        require(table.search("LandingEnd", &result) && result == 41, "renaming must preserve the registered payload");
        require(table.search("Wait", &result) && result == 61, "re-sorting must include newly registered hashes");
        require(table.search("Run", &result) && result == 31, "re-sorting must retain earlier unrelated entries");
    }

}  // namespace

int main() {
    try {
        test_unsigned_buckets_retain_values_after_sort();
        test_duplicate_names_composite_lookup_and_resort();
        std::cout << "HashSortTable tests passed (2 groups).\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "HashSortTable tests failed: " << error.what() << '\n';
        return 1;
    }
}
