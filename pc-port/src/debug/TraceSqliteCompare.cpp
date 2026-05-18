#include "DebugPaths.hpp"
#include "Sqlite.hpp"
#include "TraceAnalysis.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace {

    struct Options {
        std::filesystem::path database;
        std::optional<std::int64_t> reference_trace_id;
        std::optional<std::int64_t> candidate_trace_id;
        std::int64_t max_signature_diffs = 12;
        std::int64_t max_semantic_anchors = 40;
        std::int64_t max_layout_runtime_diffs = 40;
    };

    [[nodiscard]] std::int64_t parse_i64(std::string_view text, std::string_view name) {
        try {
            std::size_t parsed = 0;
            const auto value = std::stoll(std::string(text), &parsed, 10);
            if (parsed != text.size()) {
                throw std::runtime_error("");
            }
            return value;
        } catch (const std::exception &) {
            throw std::runtime_error(std::string(name) + " requires an integer");
        }
    }

    void print_usage(std::ostream &out) {
        out << "usage: smg-pc-trace-compare-sqlite [--database traces.sqlite] [--reference-trace-id id] [--candidate-trace-id id]\n";
        out << "Reports semantic-anchor, frame, copy, and packet-signature deltas from the derived SQLite trace index.\n";
    }

    [[nodiscard]] Options parse_args(int argc, char **argv) {
        auto options = Options{};
        options.database = smgpc::debug::pc_port_root() / ".cache" / "render-parity" / "traces.sqlite";

        for (auto i = 1; i < argc; ++i) {
            const auto arg = std::string_view(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                std::exit(0);
            }
            if (arg == "--database") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--database requires a path");
                }
                options.database = argv[++i];
                continue;
            }
            if (arg == "--reference-trace-id") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--reference-trace-id requires an id");
                }
                options.reference_trace_id = parse_i64(argv[++i], "--reference-trace-id");
                continue;
            }
            if (arg == "--candidate-trace-id") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--candidate-trace-id requires an id");
                }
                options.candidate_trace_id = parse_i64(argv[++i], "--candidate-trace-id");
                continue;
            }
            if (arg == "--max-signature-diffs") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--max-signature-diffs requires a count");
                }
                options.max_signature_diffs = parse_i64(argv[++i], "--max-signature-diffs");
                continue;
            }
            if (arg == "--max-semantic-anchors") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--max-semantic-anchors requires a count");
                }
                options.max_semantic_anchors = parse_i64(argv[++i], "--max-semantic-anchors");
                continue;
            }
            if (arg == "--max-layout-runtime-diffs") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--max-layout-runtime-diffs requires a count");
                }
                options.max_layout_runtime_diffs = parse_i64(argv[++i], "--max-layout-runtime-diffs");
                continue;
            }
            throw std::runtime_error("unknown argument: " + std::string(arg));
        }

        return options;
    }

    [[nodiscard]] std::string optional_text(const std::optional<std::string> &value) {
        return value.value_or("<null>");
    }

    [[nodiscard]] std::string optional_int(const std::optional<std::int64_t> &value) {
        return value.has_value() ? std::to_string(*value) : std::string("<null>");
    }

}  // namespace

int main(int argc, char **argv) try {
    auto options = parse_args(argc, argv);
    auto db = smgpc::sql::Database(options.database);

    if (!options.reference_trace_id.has_value()) {
        options.reference_trace_id = smgpc::trace::first_trace_id_for_emulator(db, "dolphin");
    }
    if (!options.candidate_trace_id.has_value()) {
        options.candidate_trace_id = smgpc::trace::first_trace_id_for_emulator(db, "pc-port");
    }
    if (!options.reference_trace_id.has_value() || !options.candidate_trace_id.has_value()) {
        throw std::runtime_error("database must contain one dolphin trace and one pc-port trace, or explicit trace ids");
    }

    std::cout << "database: " << options.database << '\n';
    std::cout << "reference_trace_id: " << *options.reference_trace_id << '\n';
    std::cout << "candidate_trace_id: " << *options.candidate_trace_id << '\n';

    const auto summaries = smgpc::trace::load_trace_summaries(db);
    for (const auto &summary : summaries) {
        std::cout << "trace " << summary.trace_id
                  << " emulator=" << optional_text(summary.emulator)
                  << " requested_frame=" << optional_int(summary.requested_frame)
                  << " frame_index=" << optional_int(summary.frame_index)
                  << " framebuffer=" << optional_int(summary.framebuffer_width) << "x" << optional_int(summary.framebuffer_height)
                  << " records=" << summary.record_count
                  << " render_packets=" << summary.render_packet_count
                  << " copy_events=" << summary.copy_event_count
                  << " semantic_events=" << summary.semantic_event_count
                  << " layout_runtime=" << summary.layout_runtime_count
                  << " path=" << summary.path << '\n';
    }

    const auto semantic_anchors = smgpc::trace::load_semantic_anchor_alignments(db, *options.reference_trace_id,
                                                                                *options.candidate_trace_id,
                                                                                options.max_semantic_anchors);
    if (semantic_anchors.empty()) {
        std::cout << "semantic_anchor_alignment: none\n";
    } else {
        std::cout << "semantic_anchor_alignment:\n";
        for (const auto &anchor : semantic_anchors) {
            std::cout << "  " << anchor.category << ':' << anchor.name
                      << " reference_frame=" << optional_int(anchor.reference_frame_index)
                      << " candidate_frame=" << optional_int(anchor.candidate_frame_index)
                      << " delta=" << optional_int(anchor.frame_delta)
                      << " reference_count=" << anchor.reference_count
                      << " candidate_count=" << anchor.candidate_count
                      << " reference_stage=" << optional_text(anchor.reference_stage)
                      << " candidate_stage=" << optional_text(anchor.candidate_stage)
                      << '\n';
        }
    }

    const auto layout_diffs = smgpc::trace::load_layout_runtime_diffs(db, *options.reference_trace_id,
                                                                      *options.candidate_trace_id,
                                                                      options.max_layout_runtime_diffs);
    if (layout_diffs.empty()) {
        std::cout << "layout_runtime_diffs: none\n";
    } else {
        std::cout << "layout_runtime_diffs:\n";
        for (const auto &diff : layout_diffs) {
            std::cout << "  " << diff.name << " layout=" << diff.layout_name
                      << " reference_layouts=" << diff.reference_layout_count
                      << " candidate_layouts=" << diff.candidate_layout_count
                      << " reference_dead=" << diff.reference_dead_count
                      << " candidate_dead=" << diff.candidate_dead_count
                      << " reference_suspended=" << diff.reference_suspended_count
                      << " candidate_suspended=" << diff.candidate_suspended_count
                      << " reference_panes=" << diff.reference_pane_count
                      << " candidate_panes=" << diff.candidate_pane_count
                      << " reference_materials=" << diff.reference_material_count
                      << " candidate_materials=" << diff.candidate_material_count
                      << " reference_textures=" << diff.reference_texture_count
                      << " candidate_textures=" << diff.candidate_texture_count
                      << '\n';
        }
    }

    const auto copy_diffs = smgpc::trace::load_copy_kind_diffs(db, *options.reference_trace_id, *options.candidate_trace_id);
    if (copy_diffs.empty()) {
        std::cout << "copy_kind_diffs: none\n";
    } else {
        std::cout << "copy_kind_diffs:\n";
        for (const auto &diff : copy_diffs) {
            std::cout << "  " << diff.kind << " reference=" << diff.reference_count << " candidate=" << diff.candidate_count << '\n';
        }
    }

    const auto signature_diffs = smgpc::trace::load_packet_signature_diffs(db, *options.reference_trace_id,
                                                                           *options.candidate_trace_id,
                                                                           options.max_signature_diffs);
    if (signature_diffs.empty()) {
        std::cout << "packet_signature_diffs: none\n";
    } else {
        std::cout << "packet_signature_diffs:\n";
        for (const auto &diff : signature_diffs) {
            std::cout << "  reference=" << diff.reference_count << " candidate=" << diff.candidate_count << " " << diff.signature << '\n';
        }
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "trace SQLite compare failed: " << e.what() << '\n';
    return 1;
}
