#pragma once

#include <memory>
#include <JSystem/JGeometry/TMatrix.hpp>

class StarPointerController;
class StarPointerGuidance;
class StarPointerLayout;
class StarPointerTransformHolder;

namespace smgpc::compat {
class JkrHeapRuntime;
class StarPointerLayoutBinding;

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
    void initialize_guidance();
    void draw_guidance();
    [[nodiscard]] StarPointerController& controller(s32 channel);
    [[nodiscard]] TVec3f& world_position(s32 channel);
    [[nodiscard]] StarPointerTransformHolder& transform();
    [[nodiscard]] StarPointerGuidance* guidance() const noexcept;

private:
    friend class StarPointerLayoutBinding;
    StarPointerLayout* bind_layout(s32 channel, StarPointerLayout* layout);
    struct State;
    std::unique_ptr<State> _state;
    StarPointerDepthOwnership* _previous = nullptr;
};

// The actual cursor owner must outlive this binding. Raw WPad validity is not
// equivalent to StarPointerLayout::mIsPointerValid.
class StarPointerLayoutBinding final {
public:
    StarPointerLayoutBinding(StarPointerDepthOwnership& owner, s32 channel, StarPointerLayout& layout);
    ~StarPointerLayoutBinding();
    StarPointerLayoutBinding(const StarPointerLayoutBinding&) = delete;
    StarPointerLayoutBinding& operator=(const StarPointerLayoutBinding&) = delete;
private:
    StarPointerDepthOwnership& _owner;
    s32 _channel;
    StarPointerLayout* _previous;
};

[[nodiscard]] StarPointerDepthOwnership& require_star_pointer_depth();
} // namespace smgpc::compat
