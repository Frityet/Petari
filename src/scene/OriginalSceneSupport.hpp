#pragma once

class Scene;

namespace smgpc::scene {
// Attach native execution/lifetime data to the Scene the original controller
// actually constructed. The controller remains the sole scene/heap owner.
void bind_original_scene_support(Scene&);
void prepare_original_scene_support_retirement(Scene&) noexcept;
void initialize_original_scene_effects(unsigned particles, unsigned emitters);
void begin_original_scene_frame();
}
