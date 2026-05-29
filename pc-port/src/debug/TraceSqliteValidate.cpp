#include "Sqlite.hpp"
#include "TraceAnalysis.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct Options {
        std::vector<std::filesystem::path> databases;
        std::string require_emulator;
        std::optional<std::int64_t> require_frame;
        std::vector<std::string> require_record_types;
        std::vector<std::string> require_layouts;
        bool require_semantic_events = false;
        std::int64_t min_render_packets = 0;
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

    void append_csv(std::vector<std::string> &out, std::string_view text) {
        auto value = std::string(text);
        auto begin = std::size_t {};
        while (begin <= value.size()) {
            const auto comma = value.find(',', begin);
            auto part = value.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
            const auto first = part.find_first_not_of(" \t\r\n");
            const auto last = part.find_last_not_of(" \t\r\n");
            if (first != std::string::npos) {
                out.push_back(part.substr(first, last - first + 1U));
            }
            if (comma == std::string::npos) {
                break;
            }
            begin = comma + 1U;
        }
    }

    void print_usage(std::ostream &out) {
        out << "usage: smg-pc-trace-validate-sqlite [options] trace.sqlite...\n";
        out << "Validates SQLite trace stores and prints a compact TSV summary.\n";
    }

    [[nodiscard]] Options parse_args(int argc, char **argv) {
        auto options = Options {};
        for (auto i = 1; i < argc; ++i) {
            const auto arg = std::string_view(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                std::exit(0);
            }
            if (arg == "--require-emulator") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--require-emulator requires a value");
                }
                options.require_emulator = argv[++i];
                continue;
            }
            if (arg == "--require-frame") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--require-frame requires a value");
                }
                options.require_frame = parse_i64(argv[++i], "--require-frame");
                continue;
            }
            if (arg == "--require-record-type") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--require-record-type requires a value");
                }
                append_csv(options.require_record_types, argv[++i]);
                continue;
            }
            if (arg == "--require-layout") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--require-layout requires a value");
                }
                append_csv(options.require_layouts, argv[++i]);
                continue;
            }
            if (arg == "--require-semantic-events") {
                options.require_semantic_events = true;
                continue;
            }
            if (arg == "--min-render-packets") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--min-render-packets requires a value");
                }
                options.min_render_packets = parse_i64(argv[++i], "--min-render-packets");
                continue;
            }
            options.databases.emplace_back(arg);
        }
        if (options.databases.empty()) {
            throw std::runtime_error("expected at least one SQLite trace store");
        }
        return options;
    }

    [[nodiscard]] std::int64_t layout_count(smgpc::sql::Database &db, std::int64_t trace_id, std::string_view layout) {
        auto select = smgpc::sql::Statement(db, R"SQL(
            SELECT
                (SELECT count(*) FROM render_packets WHERE trace_id = ? AND layout_name = ?) +
                (SELECT count(*) FROM layout_runtime WHERE trace_id = ? AND (layout_name = ? OR name = ?))
        )SQL");
        select.bind(1, trace_id);
        select.bind(2, layout);
        select.bind(3, trace_id);
        select.bind(4, layout);
        select.bind(5, layout);
        if (!select.step()) {
            return 0;
        }
        return select.column_int(0).value_or(0);
    }

    [[nodiscard]] std::int64_t record_type_count(const smgpc::trace::TraceSummary &summary, std::string_view type) {
        if (type == "frame") {
            return summary.frame_index.has_value() ? 1 : 0;
        }
        if (type == "render_packet") {
            return summary.render_packet_count;
        }
        if (type == "copy_event") {
            return summary.copy_event_count;
        }
        if (type == "semantic_event") {
            return summary.semantic_event_count;
        }
        if (type == "layout_runtime") {
            return summary.layout_runtime_count;
        }
        throw std::runtime_error("unknown SQLite trace record type requirement: " + std::string(type));
    }

    [[nodiscard]] std::string optional_text(const std::optional<std::string> &value) {
        return value.value_or("<null>");
    }

    [[nodiscard]] std::string optional_int(const std::optional<std::int64_t> &value) {
        return value.has_value() ? std::to_string(*value) : std::string("<null>");
    }

    void validate_summary(smgpc::sql::Database &db, const smgpc::trace::TraceSummary &summary, const Options &options) {
        if (!options.require_emulator.empty() && optional_text(summary.emulator) != options.require_emulator) {
            throw std::runtime_error("trace " + std::to_string(summary.trace_id) + " emulator is " + optional_text(summary.emulator) +
                                     ", expected " + options.require_emulator);
        }
        if (options.require_frame.has_value() && summary.frame_index != options.require_frame) {
            throw std::runtime_error("trace " + std::to_string(summary.trace_id) + " frame is " + optional_int(summary.frame_index) +
                                     ", expected " + std::to_string(*options.require_frame));
        }
        if (summary.render_packet_count < options.min_render_packets) {
            throw std::runtime_error("trace " + std::to_string(summary.trace_id) + " render_packet count is below minimum");
        }
        if (options.require_semantic_events && summary.semantic_event_count == 0) {
            throw std::runtime_error("trace " + std::to_string(summary.trace_id) + " has no semantic events");
        }
        for (const auto &type : options.require_record_types) {
            if (record_type_count(summary, type) == 0) {
                throw std::runtime_error("trace " + std::to_string(summary.trace_id) + " is missing required record type " + type);
            }
        }
        for (const auto &layout : options.require_layouts) {
            if (layout_count(db, summary.trace_id, layout) == 0) {
                throw std::runtime_error("trace " + std::to_string(summary.trace_id) + " is missing required layout " + layout);
            }
        }
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto options = parse_args(argc, argv);
    std::cout << "trace\tstatus\ttrace_id\tframe_index\temulator\trecord_count\trender_packet\tsemantic_event\tlayout_runtime\n";

    for (const auto &database_path : options.databases) {
        auto db = smgpc::sql::Database(database_path);
        const auto summaries = smgpc::trace::load_trace_summaries(db);
        if (summaries.empty()) {
            throw std::runtime_error("SQLite trace store contains no traces: " + database_path.string());
        }
        for (const auto &summary : summaries) {
            validate_summary(db, summary, options);
            std::cout << database_path << "\tpassed\t" << summary.trace_id << '\t' << optional_int(summary.frame_index)
                      << '\t' << optional_text(summary.emulator) << '\t' << summary.record_count << '\t'
                      << summary.render_packet_count << '\t' << summary.semantic_event_count << '\t'
                      << summary.layout_runtime_count << '\n';
        }
    }
    return 0;
} catch (const std::exception &e) {
    std::cerr << "trace SQLite validation failed: " << e.what() << '\n';
    return 1;
}
