#pragma once

#include <memory>
#include <JSystem/JGeometry/TMatrix.hpp>

class StarPointerController;
class StarPointerDirector;
class StarPointerOnOffController;
class StarPointerGuidance;
class StarPointerLayout;
class StarPointerTransformHolder;

namespace smgpc::compat {
class JkrHeapRuntime;
class StarPointerSceneBinding;

// GameSystem's original pointer records and native GPU callback lifetime.
class StarPointerDepthOwnership final {
public:
    explicit StarPointerDepthOwnership(std::shared_ptr<JkrHeapRuntime> heaps);
    ~StarPointerDepthOwnership();
    StarPointerDepthOwnership(const StarPointerDepthOwnership&) = delete;
    StarPointerDepthOwnership& operator=(const StarPointerDepthOwnership&) = delete;

    void set_camera(const TPos3f& view, const TProj3f& projection, f32 fovy);
    void update();
    void capture();
    void initialize_layouts();
    void draw();
    void discard_depth_samples();
    [[nodiscard]] StarPointerDirector& director();
    [[nodiscard]] StarPointerOnOffController& modes();
    [[nodiscard]] StarPointerController& controller(s32 channel);
    [[nodiscard]] TVec3f& world_position(s32 channel);
    [[nodiscard]] StarPointerTransformHolder& transform();
    [[nodiscard]] StarPointerGuidance* guidance() const noexcept;

private:
    friend class StarPointerSceneBinding;
    struct State;
    std::unique_ptr<State> _state;
    StarPointerDepthOwnership* _previous = nullptr;
};

class StarPointerSceneBinding final {
public:
    StarPointerSceneBinding();
    ~StarPointerSceneBinding();
    StarPointerSceneBinding(const StarPointerSceneBinding&) = delete;
    StarPointerSceneBinding& operator=(const StarPointerSceneBinding&) = delete;
private:
    StarPointerDepthOwnership* _owner = nullptr;
    StarPointerSceneBinding* _previous = nullptr;
    bool _previous_transform_update = false;
};

[[nodiscard]] StarPointerDepthOwnership* try_star_pointer_depth() noexcept;
[[nodiscard]] StarPointerDepthOwnership& require_star_pointer_depth();
} // namespace smgpc::compat
