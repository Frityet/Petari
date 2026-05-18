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
        std::int64_t semantic_event_count = 0;
        std::int64_t layout_runtime_count = 0;
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

    struct SemanticAnchorAlignment {
        std::string category;
        std::string name;
        std::optional<std::int64_t> reference_frame_index;
        std::optional<std::int64_t> candidate_frame_index;
        std::optional<std::int64_t> frame_delta;
        std::int64_t reference_count = 0;
        std::int64_t candidate_count = 0;
        std::optional<std::string> reference_stage;
        std::optional<std::string> candidate_stage;
        std::optional<std::string> reference_detail;
        std::optional<std::string> candidate_detail;
    };

    struct LayoutRuntimeDiff {
        std::string name;
        std::string layout_name;
        std::int64_t reference_layout_count = 0;
        std::int64_t candidate_layout_count = 0;
        std::int64_t reference_dead_count = 0;
        std::int64_t candidate_dead_count = 0;
        std::int64_t reference_suspended_count = 0;
        std::int64_t candidate_suspended_count = 0;
        std::int64_t reference_pane_count = 0;
        std::int64_t candidate_pane_count = 0;
        std::int64_t reference_material_count = 0;
        std::int64_t candidate_material_count = 0;
        std::int64_t reference_texture_count = 0;
        std::int64_t candidate_texture_count = 0;
    };

    [[nodiscard]] std::vector<TraceSummary> load_trace_summaries(sql::Database &db);
    [[nodiscard]] std::optional<std::int64_t> first_trace_id_for_emulator(sql::Database &db, std::string_view emulator);
    [[nodiscard]] std::vector<PacketSignatureDiff> load_packet_signature_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                                                std::int64_t candidate_trace_id, std::int64_t limit);
    [[nodiscard]] std::vector<CopyKindDiff> load_copy_kind_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                                 std::int64_t candidate_trace_id);
    [[nodiscard]] std::vector<SemanticAnchorAlignment> load_semantic_anchor_alignments(sql::Database &db, std::int64_t reference_trace_id,
                                                                                       std::int64_t candidate_trace_id,
                                                                                       std::int64_t limit);
    [[nodiscard]] std::vector<LayoutRuntimeDiff> load_layout_runtime_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                                           std::int64_t candidate_trace_id, std::int64_t limit);

}  // namespace smgpc::trace
