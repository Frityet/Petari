#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "render/J3dAnimation.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dModelRenderer.hpp"

namespace smgpc::compat {

    class RarcArchive;
    struct RarcEntry;

}  // namespace smgpc::compat

class LiveActorModelCompat final {
public:
    enum class DrawPass {
        All,
        Opaque,
        Translucent,
    };

    LiveActorModelCompat(std::string model_arc_name, std::string animation_arc_name);

    void startBck(std::string_view name, std::string_view file_name);
    std::optional<std::int16_t> startBrk(std::string_view name);
    void startBtk(std::string_view name);
    void setProjmapEffectMatrix(const smgpc::compat::J3dMatrix3x4 &matrix);
    void draw(smgpc::render::IRendererEngine &renderer, const smgpc::compat::CameraPoseCompat &camera_pose,
              const smgpc::compat::J3dMatrix3x4 &actor_matrix, std::uint64_t frame, DrawPass pass = DrawPass::All);

    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] std::string_view model_arc_name() const;

private:
    void ensureLoaded(smgpc::render::IRendererEngine &renderer);
    void applyStartedAnimations();
    [[nodiscard]] const smgpc::compat::RarcEntry *findModelEntry(const smgpc::compat::RarcArchive &archive) const;
    [[nodiscard]] std::optional<smgpc::compat::J3dBckAnimationSummary> findBckAnimation(const smgpc::compat::RarcArchive &archive) const;
    [[nodiscard]] std::optional<smgpc::compat::J3dBtkAnimationSummary> findBtkAnimation(const smgpc::compat::RarcArchive &archive) const;
    [[nodiscard]] std::optional<smgpc::compat::J3dBrkAnimationSummary> findBrkAnimation(const smgpc::compat::RarcArchive &archive) const;

    std::string mModelArcName = {};
    std::string mAnimationArcName = {};
    bool mLoadAttempted = false;
    bool mBckStarted = false;
    bool mBrkStarted = false;
    bool mBtkStarted = false;
    std::string mBrkName = {};
    std::string mBrkAnimationName = {};
    std::unique_ptr<smgpc::compat::J3dModelRenderer> mRenderer = {};
    std::optional<smgpc::compat::J3dBckAnimationSummary> mBckAnimation = {};
    std::optional<smgpc::compat::J3dBtkAnimationSummary> mBtkAnimation = {};
    std::optional<smgpc::compat::J3dBrkAnimationSummary> mBrkAnimation = {};
    std::optional<smgpc::compat::J3dMatrix3x4> mProjmapEffectMatrix = {};
};
