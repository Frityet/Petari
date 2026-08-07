#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace smgpc::resource {
    class RarcArchive;
}  // namespace smgpc::resource

namespace smgpc::runtime {
    class DvdFileSystemService;
}  // namespace smgpc::runtime

namespace smgpc::scene::nameobj {

    class ObjectNameTable final {
    public:
        explicit ObjectNameTable(smgpc::runtime::DvdFileSystemService &dvd);
        explicit ObjectNameTable(const smgpc::resource::RarcArchive &archive);

        [[nodiscard]] const std::string *lookup(std::string_view english_name) const;
        [[nodiscard]] std::string_view lookup_or_self(std::string_view english_name) const;
        [[nodiscard]] std::size_t size() const;

    private:
        struct Entry {
            std::string english_name;
            std::string japanese_name;
        };

        std::vector<Entry> _entries;
        std::unordered_map<std::string, std::size_t> _indices;
    };

}  // namespace smgpc::scene::nameobj
