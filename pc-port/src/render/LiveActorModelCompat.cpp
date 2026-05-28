#include "render/LiveActorModelCompat.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>

#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {
    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] bool ends_with_lower(std::string_view value, std::string_view suffix) {
        return lower_copy(value).ends_with(suffix);
    }

    [[nodiscard]] const smgpc::compat::RarcEntry *find_first_entry_with_suffix(const smgpc::compat::RarcArchive &archive, std::string_view suffix) {
        const auto it = std::ranges::find_if(archive.entries(), [suffix](const auto &entry) { return ends_with_lower(entry.path, suffix); });
        return it == archive.entries().end() ? nullptr : &*it;
    }

    [[nodiscard]] const char *draw_pass_name(LiveActorModelCompat::DrawPass pass) {
        switch (pass) {
        case LiveActorModelCompat::DrawPass::All:
            return "All";
        case LiveActorModelCompat::DrawPass::Opaque:
            return "Opaque";
        case LiveActorModelCompat::DrawPass::Translucent:
            return "Translucent";
        }

        return "Unknown";
    }

    [[nodiscard]] bool packet_matches_draw_pass(LiveActorModelCompat::DrawPass pass, const smgpc::compat::J3dRendererPacketState &packet) {
        switch (pass) {
        case LiveActorModelCompat::DrawPass::All:
            return true;
        case LiveActorModelCompat::DrawPass::Opaque:
            return packet.draw_buffer_opaque;
        case LiveActorModelCompat::DrawPass::Translucent:
            return !packet.draw_buffer_opaque;
        }

        return false;
    }

#ifndef NDEBUG
    [[nodiscard]] std::string_view debug_environment(std::string_view name) {
        const auto key = std::string(name);
        const auto *value = std::getenv(key.c_str());
        return value == nullptr ? std::string_view{} : std::string_view(value);
    }

    [[nodiscard]] bool debug_model_filter_matches(std::string_view model_name) {
        const auto filter = debug_environment("SMGPC_J3D_MODEL_FILTER");
        return filter.empty() || filter == model_name;
    }
#endif
}  // namespace

LiveActorModelCompat::LiveActorModelCompat(std::string model_arc_name, std::string animation_arc_name)
    : mModelArcName(std::move(model_arc_name)), mAnimationArcName(std::move(animation_arc_name)) {
}

void LiveActorModelCompat::startBck(std::string_view, std::string_view) {
    mBckStarted = true;
    applyStartedAnimations();
}

std::optional<std::int16_t> LiveActorModelCompat::startBrk(std::string_view name) {
    mBrkStarted = true;
    const auto requested_name = std::string(name);
    if (mBrkName != requested_name) {
        mBrkAnimation.reset();
    }
    mBrkName = requested_name;
    if (!mBrkAnimation.has_value()) {
        auto *runtime = smgpc::compat::RuntimeContext::try_instance();
        const auto archive_path = runtime != nullptr ? runtime->find_object_archive(mModelArcName) : std::nullopt;
        if (archive_path.has_value()) {
            try {
                const auto &archive = runtime->dvd().archive_for_path(*archive_path);
                mBrkAnimation = findBrkAnimation(archive);
                mBrkAnimationName = mBrkAnimation.has_value() ? mBrkName : std::string{};
            } catch (const std::exception &) {
                mBrkAnimation.reset();
                mBrkAnimationName.clear();
            }
        }
    }
    return mBrkAnimation.has_value() ? std::optional<std::int16_t>{mBrkAnimation->frame_max} : std::nullopt;
}

void LiveActorModelCompat::startBtk(std::string_view) {
    mBtkStarted = true;
    applyStartedAnimations();
}

void LiveActorModelCompat::setProjmapEffectMatrix(const smgpc::compat::J3dMatrix3x4 &matrix) {
    mProjmapEffectMatrix = matrix;
}

void LiveActorModelCompat::draw(smgpc::render::IRendererEngine &renderer, const smgpc::compat::CameraPoseCompat &camera_pose,
                                const smgpc::compat::J3dMatrix3x4 &actor_matrix, std::uint64_t frame, DrawPass pass) {
    ensureLoaded(renderer);
    if (mRenderer == nullptr || !mRenderer->is_loaded()) {
        return;
    }

    auto options = smgpc::compat::J3dModelRendererDrawOptions{};
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
#ifndef NDEBUG
    if (debug_model_filter_matches(mModelArcName)) {
        options.material_filter = debug_environment("SMGPC_J3D_MATERIAL_FILTER");
    }
#endif
    auto *runtime = smgpc::compat::RuntimeContext::try_instance();
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

bool LiveActorModelCompat::isLoaded() const {
    return mRenderer != nullptr && mRenderer->is_loaded();
}

std::string_view LiveActorModelCompat::model_arc_name() const {
    return mModelArcName;
}

void LiveActorModelCompat::ensureLoaded(smgpc::render::IRendererEngine &renderer) {
    if (mLoadAttempted) {
        return;
    }
    mLoadAttempted = true;

    auto *runtime = smgpc::compat::RuntimeContext::try_instance();
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

        mBckAnimation = findBckAnimation(archive);
        mBtkAnimation = findBtkAnimation(archive);
        if ((mBrkStarted || !mBrkName.empty()) && (!mBrkAnimation.has_value() || mBrkAnimationName != mBrkName)) {
            mBrkAnimation = findBrkAnimation(archive);
            mBrkAnimationName = mBrkAnimation.has_value() ? mBrkName : std::string{};
        }
        mRenderer = std::make_unique<smgpc::compat::J3dModelRenderer>();
        mRenderer->load(renderer, archive.file_data(*model_entry));
        applyStartedAnimations();
        runtime->note_object_archive(mModelArcName, *archive_path);
    } catch (const std::exception &e) {
        runtime->note_object_texture_decode_failed(mModelArcName, e.what());
        mRenderer.reset();
    }
}

void LiveActorModelCompat::applyStartedAnimations() {
    if (mRenderer == nullptr) {
        return;
    }
    if (mBckStarted && mBckAnimation.has_value()) {
        mRenderer->set_bck_animation(*mBckAnimation);
    }
    if (mBtkStarted && mBtkAnimation.has_value()) {
        mRenderer->set_btk_animation(*mBtkAnimation);
    }
}

const smgpc::compat::RarcEntry *LiveActorModelCompat::findModelEntry(const smgpc::compat::RarcArchive &archive) const {
    const auto requested_bdl = lower_copy(mModelArcName) + ".bdl";
    if (const auto *entry = archive.find_by_basename(requested_bdl); entry != nullptr) {
        return entry;
    }

    const auto requested_bmd = lower_copy(mModelArcName) + ".bmd";
    if (const auto *entry = archive.find_by_basename(requested_bmd); entry != nullptr) {
        return entry;
    }

    if (const auto *entry = find_first_entry_with_suffix(archive, ".bdl"); entry != nullptr) {
        return entry;
    }

    return find_first_entry_with_suffix(archive, ".bmd");
}

std::optional<smgpc::compat::J3dBckAnimationSummary> LiveActorModelCompat::findBckAnimation(const smgpc::compat::RarcArchive &archive) const {
    const auto requested = lower_copy(mModelArcName) + ".bck";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        entry = find_first_entry_with_suffix(archive, ".bck");
    }
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::compat::inspect_j3d_animation(archive.file_data(*entry)).bck;
}

std::optional<smgpc::compat::J3dBtkAnimationSummary> LiveActorModelCompat::findBtkAnimation(const smgpc::compat::RarcArchive &archive) const {
    const auto requested = lower_copy(mModelArcName) + ".btk";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        entry = find_first_entry_with_suffix(archive, ".btk");
    }
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::compat::inspect_j3d_animation(archive.file_data(*entry)).btk;
}

std::optional<smgpc::compat::J3dBrkAnimationSummary> LiveActorModelCompat::findBrkAnimation(const smgpc::compat::RarcArchive &archive) const {
    const auto requested = lower_copy(mBrkName.empty() ? std::string_view{mModelArcName} : std::string_view{mBrkName}) + ".brk";
    auto *entry = archive.find_by_basename(requested);
    if (entry == nullptr) {
        entry = find_first_entry_with_suffix(archive, ".brk");
    }
    if (entry == nullptr) {
        return std::nullopt;
    }

    return smgpc::compat::inspect_j3d_animation(archive.file_data(*entry)).brk;
}
