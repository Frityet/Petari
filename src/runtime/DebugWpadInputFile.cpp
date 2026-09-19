#include "runtime/DebugWpadInputFile.hpp"

#ifndef NDEBUG
#include <aurora/allocation.hpp>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace smgpc::runtime {
    DebugWpadInputFile::DebugWpadInputFile(std::string path) : _path(std::move(path)) {}

    DebugWpadInputFile DebugWpadInputFile::from_environment() {
        const aurora::allocation::HostAllocationScope host;
        const auto* path = std::getenv("SMGPC_DEBUG_WPAD_INPUT_FILE");
        return DebugWpadInputFile(path ? path : "");
    }

    DebugWpadInputScript::Applied DebugWpadInputFile::apply(std::uint64_t frame, std::uint32_t& hold,
        render::core::InputPointerState& pointer, float& stick_x, float& stick_y) {
        if (_path.empty()) return {};
        const aurora::allocation::HostAllocationScope host;
        std::ifstream input(_path, std::ios::binary | std::ios::ate);
        if (!input) throw std::runtime_error("Cannot open debug controller input file: " + _path);
        const auto size = input.tellg();
        if (size <= 0 || size > 65536)
            throw std::runtime_error("Debug controller input file must contain 1..65536 bytes");
        std::string contents(static_cast<std::size_t>(size), '\0');
        input.seekg(0);
        if (!input.read(contents.data(), static_cast<std::streamsize>(contents.size())))
            throw std::runtime_error("Cannot read complete debug controller input file; replace it atomically");
        if (!_revision || contents != _contents) {
            const auto config = nlohmann::json::parse(contents);
            if (!config.is_object() || config.size() != 3 || !config.contains("buttons") ||
                !config.contains("pointer") || !config.contains("stick") ||
                !config["buttons"].is_string() || !config["pointer"].is_string() || !config["stick"].is_string())
                throw std::invalid_argument("Debug controller input requires exactly three string fields: buttons, pointer, stick");
            // Parse every channel before publishing anything. Invalid updates
            // fail explicitly and leave the previous revision untouched.
            DebugWpadInputScript candidate(config["buttons"].get_ref<const std::string&>(),
                config["pointer"].get_ref<const std::string&>(), config["stick"].get_ref<const std::string&>());
            _script = std::move(candidate);
            _contents = std::move(contents);
            ++_revision;
            const auto canonical = config.dump();
            std::fprintf(stderr, "[debug-input-file] frame=%llu revision=%llu scripts=%s\n",
                static_cast<unsigned long long>(frame), static_cast<unsigned long long>(_revision), canonical.c_str());
            std::fflush(stderr);
        }
        return _script.apply(frame, hold, pointer, stick_x, stick_y);
    }
}
#endif
