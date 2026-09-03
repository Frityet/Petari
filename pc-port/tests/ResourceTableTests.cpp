#include "Game/System/ResourceInfo.hpp"
#include "Game/Util/HashUtil.hpp"

#include <cstring>
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

    // Retail allocates these children in a stage arena. The fixture owns and
    // releases the same original objects without changing the Game lifecycle.
    struct TableOwner {
        explicit TableOwner(u32 capacity) { table.newFileInfoTable(capacity); }
        ~TableOwner() {
            for (u32 i = 0; i < table.mCount; ++i) {
                delete[] table.mFileInfoTable[i].mName;
            }
            delete[] table.mFileInfoTable;
        }
        ResTable table;
    };

    void names_and_metadata() {
        TableOwner owner(3);
        auto &table = owner.table;
        int motion = 7;
        auto *entry = table.add("Mario.Wait.BCK", &motion, true);
        require(std::strcmp(entry->mName, "Mario.Wait") == 0 &&
                    table.getRes("mARIO.wAIT") == &motion &&
                    table.getResIndex("Mario.Wait.BCK") == -1,
                "motion names must strip only the last extension and preserve case-insensitive lookup");
        require(entry->_4 == 0 && entry->_8 == nullptr && entry->_C == 0,
                "new resource metadata must preserve the original default state");

        const char raw[] = "archive bytes";
        entry->_8 = const_cast<char *>(raw);
        entry->_4 = sizeof(raw);
        entry->_C = 41;
        require(table.findFileInfo("MARIO.WAIT") == entry && table.getFileInfo(0) == entry &&
                    table.getRes(0U) == &motion && std::strcmp(table.getResName(0U), "Mario.Wait") == 0 &&
                    entry->_8 == raw && entry->_4 == sizeof(raw) && entry->_C == 41,
                "index/name lookup must retain both typed and raw resource identity and archive metadata");

        char source_name[] = "Raw.File.bcsv";
        table.add(source_name, const_cast<char *>(raw), false);
        source_name[0] = 'X';
        require(table.getRes("raw.file.bcsv") == raw && !table.isExistRes("raw.file") &&
                    std::strcmp(table.getResName(raw), "Raw.File.bcsv") == 0,
                "raw file names must retain their extension and owned name storage");
    }

    void absence_and_aliases() {
        TableOwner owner(4);
        auto &table = owner.table;
        int payload = 1;
        auto *null_entry = table.add("optional.bck", nullptr, true);
        require(table.isExistRes("OPTIONAL") && table.getRes("optional") == nullptr &&
                    table.findFileInfo("optional") == null_entry,
                "a registered null resource is distinct from an absent name");
        require(table.getResIndex("missing") == -1 && table.findFileInfo("missing") == nullptr &&
                    table.getRes("missing") == nullptr && !table.isExistRes("missing"),
                "missing resources must preserve the original absent results");
        table.add("first", &payload, false);
        table.add("alias", &payload, false);
        require(std::strcmp(table.findResName(&payload), "first") == 0,
                "reverse lookup must retain the first registered alias");
        int absent_payload = 2;
        require(table.findResName(&absent_payload) == nullptr && table.getResName(&absent_payload) == nullptr,
                "unregistered resource pointers must remain absent");
    }

    void retail_hash_semantics() {
        TableOwner owner(3);
        auto &table = owner.table;
        int first = 1;
        int second = 2;
        require(MR::getHashCodeLower("a~") == MR::getHashCodeLower("b_"),
                "fixture must exercise a real original 31-based hash collision");
        table.add("a~", &first, false);
        table.add("b_", &second, false);
        require(table.getRes("b_") == &first && table.getResIndex("b_") == 0,
                "retail resource lookup compares hashes and retains the first collision");

        const char encoded_name[] = {static_cast<char>(0x82), static_cast<char>(0xA0), 'A', 0};
        const char encoded_query[] = {static_cast<char>(0x82), static_cast<char>(0xA0), 'a', 0};
        table.add(encoded_name, &second, false);
        require(table.getRes(encoded_query) == &second &&
                    table.getFileInfo(2)->mHashCode == ((0x82U * 31U + 0xA0U) * 31U + 'a'),
                "MSL resource hashing must retain high bytes while folding ASCII case");
    }

}  // namespace

int main() {
    try {
        names_and_metadata();
        absence_and_aliases();
        retail_hash_semantics();
        std::cout << "3/3 original resource-table groups passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
