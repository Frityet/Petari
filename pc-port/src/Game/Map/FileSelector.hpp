#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <revolution.h>

#include "Game/System/NerveExecutor.hpp"
#include "Game/compat/CameraParam.hpp"
#include "RendererService.hpp"

class FileSelectCameraController;
class FileSelectItem;
class FileSelectSky;
class HitSensor;
class MiiSelect;
class TitleSequenceProduct;

class FileSelector : public NerveExecutor {
public:
    FileSelector();
    ~FileSelector();

    void update();
    void draw(smgpc::render::IRendererEngine& renderer);
    bool receiveOtherMsg(u32 msg, HitSensor* pSender = nullptr, HitSensor* pReceiver = nullptr);

    void createCameraController();
    void createTitle();
    void createSky();
    void createMiiSelect();
    void createFileItems();
    void appearAllItems();
    void initAllItems();
    void invalidateSelectAll();
    void validateSelectAll();
    void validateRotateAllItems();
    void calcBasePos(float ratio);
    void exeWaitBind();
    void exeTitle();
    void exeTitleEnd();
    void exeRFLError();
    void exeRFLWait();
    void exeFileSelect();

    [[nodiscard]] std::uint64_t getSkyStep() const;
    [[nodiscard]] bool isTitleActive() const;
    [[nodiscard]] bool isTitleStarted() const;
    [[nodiscard]] bool isTitleEnded() const;
    [[nodiscard]] bool isCameraAtFarPoint() const;
    [[nodiscard]] bool isFileSelectStarted() const;
    [[nodiscard]] bool didAppearAllItems() const;
    [[nodiscard]] bool didInitAllItems() const;
    [[nodiscard]] bool didInvalidateSelectAll() const;
    [[nodiscard]] bool didValidateRotateAllItems() const;
    [[nodiscard]] float getBasePosRatio() const;
    [[nodiscard]] s32 getMiiValidIndexCollectionCount() const;
    [[nodiscard]] s32 getItemCount() const;
    [[nodiscard]] s32 getAppearedItemCount() const;
    [[nodiscard]] s32 getSelectInvalidItemCount() const;
    [[nodiscard]] s32 getRotateInvalidItemCount() const;
    [[nodiscard]] s32 getItemFileNo(s32 index) const;
    [[nodiscard]] const smgpc::game::CameraParamVec3& getItemBasePosition(s32 index) const;
    [[nodiscard]] const smgpc::game::CameraParamVec3& getItemPosition(s32 index) const;

private:
    static constexpr s32 cItemCount = 6;

    std::unique_ptr< FileSelectCameraController > mCameraController;
    std::unique_ptr< TitleSequenceProduct > mTitleSeq;
    std::unique_ptr< FileSelectSky > mSky;
    std::unique_ptr< MiiSelect > mMiiSelect;
    std::array< std::unique_ptr< FileSelectItem >, cItemCount > mItems{};
    std::array< smgpc::game::CameraParamVec3, cItemCount > mItemBasePositions{};
    float mBasePosRatio = 0.0F;
    bool mTitleStarted = false;
    bool mTitleEnded = false;
    bool mAllItemsAppeared = false;
    bool mAllItemsInitialized = false;
    bool mSelectAllInvalidated = false;
    bool mRotateAllItemsValidated = false;
    bool mFileSelectStarted = false;
};
