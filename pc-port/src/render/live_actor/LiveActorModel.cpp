#include "render/live_actor/LiveActorModel.hpp"

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <exception>

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "render/J3dMatrix.hpp"
#include "runtime/RuntimeContext.hpp"

namespace smgpc::render::live_actor {

namespace {
    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] const char *draw_pass_name(LiveActorModel::DrawPass pass) {
        switch (pass) {
        case LiveActorModel::DrawPass::All:
            return "All";
        case LiveActorModel::DrawPass::Opaque:
            return "Opaque";
        case LiveActorModel::DrawPass::Translucent:
            return "Translucent";
        }

        return "Unknown";
    }

    [[nodiscard]] bool packet_matches_draw_pass(LiveActorModel::DrawPass pass, const smgpc::render::J3dRendererPacketState &packet) {
        switch (pass) {
        case LiveActorModel::DrawPass::All:
            return true;
        case LiveActorModel::DrawPass::Opaque:
            return packet.draw_buffer_opaque;
        case LiveActorModel::DrawPass::Translucent:
            return !packet.draw_buffer_opaque;
        }

        return false;
    }

#ifndef NDEBUG
    [[nodiscard]] std::string_view debug_environment(std::string_view name) {
        const auto key = std::string(name);
        const auto *value = std::getenv(key.c_str());
        return value == nullptr ? std::string_view {} : std::string_view(value);
    }

    [[nodiscard]] bool debug_model_filter_matches(std::string_view model_name) {
        const auto filter = debug_environment("SMGPC_J3D_MODEL_FILTER");
        return filter.empty() || filter == model_name;
    }
#endif
}  // namespace

LiveActorModel::LiveActorModel(std::string model_arc_name, std::string animation_arc_name)
    : mModelArcName(std::move(model_arc_name)), mAnimationArcName(std::move(animation_arc_name)) {
}

std::optional<std::int16_t> LiveActorModel::startBck(std::string_view name, std::string_view file_name) {
    mJointAnimationSource = nullptr;
    mBckStarted = true;
    mBckResourceName = std::string(file_name.empty() ? name : file_name);
    mBckAnimation = loadBckAnimation(mBckResourceName);
    mBckFrame = 0.0F;
    mBckRate = mBckAnimation.has_value() && mBckAnimation->frame_max > 0 ? 1.0F : 0.0F;
    mBckState = 0U;
    ++mJointAnimationVersion;
    applyStartedAnimations();
    return mBckAnimation.has_value() ? std::optional<std::int16_t>{mBckAnimation->frame_max} : std::nullopt;
}

void LiveActorModel::syncBckFrameController(float frame, float rate, std::uint8_t state) {
    if (!mBckStarted || !mBckAnimation.has_value()) {
        throw std::logic_error("BCK animation data is unavailable.");
    }
    mBckFrame = frame;
    mBckRate = rate;
    mBckState = state;
}

void LiveActorModel::setBckFrameAndStop(float frame) {
    if (!mBckStarted || !mBckAnimation.has_value()) {
        throw std::logic_error("BCK animation data is unavailable.");
    }
    mBckFrame = frame;
    mBckRate = 0.0F;
}

std::optional<std::int16_t> LiveActorModel::startBrk(std::string_view name) {
    mMaterialAnimationSource = nullptr;
    mBrkStarted = true;
    const auto requested_name = std::string(name);
    if (mBrkName != requested_name) {
        mBrkAnimation.reset();
    }
    mBrkName = requested_name;
    mBrkFrame = 0.0F;
    if (!mBrkAnimation.has_value()) {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto archive_path = runtime != nullptr ? runtime->find_object_archive(mModelArcName) : std::nullopt;
        if (archive_path.has_value()) {
            try {
                const auto &archive = runtime->dvd().archive_for_path(*archive_path);
                mBrkAnimation = findBrkAnimation(archive, mBrkName);
                mBrkAnimationName = mBrkAnimation.has_value() ? mBrkName : std::string {};
            } catch (const std::exception &) {
                mBrkAnimation.reset();
                mBrkAnimationName.clear();
            }
        }
    }
    ++mMaterialAnimationVersion;
    applyStartedAnimations();
    return mBrkAnimation.has_value() ? std::optional<std::int16_t>{mBrkAnimation->frame_max} : std::nullopt;
}

void LiveActorModel::setBrkFrame(float frame) {
    if (!mBrkStarted || !mBrkAnimation.has_value() || !std::isfinite(frame)) {
        throw std::logic_error("BRK animation data is unavailable.");
    }
    mBrkFrame = frame;
}

bool LiveActorModel::startBtk(std::string_view name) {
    mMaterialAnimationSource = nullptr;
    mBtkStarted = true;
    const auto requested_name = std::string(name);
    if (mBtkName != requested_name) {
        mBtkAnimation.reset();
    }
    mBtkName = requested_name;
    if (!mBtkAnimation.has_value()) {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto archive_path = runtime != nullptr ? runtime->find_object_archive(mModelArcName) : std::nullopt;
        if (archive_path.has_value()) {
            try {
                const auto &archive = runtime->dvd().archive_for_path(*archive_path);
                mBtkAnimation = findBtkAnimation(archive, mBtkName);
                mBtkAnimationName = mBtkAnimation.has_value() ? mBtkName : std::string {};
            } catch (const std::exception &) {
                mBtkAnimation.reset();
                mBtkAnimationName.clear();
            }
        }
    }
    ++mMaterialAnimationVersion;
    applyStartedAnimations();
    return mBtkAnimation.has_value();
}

std::optional<std::int16_t> LiveActorModel::startBtp(std::string_view name) {
    mMaterialAnimationSource = nullptr;
    mBtpStarted = true;
    mBtpName = std::string(name);
    if (const auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        mBtpStartFrame = runtime->frame_index();
    } else {
        mBtpStartFrame = 0U;
    }
    mBtpAnimation = loadBtpAnimation(mBtpName);
    ++mMaterialAnimationVersion;
    applyStartedAnimations();
    return mBtpAnimation.has_value() ? std::optional<std::int16_t>{mBtpAnimation->frame_max} : std::nullopt;
}

std::optional<std::int16_t> LiveActorModel::startActionBtp(std::string_view action_name) {
    const auto resource_name = resolveActionBtpName(action_name);
    if (!resource_name.has_value()) {
        mMaterialAnimationSource = nullptr;
        mBtpStarted = true;
        mBtpName.clear();
        mBtpAnimation.reset();
        ++mMaterialAnimationVersion;
        applyStartedAnimations();
        return std::nullopt;
    }
    return startBtp(*resource_name);
}

bool LiveActorModel::hasBck(std::string_view name, std::string_view file_name) const {
    return loadBckAnimation(file_name.empty() ? name : file_name).has_value();
}

bool LiveActorModel::hasBrk(std::string_view name) const {
    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    const auto archive_path = runtime != nullptr ? runtime->find_object_archive(mModelArcName) : std::nullopt;
    if (!archive_path.has_value()) {
        return false;
    }
    try {
        return findBrkAnimation(runtime->dvd().archive_for_path(*archive_path), name).has_value();
    } catch (const std::exception&) {
        return false;
    }
}

bool LiveActorModel::hasBtk(std::string_view name) const {
    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    const auto archive_path = runtime != nullptr ? runtime->find_object_archive(mModelArcName) : std::nullopt;
    if (!archive_path.has_value()) {
        return false;
    }
    try {
        return findBtkAnimation(runtime->dvd().archive_for_path(*archive_path), name).has_value();
    } catch (const std::exception&) {
        return false;
    }
}

bool LiveActorModel::hasBtp(std::string_view name) const {
    return loadBtpAnimation(name).has_value();
}

void LiveActorModel::syncJointAnimationFrom(const LiveActorModel &source) {
    mJointAnimationSource = &source.jointAnimationSource();
    mAppliedJointAnimationVersion = ~std::uint64_t{};
    applyStartedAnimations();
}

void LiveActorModel::syncMaterialAnimationFrom(const LiveActorModel &source) {
    mMaterialAnimationSource = &source.materialAnimationSource();
    mAppliedMaterialAnimationVersion = ~std::uint64_t{};
    applyStartedAnimations();
}

void LiveActorModel::setProjmapEffectMatrix(const smgpc::render::J3dMatrix3x4 &matrix) {
    mProjmapEffectMatrix = matrix;
}

void LiveActorModel::requireLoaded() {
    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        throw std::runtime_error("Required J3D model archive/model is unavailable: " + mModelArcName);
    }
}

std::int16_t LiveActorModel::requireBck(std::string_view name, std::string_view file_name) {
    requireLoaded();
    const auto frame_max = startBck(name, file_name);
    if (!frame_max.has_value()) {
        const auto resource = file_name.empty() ? name : file_name;
        throw std::runtime_error("Required BCK animation is unavailable for " + mModelArcName + ": " +
                                 std::string(resource));
    }
    return *frame_max;
}

void LiveActorModel::draw(const smgpc::camera::CameraPose &camera_pose, const smgpc::render::J3dMatrix3x4 &actor_matrix,
                          std::uint64_t frame, DrawPass pass) {
    drawImpl(&camera_pose, nullptr, actor_matrix, frame, pass);
}

void LiveActorModel::drawModel3DFor2D(
    const smgpc::render::Model3DFor2DProjection &projection,
    const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
    DrawPass pass) {
    drawImpl(nullptr, &projection, actor_matrix, frame, pass);
}

void LiveActorModel::drawImpl(
    const smgpc::camera::CameraPose *camera_pose,
    const smgpc::render::Model3DFor2DProjection *model_3d_for_2d,
    const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t frame,
    DrawPass pass) {
    if ((camera_pose == nullptr) == (model_3d_for_2d == nullptr)) {
        throw std::logic_error(
            "LiveActorModel draw requires exactly one perspective or Model3DFor2D projection");
    }
    auto &renderer = smgpc::render::current_aurora_renderer();
    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return;
    }
    applyStartedAnimations();

    auto options = smgpc::render::J3dModelRendererDrawOptions {};
    switch (pass) {
    case DrawPass::All:
        break;
    case DrawPass::Opaque:
        options.translucent_filter = false;
        break;
    case DrawPass::Translucent:
        options.translucent_filter = true;
        break;
    }
    options.projmap_effect_matrix = mProjmapEffectMatrix;
    const auto &joint_animation = jointAnimationSource();
    if (joint_animation.mBckStarted && joint_animation.mBckAnimation.has_value()) {
        options.bck_animation_frame = joint_animation.bck_frame(frame);
    }
    const auto &material_animation = materialAnimationSource();
    if (material_animation.mBrkStarted && material_animation.mBrkAnimation.has_value()) {
        options.brk_animation_frame = material_animation.brk_frame();
    }
    if (material_animation.mBtpStarted && material_animation.mBtpAnimation.has_value()) {
        options.btp_animation_frame = material_animation.btp_frame(frame);
    }
#ifndef NDEBUG
    if (debug_model_filter_matches(mModelArcName)) {
        options.material_filter = debug_environment("SMGPC_J3D_MATERIAL_FILTER");
    }
#endif
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime != nullptr) {
        options.scene_lights = runtime->scene_lights().lights();
        options.scene_ambient_color = runtime->scene_lights().actor_ambient();
        if (runtime->j3d_pixel_update_state().has_value()) {
            options.gx_color_update = runtime->j3d_pixel_update_state()->color_update;
            options.gx_alpha_update = runtime->j3d_pixel_update_state()->alpha_update;
        }
    }
    if (model_3d_for_2d != nullptr) {
        mRenderer->draw_model_3d_for_2d(renderer, *model_3d_for_2d, actor_matrix,
                                        frame, options);
    } else {
        mRenderer->draw(renderer, *camera_pose, actor_matrix, frame, options);
    }

#ifndef NDEBUG
    if (runtime == nullptr || !runtime->should_record_j3d_packet_trace()) {
        return;
    }

    const auto runtime_packets = mRenderer->render_packets(frame, runtime->scene_lights().lights(), options);
    for (const auto &packet : runtime_packets) {
        if (packet_matches_draw_pass(pass, packet)) {
            runtime->record_j3d_packet_trace(mModelArcName, frame, draw_pass_name(pass), packet);
        }
    }
#endif
}

bool LiveActorModel::isLoaded() const {
    return mRenderer != nullptr && mRenderer->is_loaded();
}

std::size_t LiveActorModel::joint_count() {
    requireLoaded();
    return mRenderer->joint_count();
}

std::string_view LiveActorModel::model_arc_name() const {
    return mModelArcName;
}

std::optional<std::int16_t> LiveActorModel::bck_frame_max(std::string_view name) const {
    const auto &source = jointAnimationSource();
    if (&source != this) {
        return source.bck_frame_max(name);
    }
    if (mBckAnimation.has_value() && lower_copy(name) == lower_copy(mBckResourceName)) {
        return mBckAnimation->frame_max;
    }
    const auto animation = loadBckAnimation(name);
    return animation.has_value() ? std::optional<std::int16_t>{animation->frame_max} : std::nullopt;
}

std::optional<std::uint8_t> LiveActorModel::bck_attribute() const {
    const auto& source = jointAnimationSource();
    if (&source != this) {
        return source.bck_attribute();
    }
    return mBckAnimation.has_value()
               ? std::optional<std::uint8_t>{mBckAnimation->attribute}
               : std::nullopt;
}

std::optional<std::uint8_t> LiveActorModel::brk_attribute() const {
    const auto& source = materialAnimationSource();
    if (&source != this) {
        return source.brk_attribute();
    }
    return mBrkAnimation.has_value()
               ? std::optional<std::uint8_t>{mBrkAnimation->attribute}
               : std::nullopt;
}

float LiveActorModel::brk_frame() const {
    const auto& source = materialAnimationSource();
    return &source != this ? source.brk_frame() : mBrkFrame;
}

float LiveActorModel::bck_frame(std::uint64_t runtime_frame) const {
    const auto &source = jointAnimationSource();
    if (&source != this) {
        return source.bck_frame(runtime_frame);
    }
    if (!mBckStarted || !mBckAnimation.has_value()) {
        return 0.0F;
    }
    static_cast<void>(runtime_frame);
    return mBckFrame;
}

std::optional<bool> LiveActorModel::is_bck_stopped(std::uint64_t runtime_frame) const {
    const auto &source = jointAnimationSource();
    if (&source != this) {
        return source.is_bck_stopped(runtime_frame);
    }
    if (!mBckStarted || !mBckAnimation.has_value()) {
        return std::nullopt;
    }
    static_cast<void>(runtime_frame);
    return (mBckState & 1U) != 0U;
}

std::optional<bool> LiveActorModel::check_pass_bck_frame(std::uint64_t runtime_frame, float frame) const {
    const auto &source = jointAnimationSource();
    if (&source != this) {
        return source.check_pass_bck_frame(runtime_frame, frame);
    }
    if (!mBckStarted || !mBckAnimation.has_value()) {
        return std::nullopt;
    }
    static_cast<void>(runtime_frame);
    auto controller = J3DFrameCtrl{mBckAnimation->frame_max};
    controller.setAttribute(mBckAnimation->attribute);
    controller.setFrame(mBckFrame);
    controller.setRate(mBckRate);
    return controller.checkPass(frame) == TRUE;
}

float LiveActorModel::btp_frame(std::uint64_t runtime_frame) const {
    const auto &source = materialAnimationSource();
    if (&source != this) {
        return source.btp_frame(runtime_frame);
    }
    if (!mBtpStarted || !mBtpAnimation.has_value()) {
        return 0.0F;
    }
    const auto elapsed = runtime_frame >= mBtpStartFrame ? static_cast<float>(runtime_frame - mBtpStartFrame) : 0.0F;
    return smgpc::render::j3d_animation_frame(mBtpAnimation->attribute, mBtpAnimation->frame_max, elapsed);
}

std::optional<bool> LiveActorModel::is_btp_stopped(std::uint64_t runtime_frame) const {
    const auto &source = materialAnimationSource();
    if (&source != this) {
        return source.is_btp_stopped(runtime_frame);
    }
    if (!mBtpStarted || !mBtpAnimation.has_value()) {
        return std::nullopt;
    }
    const auto elapsed = runtime_frame >= mBtpStartFrame ? static_cast<float>(runtime_frame - mBtpStartFrame) : 0.0F;
    return smgpc::render::j3d_animation_stopped(mBtpAnimation->attribute, mBtpAnimation->frame_max, elapsed);
}

const smgpc::render::J3dMatrix3x4 *LiveActorModel::joint_world_matrix(
    std::string_view name, const smgpc::render::J3dMatrix3x4 &actor_matrix, std::uint64_t runtime_frame) {
    if (name.empty()) {
        return nullptr;
    }

    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return nullptr;
    }

    applyStartedAnimations();
    const auto &source = jointAnimationSource();
    const auto animation_frame = source.mBckStarted && source.mBckAnimation.has_value() ? source.bck_frame(runtime_frame) : 0.0F;
    const auto joint_matrix = mRenderer->joint_model_matrix(name, animation_frame);
    if (!joint_matrix.has_value()) {
        return nullptr;
    }

    auto [entry, inserted] = mResolvedJointMatrices.insert_or_assign(
        std::string(name), smgpc::render::j3d_concat_matrix(actor_matrix, *joint_matrix));
    static_cast<void>(inserted);
    return &entry->second;
}

void LiveActorModel::refresh_resolved_joint_matrices(const smgpc::render::J3dMatrix3x4 &actor_matrix) {
    if (mResolvedJointMatrices.empty()) {
        return;
    }

    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return;
    }

    applyStartedAnimations();
    const auto &source = jointAnimationSource();
    const auto animation_frame = source.mBckStarted && source.mBckAnimation.has_value()
                                     ? source.mBckFrame
                                     : 0.0F;
    for (auto &[name, matrix] : mResolvedJointMatrices) {
        const auto joint_matrix = mRenderer->joint_model_matrix(name, animation_frame);
        if (joint_matrix.has_value()) {
            matrix = smgpc::render::j3d_concat_matrix(actor_matrix, *joint_matrix);
        }
    }
}

std::optional<float> LiveActorModel::model_bounding_radius() {
    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return std::nullopt;
    }

    applyStartedAnimations();
    const auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    const auto runtime_frame = runtime != nullptr ? runtime->frame_index() : 0U;
    const auto &source = jointAnimationSource();
    const auto animation_frame = source.mBckStarted && source.mBckAnimation.has_value() ? source.bck_frame(runtime_frame) : 0.0F;
    return mRenderer->model_bounding_radius(animation_frame);
}

bool LiveActorModel::has_effect_texture_matrix() {
    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return false;
    }

    return std::ranges::any_of(mRenderer->render_packets(), [](const auto &packet) {
        return std::ranges::any_of(packet.tex_matrices, [](const auto &matrix) {
            // J3D texture-matrix mode 8 is the model effect/projection matrix
            // consumed by ProjmapEffectMtxSetter in the original runtime.
            return (matrix.info & 0x3FU) == 8U;
        });
    });
}

bool LiveActorModel::has_indirect_texture() {
    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return false;
    }
    return std::ranges::any_of(mRenderer->render_packets(), [](const auto &packet) {
        return packet.indirect_stage_count != 0U;
    });
}

const LiveActorModel &LiveActorModel::jointAnimationSource() const {
    return mJointAnimationSource != nullptr ? mJointAnimationSource->jointAnimationSource() : *this;
}

const LiveActorModel &LiveActorModel::materialAnimationSource() const {
    return mMaterialAnimationSource != nullptr ? mMaterialAnimationSource->materialAnimationSource() : *this;
}

void LiveActorModel::ensureLoaded() {
    if (mLoadAttempted) {
        return;
    }

    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr || mModelArcName.empty()) {
        return;
    }
    auto &renderer = smgpc::render::current_aurora_renderer();
    mLoadAttempted = true;

    const auto archive_path = runtime->find_object_archive(mModelArcName);
    if (!archive_path.has_value()) {
        runtime->note_missing_object_archive(mModelArcName);
        return;
    }

    try {
        const auto &archive = runtime->dvd().archive_for_path(*archive_path);
        const auto *model_entry = findModelEntry(archive);
        if (model_entry == nullptr) {
            runtime->note_object_texture_decode_failed(mModelArcName, "archive contains no BDL/BMD model");
            return;
        }

        if (mBckStarted && !mBckAnimation.has_value()) {
            resolveBckAnimation();
        }
        if (mBtkStarted && (!mBtkAnimation.has_value() || mBtkAnimationName != mBtkName)) {
            mBtkAnimation = findBtkAnimation(archive, mBtkName);
            mBtkAnimationName = mBtkAnimation.has_value() ? mBtkName : std::string {};
        }
        if (mBtpStarted && !mBtpAnimation.has_value() && !mBtpName.empty()) {
            mBtpAnimation = findBtpAnimation(archive, mBtpName);
        }
        if ((mBrkStarted || !mBrkName.empty()) && (!mBrkAnimation.has_value() || mBrkAnimationName != mBrkName)) {
            mBrkAnimation = findBrkAnimation(archive, mBrkName);
            mBrkAnimationName = mBrkAnimation.has_value() ? mBrkName : std::string {};
        }
        mRenderer = std::make_unique<smgpc::render::J3dModelRenderer>();
        mRenderer->load(renderer, archive.file_data(*model_entry));
        applyStartedAnimations();
        runtime->note_object_archive(mModelArcName, *archive_path);
    } catch (const std::exception &e) {
        runtime->note_object_texture_decode_failed(mModelArcName, e.what());
        mRenderer.reset();
    }
}

void LiveActorModel::resolveBckAnimation() {
    mBckAnimation = loadBckAnimation(mBckResourceName);
}

std::optional<smgpc::render::J3dBckAnimationSummary>
LiveActorModel::loadBckAnimation(std::string_view resource_name) const {
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return std::nullopt;
    }

    auto animation = std::optional<smgpc::render::J3dBckAnimationSummary>{};

    const auto try_archive = [this, runtime, resource_name, &animation](std::string_view archive_name) {
        if (archive_name.empty()) {
            return false;
        }
        const auto archive_path = runtime->find_object_archive(archive_name);
        if (!archive_path.has_value()) {
            runtime->note_missing_object_archive(archive_name);
            return false;
        }
        try {
            const auto &archive = runtime->dvd().archive_for_path(*archive_path);
            animation = findBckAnimation(archive, resource_name);
            runtime->note_object_archive(archive_name, *archive_path);
            return animation.has_value();
        } catch (const std::exception &error) {
            runtime->note_object_texture_decode_failed(archive_name, error.what());
            return false;
        }
    };

    if (!mAnimationArcName.empty() && try_archive(mAnimationArcName)) {
        return animation;
    }
    if (mAnimationArcName != mModelArcName) {
        (void)try_archive(mModelArcName);
    }
    return animation;
}

std::optional<smgpc::render::J3dBtpAnimationSummary>
LiveActorModel::loadBtpAnimation(std::string_view resource_name) const {
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr || resource_name.empty()) {
        return std::nullopt;
    }

    auto animation = std::optional<smgpc::render::J3dBtpAnimationSummary>{};
    const auto try_archive = [this, runtime, resource_name, &animation](std::string_view archive_name) {
        if (archive_name.empty()) {
            return false;
        }
        const auto archive_path = runtime->find_object_archive(archive_name);
        if (!archive_path.has_value()) {
            runtime->note_missing_object_archive(archive_name);
            return false;
        }
        try {
            const auto &archive = runtime->dvd().archive_for_path(*archive_path);
            animation = findBtpAnimation(archive, resource_name);
            runtime->note_object_archive(archive_name, *archive_path);
            return animation.has_value();
        } catch (const std::exception &error) {
            runtime->note_object_texture_decode_failed(archive_name, error.what());
            return false;
        }
    };

    if (!mAnimationArcName.empty() && try_archive(mAnimationArcName)) {
        return animation;
    }
    if (mAnimationArcName != mModelArcName) {
        (void)try_archive(mModelArcName);
    }
    return animation;
}

std::optional<std::string> LiveActorModel::resolveActionBtpName(std::string_view action_name) const {
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr || action_name.empty()) {
        return std::nullopt;
    }

    auto found_table = false;
    const auto try_archive = [this, runtime, action_name, &found_table](std::string_view archive_name)
        -> std::optional<std::string> {
        if (archive_name.empty()) {
            return std::nullopt;
        }
        const auto archive_path = runtime->find_object_archive(archive_name);
        if (!archive_path.has_value()) {
            runtime->note_missing_object_archive(archive_name);
            return std::nullopt;
        }
        try {
            const auto &archive = runtime->dvd().archive_for_path(*archive_path);
            const auto *entry = archive.find_by_basename("actoranimctrl.bcsv");
            runtime->note_object_archive(archive_name, *archive_path);
            if (entry == nullptr) {
                return std::nullopt;
            }
            found_table = true;
            const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry));
            for (auto row = 0U; row < table.entry_count(); ++row) {
                const auto name = table.get_string(row, "ActorAnimName");
                if (!name.has_value() || lower_copy(*name) != lower_copy(action_name)) {
                    continue;
                }
                const auto btp_name = table.get_string(row, "BtpName");
                if (!btp_name.has_value()) {
                    throw std::runtime_error("ActorAnimCtrl is missing its BtpName field");
                }
                return btp_name->empty() ? std::optional<std::string>{std::string(action_name)} : btp_name;
            }
            return std::optional<std::string>{std::string(action_name)};
        } catch (const std::exception &error) {
            runtime->note_object_texture_decode_failed(archive_name, error.what());
            found_table = true;
            return std::nullopt;
        }
    };

    if (!mAnimationArcName.empty()) {
        if (auto name = try_archive(mAnimationArcName); name.has_value() || found_table) {
            return name;
        }
    }
    if (mAnimationArcName != mModelArcName) {
        if (auto name = try_archive(mModelArcName); name.has_value() || found_table) {
            return name;
        }
    }
    return std::string(action_name);
}

void LiveActorModel::applyStartedAnimations() {
    if (mRenderer == nullptr) {
        return;
    }

    const auto &joint_animation = jointAnimationSource();
    if (mAppliedJointAnimationVersion != joint_animation.mJointAnimationVersion) {
        if (joint_animation.mBckStarted && joint_animation.mBckAnimation.has_value()) {
            mRenderer->set_bck_animation(*joint_animation.mBckAnimation);
        } else {
            mRenderer->clear_bck_animation();
        }
        mAppliedJointAnimationVersion = joint_animation.mJointAnimationVersion;
    }

    const auto &material_animation = materialAnimationSource();
    if (mAppliedMaterialAnimationVersion == material_animation.mMaterialAnimationVersion) {
        return;
    }
    if (material_animation.mBtkStarted) {
        if (material_animation.mBtkAnimation.has_value()) {
            mRenderer->set_btk_animation(*material_animation.mBtkAnimation);
        } else {
            mRenderer->clear_btk_animation();
        }
    } else {
        mRenderer->clear_btk_animation();
    }
    if (material_animation.mBrkStarted) {
        if (!material_animation.mBrkAnimation.has_value() ||
            !mRenderer->set_brk_animation(*material_animation.mBrkAnimation)) {
            mRenderer->clear_brk_animation();
        }
    } else {
        mRenderer->clear_brk_animation();
    }
    if (material_animation.mBtpStarted) {
        if (material_animation.mBtpAnimation.has_value()) {
            auto &renderer = smgpc::render::current_aurora_renderer();
            if (!mRenderer->set_btp_animation(renderer, *material_animation.mBtpAnimation)) {
                mRenderer->clear_btp_animation();
            }
        } else {
            mRenderer->clear_btp_animation();
        }
    } else {
        mRenderer->clear_btp_animation();
    }
    mAppliedMaterialAnimationVersion = material_animation.mMaterialAnimationVersion;
}

const smgpc::resource::RarcEntry *LiveActorModel::findModelEntry(const smgpc::resource::RarcArchive &archive) const {
    const auto requested_bdl = lower_copy(mModelArcName) + ".bdl";
    if (const auto *entry = archive.find_by_basename(requested_bdl); entry != nullptr) {
        return entry;
    }

    const auto requested_bmd = lower_copy(mModelArcName) + ".bmd";
    if (const auto *entry = archive.find_by_basename(requested_bmd); entry != nullptr) {
        return entry;
    }

    return nullptr;
}

std::optional<smgpc::render::J3dBckAnimationSummary>
LiveActorModel::findBckAnimation(const smgpc::resource::RarcArchive &archive, std::string_view resource_name) const {
    auto requested = lower_copy(resource_name.empty() ? std::string_view {mModelArcName} : resource_name);
    if (!requested.ends_with(".bck")) {
        requested += ".bck";
    }
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::render::inspect_j3d_animation(archive.file_data(*entry)).bck;
}

std::optional<smgpc::render::J3dBtkAnimationSummary> LiveActorModel::findBtkAnimation(
    const smgpc::resource::RarcArchive &archive, std::string_view name) const {
    if (name.empty()) {
        return std::nullopt;
    }
    const auto requested = lower_copy(name) + ".btk";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::render::inspect_j3d_animation(archive.file_data(*entry)).btk;
}

std::optional<smgpc::render::J3dBrkAnimationSummary> LiveActorModel::findBrkAnimation(
    const smgpc::resource::RarcArchive &archive, std::string_view name) const {
    if (name.empty()) {
        return std::nullopt;
    }
    const auto requested = lower_copy(name) + ".brk";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::render::inspect_j3d_animation(archive.file_data(*entry)).brk;
}

std::optional<smgpc::render::J3dBtpAnimationSummary>
LiveActorModel::findBtpAnimation(const smgpc::resource::RarcArchive &archive, std::string_view resource_name) const {
    if (resource_name.empty()) {
        return std::nullopt;
    }
    auto requested = lower_copy(resource_name);
    if (!requested.ends_with(".btp")) {
        requested += ".btp";
    }
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        return std::nullopt;
    }

    const auto animation = smgpc::render::inspect_j3d_animation(archive.file_data(*entry)).btp;
    if (!animation.has_value() || animation->materials.empty()) {
        return std::nullopt;
    }
    return animation;
}

}  // namespace smgpc::render::live_actor
