#pragma once

#include <array>
#include <cstdint>

namespace smgpc::render::compat::gx {

struct MatrixStackFrame {
    std::array<float, 16> matrix {};
    bool identity {true};
};

class GXCompatState {
public:
    void reset() {
        _viewport = {0U, 0U, 0U, 0U};
        _scissor = {0U, 0U, 0U, 0U};
        _texture_id = 0U;
        _active_texture_slot = 0U;
        _matrix_stack[0] = MatrixStackFrame {};
        _matrix_stack[1] = MatrixStackFrame {};
        _matrix_stack[2] = MatrixStackFrame {};
        _scissor_enabled = false;
    }

    void set_viewport(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) {
        _viewport = {x, y, width, height};
    }

    void set_scissor(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) {
        _scissor = {x, y, width, height};
        _scissor_enabled = true;
    }

    void disable_scissor() {
        _scissor_enabled = false;
    }

    void set_bound_texture_id(std::uint64_t texture_id, std::uint8_t slot) {
        _texture_id = texture_id;
        _active_texture_slot = slot;
    }

    [[nodiscard]] std::uint64_t texture_id() const {
        return _texture_id;
    }

    [[nodiscard]] std::uint8_t active_texture_slot() const {
        return _active_texture_slot;
    }

    [[nodiscard]] const std::array<std::uint16_t, 4> &viewport() const {
        return _viewport;
    }

    [[nodiscard]] const std::array<std::uint16_t, 4> &scissor() const {
        return _scissor;
    }

    [[nodiscard]] bool scissor_enabled() const {
        return _scissor_enabled;
    }

    MatrixStackFrame &transform_matrix(std::uint8_t slot) {
        return _matrix_stack[slot % _matrix_stack.size()];
    }

private:
    std::array<std::uint16_t, 4> _viewport {0U, 0U, 0U, 0U};
    std::array<std::uint16_t, 4> _scissor {0U, 0U, 0U, 0U};
    std::uint64_t _texture_id {};
    std::uint8_t _active_texture_slot {};
    std::array<MatrixStackFrame, 3> _matrix_stack {};
    bool _scissor_enabled {};
};

}  // namespace smgpc::render::compat::gx
