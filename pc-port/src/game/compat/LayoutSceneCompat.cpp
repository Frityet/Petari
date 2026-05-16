#include "compat/LayoutSceneCompat.hpp"

#include <algorithm>
#include <vector>

#include "Game/Screen/LayoutActor.hpp"

namespace smgpc::game::compat {
    namespace {

        struct LayoutSceneEntry {
            LayoutActor *actor{};
            LayoutSceneLayer layer{};
        };

        std::vector< LayoutSceneEntry > sLayoutSceneEntries{};

        [[nodiscard]] auto find_entry(const LayoutActor *pActor) {
            return std::find_if(sLayoutSceneEntries.begin(), sLayoutSceneEntries.end(), [pActor](const LayoutSceneEntry &entry) {
                return entry.actor == pActor;
            });
        }

    }  // namespace

    void connect_layout_scene_actor(LayoutActor *pActor, LayoutSceneLayer layer) {
        if (pActor == nullptr) {
            return;
        }

        const auto existing = find_entry(pActor);
        if (existing != sLayoutSceneEntries.end()) {
            existing->layer = layer;
            return;
        }

        sLayoutSceneEntries.push_back(LayoutSceneEntry{
            .actor = pActor,
            .layer = layer,
        });
    }

    void disconnect_layout_scene_actor(const LayoutActor *pActor) {
        if (pActor == nullptr) {
            return;
        }

        sLayoutSceneEntries.erase(std::remove_if(sLayoutSceneEntries.begin(), sLayoutSceneEntries.end(), [pActor](const LayoutSceneEntry &entry) {
            return entry.actor == pActor;
        }), sLayoutSceneEntries.end());
    }

    void movement_layout_scene_layer(LayoutSceneLayer layer, const LayoutActor *pExcludedActor) {
        const auto entries = sLayoutSceneEntries;
        for (const auto &entry : entries) {
            if (entry.actor == nullptr || entry.actor == pExcludedActor || entry.layer != layer) {
                continue;
            }
            entry.actor->movement();
        }
    }

    void append_layout_scene_layer_draw_commands(LayoutSceneLayer layer, smgpc::render::layout::LayoutDrawList *pDrawList, const LayoutActor *pExcludedActor) {
        if (pDrawList == nullptr) {
            return;
        }

        const auto entries = sLayoutSceneEntries;
        for (const auto &entry : entries) {
            if (entry.actor == nullptr || entry.actor == pExcludedActor || entry.layer != layer) {
                continue;
            }
            entry.actor->appendDrawCommands(pDrawList);
        }
    }

}  // namespace smgpc::game::compat
