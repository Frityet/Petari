#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Sqlite.hpp"

namespace smgpc::trace {

    struct TraceSummary {
        std::int64_t trace_id = 0;
        std::filesystem::path path;
        std::optional<std::string> emulator;
        std::optional<std::int64_t> requested_frame;
        std::optional<std::int64_t> frame_index;
        std::optional<std::int64_t> framebuffer_width;
        std::optional<std::int64_t> framebuffer_height;
        std::int64_t record_count = 0;
        std::int64_t render_packet_count = 0;
        std::int64_t copy_event_count = 0;
    };

    struct PacketSignatureDiff {
        std::string signature;
        std::int64_t reference_count = 0;
        std::int64_t candidate_count = 0;
    };

    struct CopyKindDiff {
        std::string kind;
        std::int64_t reference_count = 0;
        std::int64_t candidate_count = 0;
    };

    [[nodiscard]] std::vector<TraceSummary> load_trace_summaries(sql::Database &db);
    [[nodiscard]] std::optional<std::int64_t> first_trace_id_for_emulator(sql::Database &db, std::string_view emulator);
    [[nodiscard]] std::vector<PacketSignatureDiff> load_packet_signature_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                                                std::int64_t candidate_trace_id, std::int64_t limit);
    [[nodiscard]] std::vector<CopyKindDiff> load_copy_kind_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                                 std::int64_t candidate_trace_id);

}  // namespace smgpc::trace
