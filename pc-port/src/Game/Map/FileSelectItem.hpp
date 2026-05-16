#pragma once

#include <cstdint>
#include <memory>

#include <revolution/types.h>

#include "Game/compat/CameraParam.hpp"
#include "Game/compat/J3dModelRenderer.hpp"

namespace smgpc::game {
    struct CameraPoseCompat;
}  // namespace smgpc::game

class FileSelectItem {
public:
    FileSelectItem(s32 file_no, bool is_new);

    void appear();
    void update(const smgpc::game::CameraParamVec3& base_position);
    void draw(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose);
    void forceChange(bool is_new);
    void invalidateSelect();
    void validateSelect();
    void validateRotate();

    [[nodiscard]] s32 getFileNo() const;
    [[nodiscard]] bool isAppeared() const;
    [[nodiscard]] bool isNew() const;
    [[nodiscard]] bool isSelectInvalid() const;
    [[nodiscard]] bool isRotateInvalid() const;
    [[nodiscard]] const smgpc::game::CameraParamVec3& getPosition() const;
    [[nodiscard]] const smgpc::game::CameraParamVec3& getBasePosition() const;

private:
    s32 mFileNo = 0;
    bool mIsNew = true;
    bool mIsAppeared = false;
    bool mIsSelectInvalid = false;
    bool mIsRotateInvalid = true;
    smgpc::game::CameraParamVec3 mPosition{};
    smgpc::game::CameraParamVec3 mBasePosition{};
    std::unique_ptr< smgpc::game::J3dModelRenderer > mModelRenderer{};
    std::uint64_t mDrawFrame = 0U;
    bool mLoadAttempted = false;
};
