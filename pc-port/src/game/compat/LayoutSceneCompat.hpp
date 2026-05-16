#pragma once

#include "layout/LayoutDrawList.hpp"

class LayoutActor;

namespace smgpc::game::compat {

enum class LayoutSceneLayer {
    Layout,
    LayoutDecoration,
    TalkLayout,
    LayoutOnPause,
};

void connect_layout_scene_actor(LayoutActor *pActor, LayoutSceneLayer layer);
void disconnect_layout_scene_actor(const LayoutActor *pActor);
void movement_layout_scene_layer(LayoutSceneLayer layer, const LayoutActor *pExcludedActor = nullptr);
void append_layout_scene_layer_draw_commands(LayoutSceneLayer layer, smgpc::render::layout::LayoutDrawList *pDrawList, const LayoutActor *pExcludedActor = nullptr);

}  // namespace smgpc::game::compat
