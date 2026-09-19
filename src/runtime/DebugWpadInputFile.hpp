#pragma once

#ifndef NDEBUG
#include "runtime/DebugWpadInputScript.hpp"
#include <string>

namespace smgpc::runtime {
    // Optional live replay source. Writers atomically replace a JSON object
    // containing the three ordinary controller scripts. Each accepted revision
    // is logged with the exact original frame at which it becomes effective.
    class DebugWpadInputFile final {
    public:
        explicit DebugWpadInputFile(std::string path = {});
        [[nodiscard]] static DebugWpadInputFile from_environment();
        DebugWpadInputScript::Applied apply(std::uint64_t frame, std::uint32_t& hold,
            render::core::InputPointerState& pointer, float& stick_x, float& stick_y);
        [[nodiscard]] std::uint64_t revision() const { return _revision; }
    private:
        std::string _path;
        std::string _contents;
        DebugWpadInputScript _script;
        std::uint64_t _revision = 0;
    };
}
#endif
