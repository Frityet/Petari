#pragma once

#include <filesystem>

#include "DumpJson.hpp"
#include "RendererService.hpp"

namespace smgpc::game {

    class RuntimeContext;

    [[nodiscard]] dump::Json runtime_parity_trace_json(const render::FrameContext &frame_context, const RuntimeContext &runtime);
    void write_runtime_parity_trace(const std::filesystem::path &path, const render::FrameContext &frame_context, const RuntimeContext &runtime);
    [[nodiscard]] dump::Json load_runtime_parity_trace(const std::filesystem::path &path);

}  // namespace smgpc::game
