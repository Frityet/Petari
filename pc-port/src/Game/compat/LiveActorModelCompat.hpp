#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "Game/compat/CameraPose.hpp"
#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/J3dMaterialRuntime.hpp"
#include "Game/compat/J3dModelRenderer.hpp"
#include "RendererService.hpp"

namespace smgpc::game {

    class RarcArchive;
    struct RarcEntry;

}  // namespace smgpc::game

class LiveActorModelCompat final {
public:
    enum class DrawPass {
        All,
        Opaque,
        Translucent,
    };

    LiveActorModelCompat(std::string model_arc_name, std::string animation_arc_name);

    void startBck(std::string_view name, std::string_view file_name);
    void startBtk(std::string_view name);
    void draw(smgpc::render::IRendererEngine &renderer, const smgpc::game::CameraPoseCompat &camera_pose,
              const smgpc::game::J3dMatrix3x4 &actor_matrix, std::uint64_t frame, DrawPass pass = DrawPass::All);

    [[nodiscard]] bool isLoaded() const;

private:
    void ensureLoaded(smgpc::render::IRendererEngine &renderer);
    void applyStartedAnimations();
    [[nodiscard]] const smgpc::game::RarcEntry *findModelEntry(const smgpc::game::RarcArchive &archive) const;
    [[nodiscard]] std::optional<smgpc::game::J3dBckAnimationSummary> findBckAnimation(const smgpc::game::RarcArchive &archive) const;
    [[nodiscard]] std::optional<smgpc::game::J3dBtkAnimationSummary> findBtkAnimation(const smgpc::game::RarcArchive &archive) const;

    std::string mModelArcName{};
    std::string mAnimationArcName{};
    bool mLoadAttempted = false;
    bool mBckStarted = false;
    bool mBtkStarted = false;
    std::unique_ptr<smgpc::game::J3dModelRenderer> mRenderer{};
    std::optional<smgpc::game::J3dBckAnimationSummary> mBckAnimation{};
    std::optional<smgpc::game::J3dBtkAnimationSummary> mBtkAnimation{};
};
