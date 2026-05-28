#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "DumpJson.hpp"
#include "Sqlite.hpp"

namespace smgpc::trace {

    inline constexpr std::string_view kTraceNdjsonSchema = "smgpc-trace-ndjson-v1";

    struct ImportResult {
        std::int64_t trace_id = 0;
        std::size_t record_count = 0;
        std::size_t render_packet_count = 0;
        std::size_t copy_event_count = 0;
        std::size_t semantic_event_count = 0;
        std::size_t layout_runtime_count = 0;
        std::size_t layout_pane_count = 0;
        std::size_t layout_material_count = 0;
        std::size_t layout_texture_count = 0;
    };

    [[nodiscard]] std::vector<dump::Json> trace_ndjson_records_from_json(const dump::Json &trace, std::optional<std::string> emulator = {});
    [[nodiscard]] dump::Json trace_json_from_ndjson_records(std::span<const dump::Json> records);
    void write_trace_ndjson_file(const std::filesystem::path &path, const dump::Json &trace, std::optional<std::string> emulator = {});
    [[nodiscard]] dump::Json load_trace_ndjson_file(const std::filesystem::path &path);

    void create_trace_sqlite_schema(sql::Database &db);
    [[nodiscard]] ImportResult import_trace_ndjson_file(sql::Database &db, const std::filesystem::path &path);

}  // namespace smgpc::trace
