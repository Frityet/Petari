#pragma once

#include <filesystem>

#include "RendererService.hpp"

namespace smgpc::game {

    class RuntimeContext;

    void write_runtime_parity_trace(const std::filesystem::path &path, const render::FrameContext &frame_context, const RuntimeContext &runtime);

}  // namespace smgpc::game
