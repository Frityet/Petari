#include "tests/TestHarness.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

std::string QuoteShell(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 2U);
    out.push_back('\'');
    for (const char ch : value) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('\'');
    return out;
}

bool IsExecutableFile(const std::filesystem::path& path) {
    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::status(path, ec);
    if (ec || !std::filesystem::is_regular_file(status)) {
        return false;
    }

    const std::filesystem::perms perms = status.permissions();
    return (perms & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec)) !=
           std::filesystem::perms::none;
}

std::filesystem::path FindAppBinary() {
    const std::filesystem::path cwd = std::filesystem::current_path();
    for (const auto& candidate : {cwd / "pc-port", cwd / "bin" / "pc-port", cwd / "build" / "linux" / "x86_64" / "release" / "pc-port",
                                  cwd.parent_path() / "pc-port"}) {
        if (IsExecutableFile(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("pc-port executable not found for CLI contract test");
}

$pc_port_test(CliInvalidGameRootFails) {
    const std::filesystem::path app = FindAppBinary();
    const std::filesystem::path output = std::filesystem::temp_directory_path() / "pc_port_cli_test_output.txt";

    const std::string command = QuoteShell(app.string()) + " --game-root /tmp/pc-port-does-not-exist --headless >" + QuoteShell(output.string()) +
                                " 2>&1";

    const int code = std::system(command.c_str());
    $pc_port_require(code != 0);

    std::ifstream file(output);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    $pc_port_require(content.find("--game-root does not exist") != std::string::npos);

    std::error_code ec;
    std::filesystem::remove(output, ec);
}

}  // namespace
