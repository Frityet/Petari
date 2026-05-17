#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string_view>

class NameObj;

namespace smgpc::game {

    struct NameObjFactoryEntry {
        std::string_view object_name;
        std::string_view archive_name;
    };

#ifndef NDEBUG
    struct FileSelectStageState {
        bool title_active = false;
        bool title_started = false;
        bool title_ended = false;
        bool file_select_start = false;
        bool file_select_started = false;
        bool demo_start_wait = false;
    };
#endif

    [[nodiscard]] std::span<const NameObjFactoryEntry> name_obj_factory_entries();
    [[nodiscard]] bool can_create_name_obj(std::string_view object_name);
    [[nodiscard]] std::unique_ptr<NameObj> create_name_obj(std::string_view object_name, std::string_view actor_name);
#ifndef NDEBUG
    [[nodiscard]] std::optional<FileSelectStageState> file_select_stage_state(const NameObj &object);
#endif

}  // namespace smgpc::game
