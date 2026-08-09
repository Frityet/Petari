#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "render/J3dAnimation.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dModelRenderer.hpp"

namespace smgpc::resource {
    class RarcArchive;
    struct RarcEntry;
}  // namespace smgpc::resource

namespace smgpc::render::live_actor {

class LiveActorModel final {
public:
    enum class DrawPass {
        All,
        Opaque,
        Translucent,
    };

    LiveActorModel(std::string model_arc_name, std::string animation_arc_name);

    std::optional<std::int16_t> startBck(std::string_view name, std::string_view file_name);
    void setBckFrameAndStop(float frame);
    std::optional<std::int16_t> startBrk(std::string_view name);
    void setBrkFrame(float frame);
    [[nodiscard]] bool startBtk(std::string_view name);
    std::optional<std::int16_t> startBtp(std::string_view name);
    std::optional<std::int16_t> startActionBtp(std::string_view action_name);
    [[nodiscard]] bool hasBck(std::string_view name, std::string_view file_name) const;
    [[nodiscard]] bool hasBrk(std::string_view name) const;
    [[nodiscard]] bool hasBtk(std::string_view name) const;
    [[nodiscard]] bool hasBtp(std::string_view name) const;
    void syncJointAnimationFrom(const LiveActorModel &source);
    void syncMaterialAnimationFrom(const LiveActorModel &source);
    void setProjmapEffectMatrix(const smgpc::render::J3dMatrix3x4 &matrix);
    void requireLoaded();
    [[nodiscard]] std::int16_t requireBck(std::string_view name, std::string_view file_name);
    void draw(const smgpc::camera::CameraPose &camera_pose, const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
              DrawPass pass = DrawPass::All);
    void drawModel3DFor2D(
        const smgpc::render::Model3DFor2DProjection &projection,
        const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
        DrawPass pass = DrawPass::All);

    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] std::size_t joint_count();
    [[nodiscard]] std::string_view model_arc_name() const;
    [[nodiscard]] std::optional<std::int16_t> bck_frame_max(std::string_view name) const;
    [[nodiscard]] std::optional<std::uint8_t> bck_attribute() const;
    [[nodiscard]] std::optional<std::uint8_t> brk_attribute() const;
    [[nodiscard]] float brk_frame() const;
    [[nodiscard]] float bck_frame(std::uint64_t runtime_frame) const;
    [[nodiscard]] std::optional<bool> is_bck_stopped(std::uint64_t runtime_frame) const;
    [[nodiscard]] std::optional<bool> check_pass_bck_frame(std::uint64_t runtime_frame, float frame) const;
    [[nodiscard]] float btp_frame(std::uint64_t runtime_frame) const;
    [[nodiscard]] std::optional<bool> is_btp_stopped(std::uint64_t runtime_frame) const;
    [[nodiscard]] std::optional<float> model_bounding_radius();
    [[nodiscard]] bool has_effect_texture_matrix();
    [[nodiscard]] bool has_indirect_texture();
    [[nodiscard]] const smgpc::render::J3dMatrix3x4 *joint_world_matrix(
        std::string_view name, const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t runtime_frame);

private:
    [[nodiscard]] const LiveActorModel &jointAnimationSource() const;
    [[nodiscard]] const LiveActorModel &materialAnimationSource() const;
    void drawImpl(
        const smgpc::camera::CameraPose *camera_pose,
        const smgpc::render::Model3DFor2DProjection *model_3d_for_2d,
        const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
        DrawPass pass);
    void ensureLoaded();
    void resolveBckAnimation();
    [[nodiscard]] std::optional<smgpc::render::J3dBckAnimationSummary>
    loadBckAnimation(std::string_view resource_name) const;
    [[nodiscard]] std::optional<smgpc::render::J3dBtpAnimationSummary>
    loadBtpAnimation(std::string_view resource_name) const;
    [[nodiscard]] std::optional<std::string> resolveActionBtpName(std::string_view action_name) const;
    void applyStartedAnimations();
    [[nodiscard]] const smgpc::resource::RarcEntry *findModelEntry(const smgpc::resource::RarcArchive &archive) const;
    [[nodiscard]] std::optional<smgpc::render::J3dBckAnimationSummary>
    findBckAnimation(const smgpc::resource::RarcArchive &archive, std::string_view resource_name) const;
    [[nodiscard]] std::optional<smgpc::render::J3dBtkAnimationSummary>
    findBtkAnimation(const smgpc::resource::RarcArchive &archive, std::string_view name) const;
    [[nodiscard]] std::optional<smgpc::render::J3dBrkAnimationSummary>
    findBrkAnimation(const smgpc::resource::RarcArchive &archive, std::string_view name) const;
    [[nodiscard]] std::optional<smgpc::render::J3dBtpAnimationSummary>
    findBtpAnimation(const smgpc::resource::RarcArchive &archive, std::string_view resource_name) const;

    std::string mModelArcName = {};
    std::string mAnimationArcName = {};
    bool mLoadAttempted = false;
    bool mBckStarted = false;
    bool mBrkStarted = false;
    bool mBtkStarted = false;
    bool mBtpStarted = false;
    std::uint64_t mBckStartFrame = 0U;
    std::uint64_t mBtpStartFrame = 0U;
    std::string mBckResourceName = {};
    std::string mBrkName = {};
    std::string mBrkAnimationName = {};
    std::string mBtkName = {};
    std::string mBtkAnimationName = {};
    std::string mBtpName = {};
    std::unique_ptr<smgpc::render::J3dModelRenderer> mRenderer = {};
    std::optional<smgpc::render::J3dBckAnimationSummary> mBckAnimation = {};
    std::optional<float> mBckManualFrame = {};
    std::optional<smgpc::render::J3dBtkAnimationSummary> mBtkAnimation = {};
    std::optional<smgpc::render::J3dBrkAnimationSummary> mBrkAnimation = {};
    float mBrkFrame = 0.0F;
    std::optional<smgpc::render::J3dBtpAnimationSummary> mBtpAnimation = {};
    const LiveActorModel *mJointAnimationSource = nullptr;
    const LiveActorModel *mMaterialAnimationSource = nullptr;
    std::uint64_t mJointAnimationVersion = 0U;
    std::uint64_t mMaterialAnimationVersion = 0U;
    std::uint64_t mAppliedJointAnimationVersion = ~std::uint64_t{};
    std::uint64_t mAppliedMaterialAnimationVersion = ~std::uint64_t{};
    std::optional<smgpc::render::J3dMatrix3x4> mProjmapEffectMatrix = {};
    std::unordered_map<std::string, smgpc::render::J3dMatrix3x4> mResolvedJointMatrices = {};
};

}  // namespace smgpc::render::live_actor
