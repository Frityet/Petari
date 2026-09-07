#include "DebugPaths.hpp"
#include "Sqlite.hpp"
#include "TraceStore.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

    struct Options {
        std::filesystem::path output;
        bool append = false;
        std::vector<std::filesystem::path> traces;
    };

    void print_usage(std::ostream &out) {
        out << "usage: smg-pc-trace-pack-sqlite [--output traces.sqlite] [--append] [trace.sqlite ...]\n";
        out << "Copies one or more SQLite trace stores into a single analysis database.\n";
    }

    [[nodiscard]] Options parse_args(int argc, char **argv) {
        auto options = Options {};
        options.output = smgpc::debug::pc_port_root() / ".cache" / "render-parity" / "traces.sqlite";

        for (auto i = 1; i < argc; ++i) {
            const auto arg = std::string_view(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                std::exit(0);
            }
            if (arg == "--append") {
                options.append = true;
                continue;
            }
            if (arg == "--output") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--output requires a path");
                }
                options.output = argv[++i];
                continue;
            }
            options.traces.emplace_back(arg);
        }

        if (options.traces.empty()) {
            const auto cache = smgpc::debug::pc_port_root() / ".cache" / "render-parity";
            const std::filesystem::path defaults[]{
                cache / "dolphin-frame-1900.trace.sqlite",
                cache / "pcport-frame-1900.trace.sqlite",
            };
            for (const auto &path : defaults) {
                if (std::filesystem::is_regular_file(path)) {
                    options.traces.push_back(path);
                }
            }
        }

        if (options.traces.empty()) {
            throw std::runtime_error("no SQLite trace stores provided and no cached frame-1900 SQLite traces found");
        }

        for (const auto &path : options.traces) {
            if (path.extension() != ".sqlite") {
                throw std::runtime_error("trace stores must use .sqlite: " + path.string());
            }
        }

        return options;
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto options = parse_args(argc, argv);
    if (!options.append && std::filesystem::exists(options.output)) {
        std::filesystem::remove(options.output);
    }

    auto output = smgpc::sql::Database(options.output);
    output.exec("PRAGMA foreign_keys = ON");
    smgpc::trace::create_trace_sqlite_schema(output);
    output.exec("BEGIN IMMEDIATE TRANSACTION");

    for (const auto &trace_path : options.traces) {
        auto source = smgpc::sql::Database(trace_path);
        source.exec("PRAGMA foreign_keys = ON");
        const auto ids = smgpc::trace::trace_ids(source);
        if (ids.empty()) {
            throw std::runtime_error("trace store contains no traces: " + trace_path.string());
        }
        for (const auto trace_id : ids) {
            const auto result = smgpc::trace::copy_trace_between_databases(output, source, trace_id);
            std::cout << "packed trace_id=" << result.trace_id << " records=" << result.record_count
                      << " render_packets=" << result.render_packet_count << " copy_events=" << result.copy_event_count
                      << " semantic_events=" << result.semantic_event_count << " source=" << trace_path << '\n';
        }
    }

    output.exec("COMMIT");
    std::cout << options.output << '\n';
    return 0;
} catch (const std::exception &e) {
    std::cerr << "trace SQLite pack failed: " << e.what() << '\n';
    return 1;
}
