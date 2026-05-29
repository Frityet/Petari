#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "DumpJson.hpp"
#include "Sqlite.hpp"

namespace smgpc::trace {

    inline constexpr std::string_view kTraceSqliteSchema = "smgpc-trace-sqlite-v1";

    struct TraceWriteResult {
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

    void create_trace_sqlite_schema(sql::Database &db);
    [[nodiscard]] std::vector<std::int64_t> trace_ids(sql::Database &db);
    [[nodiscard]] dump::Json load_trace_json_from_database(sql::Database &db, std::int64_t trace_id);
    [[nodiscard]] TraceWriteResult write_trace_json_to_database(sql::Database &db, const std::filesystem::path &source_path,
                                                                const dump::Json &trace, std::optional<std::string> emulator = {});
    [[nodiscard]] TraceWriteResult copy_trace_between_databases(sql::Database &destination, sql::Database &source,
                                                               std::int64_t source_trace_id);
    [[nodiscard]] TraceWriteResult write_trace_sqlite_file(const std::filesystem::path &path, const dump::Json &trace,
                                                          std::optional<std::string> emulator = {});
    [[nodiscard]] dump::Json load_trace_sqlite_file(const std::filesystem::path &path);

}  // namespace smgpc::trace
