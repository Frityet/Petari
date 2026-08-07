#include "scene/nameobj/ObjectNameTable.hpp"

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"

#include <stdexcept>

namespace smgpc::scene::nameobj {
    namespace {

        constexpr auto cObjectNameTableArchivePath = std::string_view{"/StageData/ObjNameTable.arc"};
        constexpr auto cObjectNameTableFileName = std::string_view{"ObjNameTable.tbl"};

        [[nodiscard]] bool is_string_field(smgpc::resource::BcsvFieldType type) {
            return type == smgpc::resource::BcsvFieldType::InlineString ||
                   type == smgpc::resource::BcsvFieldType::StringOffset;
        }

        [[nodiscard]] std::size_t require_string_field(const smgpc::resource::BcsvTable &table,
                                                       std::string_view field_name) {
            const auto index = table.field_index(smgpc::resource::jmap_hash(field_name));
            if (!index.has_value()) {
                throw std::runtime_error("ObjNameTable.tbl is missing required field " + std::string(field_name));
            }
            if (!is_string_field(table.fields()[*index].type)) {
                throw std::runtime_error("ObjNameTable.tbl field " + std::string(field_name) +
                                         " must be a string field");
            }
            return *index;
        }

    }  // namespace

    ObjectNameTable::ObjectNameTable(smgpc::runtime::DvdFileSystemService &dvd)
        : ObjectNameTable(dvd.archive(cObjectNameTableArchivePath)) {
    }

    ObjectNameTable::ObjectNameTable(const smgpc::resource::RarcArchive &archive) {
        const auto *entry = archive.find_by_basename(cObjectNameTableFileName);
        if (entry == nullptr) {
            throw std::runtime_error("ObjNameTable.arc does not contain ObjNameTable.tbl");
        }

        const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry));
        (void)require_string_field(table, "en_name");
        (void)require_string_field(table, "jp_name");

        _entries.reserve(table.entry_count());
        _indices.reserve(table.entry_count());
        for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
            const auto english_name = table.get_string(row, smgpc::resource::jmap_hash("en_name"));
            const auto japanese_name = table.get_string(row, smgpc::resource::jmap_hash("jp_name"));
            if (!english_name.has_value() || !japanese_name.has_value()) {
                throw std::runtime_error("ObjNameTable.tbl row " + std::to_string(row) +
                                         " does not contain both object names");
            }

            if (_indices.contains(*english_name)) {
                continue;
            }

            const auto index = _entries.size();
            _entries.push_back(Entry{
                .english_name = *english_name,
                .japanese_name = smgpc::resource::decode_cp932(*japanese_name),
            });
            _indices.emplace(_entries.back().english_name, index);
        }
    }

    const std::string *ObjectNameTable::lookup(std::string_view english_name) const {
        const auto found = _indices.find(std::string(english_name));
        return found == _indices.end() ? nullptr : &_entries[found->second].japanese_name;
    }

    std::string_view ObjectNameTable::lookup_or_self(std::string_view english_name) const {
        const auto *japanese_name = lookup(english_name);
        return japanese_name != nullptr ? std::string_view(*japanese_name) : english_name;
    }

    std::size_t ObjectNameTable::size() const {
        return _entries.size();
    }

}  // namespace smgpc::scene::nameobj
