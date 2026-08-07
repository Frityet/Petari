#include "render/live_actor/LiveActorModel.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>

#include "resource/RarcArchive.hpp"
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
    mBckStarted = true;
    mBckResourceName = std::string(file_name.empty() ? name : file_name);
    if (const auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        mBckStartFrame = runtime->frame_index();
    } else {
        mBckStartFrame = 0U;
    }
    mBckAnimation = loadBckAnimation(mBckResourceName);
    applyStartedAnimations();
    return mBckAnimation.has_value() ? std::optional<std::int16_t>{mBckAnimation->frame_max} : std::nullopt;
}

std::optional<std::int16_t> LiveActorModel::startBrk(std::string_view name) {
    mBrkStarted = true;
    const auto requested_name = std::string(name);
    if (mBrkName != requested_name) {
        mBrkAnimation.reset();
    }
    mBrkName = requested_name;
    if (!mBrkAnimation.has_value()) {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto archive_path = runtime != nullptr ? runtime->find_object_archive(mModelArcName) : std::nullopt;
        if (archive_path.has_value()) {
            try {
                const auto &archive = runtime->dvd().archive_for_path(*archive_path);
                mBrkAnimation = findBrkAnimation(archive);
                mBrkAnimationName = mBrkAnimation.has_value() ? mBrkName : std::string {};
            } catch (const std::exception &) {
                mBrkAnimation.reset();
                mBrkAnimationName.clear();
            }
        }
    }
    return mBrkAnimation.has_value() ? std::optional<std::int16_t>{mBrkAnimation->frame_max} : std::nullopt;
}

void LiveActorModel::startBtk(std::string_view name) {
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
                mBtkAnimation = findBtkAnimation(archive);
                mBtkAnimationName = mBtkAnimation.has_value() ? mBtkName : std::string {};
            } catch (const std::exception &) {
                mBtkAnimation.reset();
                mBtkAnimationName.clear();
            }
        }
    }
    applyStartedAnimations();
}

void LiveActorModel::setProjmapEffectMatrix(const smgpc::render::J3dMatrix3x4 &matrix) {
    mProjmapEffectMatrix = matrix;
}

void LiveActorModel::draw(const smgpc::camera::CameraPose &camera_pose, const smgpc::render::J3dMatrix3x4 &actor_matrix,
                          std::uint64_t frame, DrawPass pass) {
    auto &renderer = smgpc::render::current_aurora_renderer();
    ensureLoaded();
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return;
    }

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
    if (mBckStarted && mBckAnimation.has_value()) {
        options.bck_animation_frame = bck_frame(frame);
    }
#ifndef NDEBUG
    if (debug_model_filter_matches(mModelArcName)) {
        options.material_filter = debug_environment("SMGPC_J3D_MATERIAL_FILTER");
    }
#endif
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime != nullptr) {
        options.scene_lights = runtime->scene_lights().lights();
        if (runtime->j3d_pixel_update_state().has_value()) {
            options.gx_color_update = runtime->j3d_pixel_update_state()->color_update;
            options.gx_alpha_update = runtime->j3d_pixel_update_state()->alpha_update;
        }
    }
    mRenderer->draw(renderer, camera_pose, actor_matrix, frame, options);

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

std::string_view LiveActorModel::model_arc_name() const {
    return mModelArcName;
}

std::optional<std::int16_t> LiveActorModel::bck_frame_max(std::string_view name) const {
    if (mBckAnimation.has_value() && lower_copy(name) == lower_copy(mBckResourceName)) {
        return mBckAnimation->frame_max;
    }
    const auto animation = loadBckAnimation(name);
    return animation.has_value() ? std::optional<std::int16_t>{animation->frame_max} : std::nullopt;
}

float LiveActorModel::bck_frame(std::uint64_t runtime_frame) const {
    if (!mBckStarted || !mBckAnimation.has_value()) {
        return 0.0F;
    }
    const auto elapsed = runtime_frame >= mBckStartFrame ? static_cast<float>(runtime_frame - mBckStartFrame) : 0.0F;
    return smgpc::render::j3d_animation_frame(mBckAnimation->attribute, mBckAnimation->frame_max, elapsed);
}

bool LiveActorModel::is_bck_stopped(std::uint64_t runtime_frame) const {
    if (!mBckStarted || !mBckAnimation.has_value()) {
        return true;
    }
    const auto elapsed = runtime_frame >= mBckStartFrame ? static_cast<float>(runtime_frame - mBckStartFrame) : 0.0F;
    return smgpc::render::j3d_animation_stopped(mBckAnimation->attribute, mBckAnimation->frame_max, elapsed);
}

void LiveActorModel::ensureLoaded() {
    if (mLoadAttempted) {
        return;
    }
    mLoadAttempted = true;

    auto &renderer = smgpc::render::current_aurora_renderer();
    auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr || mModelArcName.empty()) {
        return;
    }

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
            mBtkAnimation = findBtkAnimation(archive);
            mBtkAnimationName = mBtkAnimation.has_value() ? mBtkName : std::string {};
        }
        if ((mBrkStarted || !mBrkName.empty()) && (!mBrkAnimation.has_value() || mBrkAnimationName != mBrkName)) {
            mBrkAnimation = findBrkAnimation(archive);
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

void LiveActorModel::applyStartedAnimations() {
    if (mRenderer == nullptr) {
        return;
    }
    if (mBckStarted) {
        if (mBckAnimation.has_value()) {
            mRenderer->set_bck_animation(*mBckAnimation);
        } else {
            mRenderer->clear_bck_animation();
        }
    }
    if (mBtkStarted) {
        if (mBtkAnimation.has_value()) {
            mRenderer->set_btk_animation(*mBtkAnimation);
        } else {
            mRenderer->clear_btk_animation();
        }
    }
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

std::optional<smgpc::render::J3dBtkAnimationSummary> LiveActorModel::findBtkAnimation(const smgpc::resource::RarcArchive &archive) const {
    if (mBtkName.empty()) {
        return std::nullopt;
    }
    const auto requested = lower_copy(mBtkName) + ".btk";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::render::inspect_j3d_animation(archive.file_data(*entry)).btk;
}

std::optional<smgpc::render::J3dBrkAnimationSummary> LiveActorModel::findBrkAnimation(const smgpc::resource::RarcArchive &archive) const {
    if (mBrkName.empty()) {
        return std::nullopt;
    }
    const auto requested = lower_copy(mBrkName) + ".brk";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::render::inspect_j3d_animation(archive.file_data(*entry)).brk;
}

}  // namespace smgpc::render::live_actor
