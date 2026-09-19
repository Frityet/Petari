#pragma once

#ifndef NDEBUG
#include "render/core/RenderTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace smgpc::runtime {
    struct DebugWpadButtonScriptSpan {
        std::uint64_t first_frame = 0;
        std::uint64_t last_frame = 0;
        std::uint32_t button_mask = 0;
    };

    struct DebugWpadPointerScriptSpan {
        std::uint64_t first_frame = 0;
        std::uint64_t last_frame = 0;
        float x = 0.0F;
        float y = 0.0F;
        bool valid = false;
    };

    struct DebugWpadStickScriptSpan {
        std::uint64_t first_frame = 0;
        std::uint64_t last_frame = 0;
        float x = 0.0F;
        float y = 0.0F;
    };

    // Debug controller input shared by both application owners. Frame ranges
    // are inclusive and zero-based. Buttons add to physical input; the last
    // active pointer/stick span overrides it. Stick axes must be finite and in
    // [-1,1]. No script means no input changes.
    class DebugWpadInputScript {
    public:
        struct Applied {
            bool buttons = false;
            bool pointer = false;
            bool stick = false;
        };

        explicit DebugWpadInputScript(std::string_view buttons = {}, std::string_view pointer = {},
                                     std::string_view stick = {});
        [[nodiscard]] static DebugWpadInputScript from_environment();
        [[nodiscard]] Applied apply(std::uint64_t frame, std::uint32_t& hold,
                                    render::core::InputPointerState& pointer, float& stick_x, float& stick_y) const;
        [[nodiscard]] std::size_t button_span_count() const { return _buttons.size(); }
        [[nodiscard]] std::size_t pointer_span_count() const { return _pointer.size(); }
        [[nodiscard]] std::size_t stick_span_count() const { return _stick.size(); }

    private:
        std::vector<DebugWpadButtonScriptSpan> _buttons;
        std::vector<DebugWpadPointerScriptSpan> _pointer;
        std::vector<DebugWpadStickScriptSpan> _stick;
    };
}
#endif
