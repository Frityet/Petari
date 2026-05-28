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
                (SELECT count(*) FROM copy_events ce WHERE ce.trace_id = t.id) AS copy_event_count,
                (SELECT count(*) FROM semantic_events se WHERE se.trace_id = t.id) AS semantic_event_count,
                (SELECT count(*) FROM layout_runtime lr WHERE lr.trace_id = t.id) AS layout_runtime_count
            FROM traces t
            LEFT JOIN frames f ON f.trace_id = t.id
            ORDER BY t.id
        )SQL");

        auto out = std::vector<TraceSummary>{};
        while (select.step()) {
            out.push_back(TraceSummary {
                .trace_id = select.column_int(0).value_or(0),
                .path = std::filesystem::path(select.column_text(1).value_or(std::string {})),
                .emulator = select.column_text(2),
                .requested_frame = select.column_int(3),
                .frame_index = select.column_int(4),
                .framebuffer_width = select.column_int(5),
                .framebuffer_height = select.column_int(6),
                .record_count = select.column_int(7).value_or(0),
                .render_packet_count = select.column_int(8).value_or(0),
                .copy_event_count = select.column_int(9).value_or(0),
                .semantic_event_count = select.column_int(10).value_or(0),
                .layout_runtime_count = select.column_int(11).value_or(0),
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
            out.push_back(PacketSignatureDiff {
                .signature = select.column_text(0).value_or(std::string {}),
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
            out.push_back(CopyKindDiff {
                .kind = select.column_text(0).value_or(std::string {}),
                .reference_count = select.column_int(1).value_or(0),
                .candidate_count = select.column_int(2).value_or(0),
            });
        }
        return out;
    }

    std::vector<SemanticAnchorAlignment> load_semantic_anchor_alignments(sql::Database &db, std::int64_t reference_trace_id,
                                                                         std::int64_t candidate_trace_id, std::int64_t limit) {
        auto select = sql::Statement(db, R"SQL(
            WITH events AS (
                SELECT
                    trace_id,
                    row_index,
                    coalesce(category, '<null>') AS category,
                    coalesce(name, '<null>') AS name,
                    frame_index,
                    stage,
                    detail
                FROM semantic_events
                WHERE trace_id = ? OR trace_id = ?
            ),
            anchor_counts AS (
                SELECT
                    trace_id,
                    category,
                    name,
                    min(row_index) AS first_row_index,
                    count(*) AS event_count
                FROM events
                GROUP BY trace_id, category, name
            ),
            anchor_firsts AS (
                SELECT
                    c.trace_id,
                    c.category,
                    c.name,
                    c.event_count,
                    e.frame_index,
                    e.stage,
                    e.detail
                FROM anchor_counts c
                JOIN events e ON e.trace_id = c.trace_id
                    AND e.category = c.category
                    AND e.name = c.name
                    AND e.row_index = c.first_row_index
            ),
            ref AS (
                SELECT category, name, event_count, frame_index, stage, detail
                FROM anchor_firsts
                WHERE trace_id = ?
            ),
            cand AS (
                SELECT category, name, event_count, frame_index, stage, detail
                FROM anchor_firsts
                WHERE trace_id = ?
            ),
            keys AS (
                SELECT category, name FROM ref
                UNION
                SELECT category, name FROM cand
            )
            SELECT
                keys.category,
                keys.name,
                ref.frame_index,
                cand.frame_index,
                CASE
                    WHEN ref.frame_index IS NOT NULL AND cand.frame_index IS NOT NULL
                    THEN cand.frame_index - ref.frame_index
                    ELSE NULL
                END AS frame_delta,
                coalesce(ref.event_count, 0) AS reference_count,
                coalesce(cand.event_count, 0) AS candidate_count,
                ref.stage,
                cand.stage,
                ref.detail,
                cand.detail
            FROM keys
            LEFT JOIN ref ON ref.category = keys.category AND ref.name = keys.name
            LEFT JOIN cand ON cand.category = keys.category AND cand.name = keys.name
            ORDER BY
                CASE WHEN ref.event_count IS NULL OR cand.event_count IS NULL THEN 0 ELSE 1 END,
                abs(coalesce(cand.frame_index, 0) - coalesce(ref.frame_index, 0)) DESC,
                keys.category,
                keys.name
            LIMIT ?
        )SQL");
        select.bind(1, reference_trace_id);
        select.bind(2, candidate_trace_id);
        select.bind(3, reference_trace_id);
        select.bind(4, candidate_trace_id);
        select.bind(5, limit);

        auto out = std::vector<SemanticAnchorAlignment>{};
        while (select.step()) {
            out.push_back(SemanticAnchorAlignment {
                .category = select.column_text(0).value_or(std::string {}),
                .name = select.column_text(1).value_or(std::string {}),
                .reference_frame_index = select.column_int(2),
                .candidate_frame_index = select.column_int(3),
                .frame_delta = select.column_int(4),
                .reference_count = select.column_int(5).value_or(0),
                .candidate_count = select.column_int(6).value_or(0),
                .reference_stage = select.column_text(7),
                .candidate_stage = select.column_text(8),
                .reference_detail = select.column_text(9),
                .candidate_detail = select.column_text(10),
            });
        }
        return out;
    }

    std::vector<LayoutRuntimeDiff> load_layout_runtime_diffs(sql::Database &db, std::int64_t reference_trace_id,
                                                             std::int64_t candidate_trace_id, std::int64_t limit) {
        auto select = sql::Statement(db, R"SQL(
            WITH layout_counts AS (
                SELECT
                    trace_id,
                    coalesce(name, '<null>') AS name,
                    coalesce(layout_name, '<null>') AS layout_name,
                    count(*) AS layout_count,
                    sum(coalesce(dead, 0)) AS dead_count,
                    sum(coalesce(suspended, 0)) AS suspended_count,
                    sum(coalesce(pane_count, 0)) AS pane_count,
                    sum(coalesce(material_count, 0)) AS material_count,
                    sum(coalesce(texture_count, 0)) AS texture_count
                FROM layout_runtime
                WHERE trace_id = ? OR trace_id = ?
                GROUP BY trace_id, name, layout_name
            ),
            ref AS (
                SELECT * FROM layout_counts WHERE trace_id = ?
            ),
            cand AS (
                SELECT * FROM layout_counts WHERE trace_id = ?
            ),
            keys AS (
                SELECT name, layout_name FROM ref
                UNION
                SELECT name, layout_name FROM cand
            )
            SELECT
                keys.name,
                keys.layout_name,
                coalesce(ref.layout_count, 0) AS reference_layout_count,
                coalesce(cand.layout_count, 0) AS candidate_layout_count,
                coalesce(ref.dead_count, 0) AS reference_dead_count,
                coalesce(cand.dead_count, 0) AS candidate_dead_count,
                coalesce(ref.suspended_count, 0) AS reference_suspended_count,
                coalesce(cand.suspended_count, 0) AS candidate_suspended_count,
                coalesce(ref.pane_count, 0) AS reference_pane_count,
                coalesce(cand.pane_count, 0) AS candidate_pane_count,
                coalesce(ref.material_count, 0) AS reference_material_count,
                coalesce(cand.material_count, 0) AS candidate_material_count,
                coalesce(ref.texture_count, 0) AS reference_texture_count,
                coalesce(cand.texture_count, 0) AS candidate_texture_count
            FROM keys
            LEFT JOIN ref ON ref.name = keys.name AND ref.layout_name = keys.layout_name
            LEFT JOIN cand ON cand.name = keys.name AND cand.layout_name = keys.layout_name
            WHERE
                coalesce(ref.layout_count, 0) != coalesce(cand.layout_count, 0) OR
                coalesce(ref.dead_count, 0) != coalesce(cand.dead_count, 0) OR
                coalesce(ref.suspended_count, 0) != coalesce(cand.suspended_count, 0) OR
                coalesce(ref.pane_count, 0) != coalesce(cand.pane_count, 0) OR
                coalesce(ref.material_count, 0) != coalesce(cand.material_count, 0) OR
                coalesce(ref.texture_count, 0) != coalesce(cand.texture_count, 0)
            ORDER BY
                abs(coalesce(ref.layout_count, 0) - coalesce(cand.layout_count, 0)) DESC,
                abs(coalesce(ref.pane_count, 0) - coalesce(cand.pane_count, 0)) DESC,
                keys.name,
                keys.layout_name
            LIMIT ?
        )SQL");
        select.bind(1, reference_trace_id);
        select.bind(2, candidate_trace_id);
        select.bind(3, reference_trace_id);
        select.bind(4, candidate_trace_id);
        select.bind(5, limit);

        auto out = std::vector<LayoutRuntimeDiff>{};
        while (select.step()) {
            out.push_back(LayoutRuntimeDiff {
                .name = select.column_text(0).value_or(std::string {}),
                .layout_name = select.column_text(1).value_or(std::string {}),
                .reference_layout_count = select.column_int(2).value_or(0),
                .candidate_layout_count = select.column_int(3).value_or(0),
                .reference_dead_count = select.column_int(4).value_or(0),
                .candidate_dead_count = select.column_int(5).value_or(0),
                .reference_suspended_count = select.column_int(6).value_or(0),
                .candidate_suspended_count = select.column_int(7).value_or(0),
                .reference_pane_count = select.column_int(8).value_or(0),
                .candidate_pane_count = select.column_int(9).value_or(0),
                .reference_material_count = select.column_int(10).value_or(0),
                .candidate_material_count = select.column_int(11).value_or(0),
                .reference_texture_count = select.column_int(12).value_or(0),
                .candidate_texture_count = select.column_int(13).value_or(0),
            });
        }
        return out;
    }

}  // namespace smgpc::trace
