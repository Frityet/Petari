#include "TraceAnalysis.hpp"

#include <algorithm>

namespace smgpc::trace {

    std::vector<TraceSummary> load_trace_summaries(sql::Database &db) {
        auto select = sql::Statement(db, R"SQL(
            SELECT
                t.id,
                t.path,
                t.emulator,
                t.requested_frame,
                f.frame_index,
                f.framebuffer_width,
                f.framebuffer_height,
                t.record_count,
                (SELECT count(*) FROM render_packets rp WHERE rp.trace_id = t.id) AS render_packet_count,
                (SELECT count(*) FROM copy_events ce WHERE ce.trace_id = t.id) AS copy_event_count
            FROM traces t
            LEFT JOIN frames f ON f.trace_id = t.id
            ORDER BY t.id
        )SQL");

        auto out = std::vector<TraceSummary>{};
        while (select.step()) {
            out.push_back(TraceSummary{
                .trace_id = select.column_int(0).value_or(0),
                .path = std::filesystem::path(select.column_text(1).value_or(std::string{})),
                .emulator = select.column_text(2),
                .requested_frame = select.column_int(3),
                .frame_index = select.column_int(4),
                .framebuffer_width = select.column_int(5),
                .framebuffer_height = select.column_int(6),
                .record_count = select.column_int(7).value_or(0),
                .render_packet_count = select.column_int(8).value_or(0),
                .copy_event_count = select.column_int(9).value_or(0),
            });
        }
        return out;
    }

    std::optional<std::int64_t> first_trace_id_for_emulator(sql::Database &db, std::string_view emulator) {
        auto select = sql::Statement(db, "SELECT id FROM traces WHERE emulator = ? ORDER BY id LIMIT 1");
        select.bind(1, emulator);
        if (!select.step()) {
            return std::nullopt;
        }
        return select.column_int(0);
    }

    std::vector<PacketSignatureDiff> load_packet_signature_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                                 std::int64_t candidate_trace_id, std::int64_t limit) {
        auto select = sql::Statement(db, R"SQL(
            WITH packet_counts AS (
                SELECT
                    trace_id,
                    'texgens=' || coalesce(texgen_count, 'null') ||
                    ' colors=' || coalesce(color_channel_count, 'null') ||
                    ' tev=' || coalesce(tev_stage_count, 'null') ||
                    ' indirect=' || coalesce(indirect_stage_count, 'null') ||
                    ' cull=' || coalesce(cull_mode, 'null') ||
                    ' vertices=' || coalesce(vertex_count, 'null') ||
                    ' lights=' || coalesce(requested_light_mask, 'null') ||
                    ' textures=' || coalesce(texture_signature, '') AS signature,
                    count(*) AS packet_count
                FROM packet_signatures
                WHERE trace_id = ? OR trace_id = ?
                GROUP BY trace_id, signature
            ),
            ref AS (
                SELECT signature, packet_count FROM packet_counts WHERE trace_id = ?
            ),
            cand AS (
                SELECT signature, packet_count FROM packet_counts WHERE trace_id = ?
            ),
            keys AS (
                SELECT signature FROM ref
                UNION
                SELECT signature FROM cand
            )
            SELECT
                keys.signature,
                coalesce(ref.packet_count, 0) AS reference_count,
                coalesce(cand.packet_count, 0) AS candidate_count
            FROM keys
            LEFT JOIN ref ON ref.signature = keys.signature
            LEFT JOIN cand ON cand.signature = keys.signature
            WHERE coalesce(ref.packet_count, 0) != coalesce(cand.packet_count, 0)
            ORDER BY abs(coalesce(ref.packet_count, 0) - coalesce(cand.packet_count, 0)) DESC, keys.signature
            LIMIT ?
        )SQL");
        select.bind(1, reference_trace_id);
        select.bind(2, candidate_trace_id);
        select.bind(3, reference_trace_id);
        select.bind(4, candidate_trace_id);
        select.bind(5, limit);

        auto out = std::vector<PacketSignatureDiff>{};
        while (select.step()) {
            out.push_back(PacketSignatureDiff{
                .signature = select.column_text(0).value_or(std::string{}),
                .reference_count = select.column_int(1).value_or(0),
                .candidate_count = select.column_int(2).value_or(0),
            });
        }
        return out;
    }

    std::vector<CopyKindDiff> load_copy_kind_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                   std::int64_t candidate_trace_id) {
        auto select = sql::Statement(db, R"SQL(
            WITH copy_counts AS (
                SELECT trace_id, coalesce(kind, 'null') AS kind, count(*) AS copy_count
                FROM copy_events
                WHERE trace_id = ? OR trace_id = ?
                GROUP BY trace_id, kind
            ),
            ref AS (
                SELECT kind, copy_count FROM copy_counts WHERE trace_id = ?
            ),
            cand AS (
                SELECT kind, copy_count FROM copy_counts WHERE trace_id = ?
            ),
            keys AS (
                SELECT kind FROM ref
                UNION
                SELECT kind FROM cand
            )
            SELECT
                keys.kind,
                coalesce(ref.copy_count, 0) AS reference_count,
                coalesce(cand.copy_count, 0) AS candidate_count
            FROM keys
            LEFT JOIN ref ON ref.kind = keys.kind
            LEFT JOIN cand ON cand.kind = keys.kind
            WHERE coalesce(ref.copy_count, 0) != coalesce(cand.copy_count, 0)
            ORDER BY keys.kind
        )SQL");
        select.bind(1, reference_trace_id);
        select.bind(2, candidate_trace_id);
        select.bind(3, reference_trace_id);
        select.bind(4, candidate_trace_id);

        auto out = std::vector<CopyKindDiff>{};
        while (select.step()) {
            out.push_back(CopyKindDiff{
                .kind = select.column_text(0).value_or(std::string{}),
                .reference_count = select.column_int(1).value_or(0),
                .candidate_count = select.column_int(2).value_or(0),
            });
        }
        return out;
    }

}  // namespace smgpc::trace
