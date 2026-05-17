#include "Game/compat/NameObjFactoryCompat.hpp"

#include "Game/Map/FileSelector.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace smgpc::game {
    namespace {

        using Creator = std::unique_ptr<NameObj> (*)(const char *);

        struct FactoryEntry {
            std::string_view object_name;
            Creator creator = nullptr;
            std::string_view archive_name;
        };

        template <typename T>
        [[nodiscard]] std::unique_ptr<NameObj> create_typed_name_obj(const char *name) {
            return std::make_unique<T>(name);
        }

        constexpr auto FACTORY_ENTRIES = std::array{
            FactoryEntry{
                .object_name = "FileSelector",
                .creator = create_typed_name_obj<FileSelector>,
                .archive_name = {},
            },
        };

        [[nodiscard]] const FactoryEntry *find_entry(std::string_view object_name) {
            for (const auto &entry : FACTORY_ENTRIES) {
                if (entry.object_name == object_name) {
                    return &entry;
                }
            }

            return nullptr;
        }

    }  // namespace

    std::span<const NameObjFactoryEntry> name_obj_factory_entries() {
        static const auto entries = [] {
            auto result = std::array<NameObjFactoryEntry, FACTORY_ENTRIES.size()>{};
            for (auto i = std::size_t{}; i < FACTORY_ENTRIES.size(); ++i) {
                result[i] = NameObjFactoryEntry{
                    .object_name = FACTORY_ENTRIES[i].object_name,
                    .archive_name = FACTORY_ENTRIES[i].archive_name,
                };
            }

            return result;
        }();

        return entries;
    }

    bool can_create_name_obj(std::string_view object_name) {
        return find_entry(object_name) != nullptr;
    }

    std::unique_ptr<NameObj> create_name_obj(std::string_view object_name, std::string_view actor_name) {
        const auto *entry = find_entry(object_name);
        if (entry == nullptr || entry->creator == nullptr) {
            throw std::runtime_error("Unsupported NameObj factory request: " + std::string(object_name));
        }

        const auto name = std::string(actor_name);
        return entry->creator(name.c_str());
    }

#ifndef NDEBUG
    std::optional<FileSelectStageState> file_select_stage_state(const NameObj &object) {
        const auto *file_selector = dynamic_cast<const FileSelector *>(&object);
        if (file_selector == nullptr) {
            return std::nullopt;
        }

        return FileSelectStageState{
            .title_active = file_selector->isTitleActive(),
            .title_started = file_selector->isTitleStarted(),
            .title_ended = file_selector->isTitleEnded(),
            .file_select_start = file_selector->isFileSelectStart(),
            .file_select_started = file_selector->isFileSelectStarted(),
            .demo_start_wait = file_selector->didStartDemoStartWait(),
        };
    }
#endif

}  // namespace smgpc::game
