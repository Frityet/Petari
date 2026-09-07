#include "DebugPaths.hpp"
#include "Sqlite.hpp"
#include "TraceAnalysis.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct Options {
        std::filesystem::path database;
        std::string query = "summary";
        std::int64_t limit = 40;
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
        out << "usage: smg-pc-trace-inspect-sqlite [--database traces.sqlite] [--query summary|semantic|layouts|materials|textures|packets|copies|views] [--limit n]\n";
        out << "Prints focused SQLite trace analysis tables for parity investigation.\n";
    }

    [[nodiscard]] Options parse_args(int argc, char **argv) {
        auto options = Options {};
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
            if (arg == "--query") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--query requires a name");
                }
                options.query = argv[++i];
                continue;
            }
            if (arg == "--limit") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--limit requires a count");
                }
                options.limit = parse_i64(argv[++i], "--limit");
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

    void print_summary(smgpc::sql::Database &db) {
        const auto summaries = smgpc::trace::load_trace_summaries(db);
        std::cout << "trace_id\temulator\trequested_frame\tframe_index\tframebuffer\trecords\trender_packets\tcopy_events\tsemantic_events\tlayout_runtime\tpath\n";
        for (const auto &summary : summaries) {
            std::cout << summary.trace_id << '\t' << optional_text(summary.emulator) << '\t'
                      << optional_int(summary.requested_frame) << '\t' << optional_int(summary.frame_index) << '\t'
                      << optional_int(summary.framebuffer_width) << 'x' << optional_int(summary.framebuffer_height) << '\t'
                      << summary.record_count << '\t' << summary.render_packet_count << '\t'
                      << summary.copy_event_count << '\t' << summary.semantic_event_count << '\t'
                      << summary.layout_runtime_count << '\t' << summary.path << '\n';
        }
    }

    void print_query(smgpc::sql::Database &db, std::string_view heading, std::string_view sql, int columns, std::int64_t limit) {
        auto select = smgpc::sql::Statement(db, sql);
        select.bind(1, limit);
        std::cout << heading << '\n';
        while (select.step()) {
            for (auto column = 0; column < columns; ++column) {
                const auto text = select.column_text(column);
                const auto integer = select.column_int(column);
                if (column != 0) {
                    std::cout << '\t';
                }
                if (text.has_value()) {
                    std::cout << *text;
                } else if (integer.has_value()) {
                    std::cout << *integer;
                } else {
                    std::cout << "<null>";
                }
            }
            std::cout << '\n';
        }
    }

    void print_views() {
        std::cout << "view\tpurpose\n";
        std::cout << "trace_overview\tone-row summary per trace\n";
        std::cout << "packet_signatures\trender packet state signatures with texture binding summaries\n";
        std::cout << "material_usage\tpacket counts by material\n";
        std::cout << "layout_usage\tpacket counts by layout\n";
        std::cout << "texture_usage\ttexture binding counts by name, format, and size\n";
        std::cout << "copy_kind_counts\tcopy event counts by kind\n";
        std::cout << "semantic_anchor_counts\tsemantic event counts and first frame by anchor\n";
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto options = parse_args(argc, argv);
    auto db = smgpc::sql::Database(options.database);

    if (options.query == "summary") {
        print_summary(db);
    } else if (options.query == "semantic") {
        print_query(db, "trace_id\tcategory\tname\tfirst_frame_index\tevent_count", R"SQL(
            SELECT trace_id, category, name, first_frame_index, event_count
            FROM semantic_anchor_counts
            ORDER BY trace_id, category, name
            LIMIT ?
        )SQL", 5, options.limit);
    } else if (options.query == "layouts") {
        print_query(db, "trace_id\tlayout_name\tpacket_count", R"SQL(
            SELECT trace_id, layout_name, packet_count
            FROM layout_usage
            ORDER BY trace_id, packet_count DESC, layout_name
            LIMIT ?
        )SQL", 3, options.limit);
    } else if (options.query == "materials") {
        print_query(db, "trace_id\tmaterial_name\tpacket_count", R"SQL(
            SELECT trace_id, material_name, packet_count
            FROM material_usage
            ORDER BY trace_id, packet_count DESC, material_name
            LIMIT ?
        )SQL", 3, options.limit);
    } else if (options.query == "textures") {
        print_query(db, "trace_id\ttexture_name\tformat\twidth\theight\tbinding_count", R"SQL(
            SELECT trace_id, texture_name, format, width, height, binding_count
            FROM texture_usage
            ORDER BY trace_id, binding_count DESC, texture_name
            LIMIT ?
        )SQL", 6, options.limit);
    } else if (options.query == "packets") {
        print_query(db, "trace_id\tmaterial_name\tlayout_name\trender_pass\ttexgen_count\ttev_stage_count\ttexture_signature\tpacket_count", R"SQL(
            SELECT trace_id, coalesce(material_name, '<null>'), coalesce(layout_name, '<null>'), coalesce(render_pass, '<null>'),
                   coalesce(texgen_count, -1), coalesce(tev_stage_count, -1), coalesce(texture_signature, '<none>'), count(*)
            FROM packet_signatures
            GROUP BY trace_id, material_name, layout_name, render_pass, texgen_count, tev_stage_count, texture_signature
            ORDER BY trace_id, count(*) DESC, material_name
            LIMIT ?
        )SQL", 8, options.limit);
    } else if (options.query == "copies") {
        print_query(db, "trace_id\tkind\tcopy_count", R"SQL(
            SELECT trace_id, kind, copy_count
            FROM copy_kind_counts
            ORDER BY trace_id, kind
            LIMIT ?
        )SQL", 3, options.limit);
    } else if (options.query == "views") {
        print_views();
    } else {
        throw std::runtime_error("unknown query: " + options.query);
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "trace SQLite inspect failed: " << e.what() << '\n';
    return 1;
}
