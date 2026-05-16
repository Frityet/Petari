#include "Game/Util/StarPointerUtil.hpp"

#include "Game/Screen/LayoutActor.hpp"
#include "compat/RuntimeContext.hpp"
#include "layout/LayoutArchiveLoader.hpp"

namespace {

[[nodiscard]] bool is_inside(float value, float min_value, float max_value) {
    return value >= min_value && value <= max_value;
}

}  // namespace

namespace MR {

bool isStarPointerPointingPane(const LayoutActor *pActor, const char *pPaneName, int port, bool check, const char *pStrength) {
    (void)port;
    (void)check;
    (void)pStrength;

    if (pActor == nullptr || pPaneName == nullptr) {
        return false;
    }

    f32 x0 {};
    f32 y0 {};
    f32 x1 {};
    f32 y1 {};
    if (!pActor->getPaneBounds(pPaneName, &x0, &y0, &x1, &y1)) {
        return false;
    }

    const auto &context = smgpc::game::compat::runtime_context();
    if (!context.input_service || !context.renderer_engine) {
        return false;
    }

    const auto cursor = context.input_service->snapshot().cursor_position();
    if (!cursor.has_value()) {
        return false;
    }

    const auto framebuffer = context.renderer_engine->framebuffer_size();
    if (framebuffer.width == 0U || framebuffer.height == 0U) {
        return false;
    }

    float layout_width = static_cast<float>(framebuffer.width);
    float layout_height = static_cast<float>(framebuffer.height);
    if (const auto *resource = pActor->getResource(); resource != nullptr && resource->layout.size.x > 0.0F && resource->layout.size.y > 0.0F) {
        layout_width = resource->layout.size.x;
        layout_height = resource->layout.size.y;
    }

    const auto layout_x = static_cast<float>(cursor->x) * layout_width / static_cast<float>(framebuffer.width);
    const auto layout_y = static_cast<float>(cursor->y) * layout_height / static_cast<float>(framebuffer.height);
    return is_inside(layout_x, x0, x1) && is_inside(layout_y, y0, y1);
}

}  // namespace MR
