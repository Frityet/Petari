#include "Game/Map/FileSelectItem.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <memory>
#include <string>
#include <string_view>

#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/J3dModelRenderer.hpp"
#include "Game/compat/RarcArchive.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace {
    [[nodiscard]] float smooth(float current, float target) {
        return (current * 0.95F) + (target * 0.05F);
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }

        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] bool ends_with_lower(std::string_view value, std::string_view suffix) {
        const auto lower = lower_copy(value);
        return lower.ends_with(suffix);
    }

    [[nodiscard]] const smgpc::game::RarcEntry* find_entry_by_basename(const smgpc::game::RarcArchive& archive, std::string_view name) {
        const auto requested = lower_copy(name);
        const auto it =
            std::ranges::find_if(archive.entries(), [&requested](const auto& entry) { return lower_copy(base_name(entry.path)) == requested; });
        return it == archive.entries().end() ? nullptr : &*it;
    }

    [[nodiscard]] const smgpc::game::RarcEntry* find_first_entry_with_suffix(const smgpc::game::RarcArchive& archive, std::string_view suffix) {
        const auto it = std::ranges::find_if(archive.entries(), [suffix](const auto& entry) { return ends_with_lower(entry.path, suffix); });
        return it == archive.entries().end() ? nullptr : &*it;
    }

    [[nodiscard]] std::string model_file_name_for_object(std::string_view object_name) {
        return lower_copy(object_name) + ".bdl";
    }
}  // namespace

FileSelectItem::FileSelectItem(s32 file_no, bool is_new) : mFileNo(file_no), mIsNew(is_new) {
}

void FileSelectItem::appear() {
    mIsAppeared = true;
    mIsSelectInvalid = false;
}

void FileSelectItem::update(const smgpc::game::CameraParamVec3& base_position) {
    mBasePosition = base_position;
    mPosition = smgpc::game::CameraParamVec3{
        .x = smooth(mPosition.x, mBasePosition.x),
        .y = smooth(mPosition.y, mBasePosition.y),
        .z = smooth(mPosition.z, mBasePosition.z),
    };
}

void FileSelectItem::draw(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose) {
    if (!mIsAppeared) {
        return;
    }

    if (!mLoadAttempted) {
        mLoadAttempted = true;

        auto* runtime = smgpc::game::RuntimeContext::try_instance();
        if (runtime != nullptr) {
            const auto object_name = std::string_view(mIsNew ? "FileSelectDataPlanet" : "FileSelectDataMario");
            const auto archive_path = runtime->find_object_archive(object_name);
            if (!archive_path.has_value()) {
                runtime->note_missing_object_archive(object_name);
            } else {
                try {
                    const auto archive = smgpc::game::RarcArchive::from_file(*archive_path);
                    const auto model_name = model_file_name_for_object(object_name);
                    const auto* model_entry = find_entry_by_basename(archive, model_name);
                    if (model_entry == nullptr) {
                        model_entry = find_first_entry_with_suffix(archive, ".bdl");
                    }

                    if (model_entry == nullptr) {
                        runtime->note_object_texture_decode_failed(object_name, "archive contains no BDL model");
                    } else {
                        mModelRenderer = std::make_unique< smgpc::game::J3dModelRenderer >();
                        mModelRenderer->load(renderer, archive.file_data(*model_entry));

                        if (const auto* bck = find_first_entry_with_suffix(archive, ".bck"); bck != nullptr) {
                            const auto animation = smgpc::game::inspect_j3d_animation(archive.file_data(*bck));
                            if (animation.bck.has_value()) {
                                mModelRenderer->set_bck_animation(*animation.bck);
                            }
                        }
                        if (const auto* btk = find_first_entry_with_suffix(archive, ".btk"); btk != nullptr) {
                            const auto animation = smgpc::game::inspect_j3d_animation(archive.file_data(*btk));
                            if (animation.btk.has_value()) {
                                mModelRenderer->set_btk_animation(*animation.btk);
                            }
                        }

                        runtime->note_object_archive(object_name, *archive_path);
                    }
                } catch (const std::exception& e) {
                    runtime->note_object_texture_decode_failed(object_name, e.what());
                    mModelRenderer.reset();
                }
            }
        }
    }

    if (mModelRenderer == nullptr || !mModelRenderer->is_loaded()) {
        return;
    }

    constexpr auto c_new_item_model_scale = 30.0F;
    constexpr auto c_new_item_local_y_offset = 900.0F;
    const auto model_position = smgpc::game::CameraParamVec3{
        .x = mPosition.x,
        .y = mPosition.y + (mIsNew ? c_new_item_local_y_offset : 0.0F),
        .z = mPosition.z,
    };
    const auto actor_matrix = smgpc::game::j3d_matrix_from_translation_scale(model_position, c_new_item_model_scale);
    mModelRenderer->draw(renderer, camera_pose, actor_matrix, mDrawFrame++);
}

void FileSelectItem::forceChange(bool is_new) {
    mIsNew = is_new;
    mLoadAttempted = false;
    mModelRenderer.reset();
}

void FileSelectItem::invalidateSelect() {
    mIsSelectInvalid = true;
}

void FileSelectItem::validateSelect() {
    mIsSelectInvalid = false;
}

void FileSelectItem::validateRotate() {
    mIsRotateInvalid = false;
}

s32 FileSelectItem::getFileNo() const {
    return mFileNo;
}

bool FileSelectItem::isAppeared() const {
    return mIsAppeared;
}

bool FileSelectItem::isNew() const {
    return mIsNew;
}

bool FileSelectItem::isSelectInvalid() const {
    return mIsSelectInvalid;
}

bool FileSelectItem::isRotateInvalid() const {
    return mIsRotateInvalid;
}

const smgpc::game::CameraParamVec3& FileSelectItem::getPosition() const {
    return mPosition;
}

const smgpc::game::CameraParamVec3& FileSelectItem::getBasePosition() const {
    return mBasePosition;
}
