#pragma once

#include <array>
#include <cstdint>

#include <revolution/types.h>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/FileSelectIconID.hpp"
#include "Game/System/NerveExecutor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/compat/CameraParam.hpp"

class PartsModel;
class FileSelectModel;
class FileSelectNumber;
class FileSelectItem;
class FileSelectItemDelegatorBase;

namespace FileSelectItemSub {
    class ScaleController : public NerveExecutor {
    public:
        ScaleController();

        void exeToSmall();
        void exeToBig();
        void exeSmall();
        void exeBig();

        /* 0x8 */ f32 _8 = 1.0F;
    };

    class BlinkController : public NerveExecutor {
    public:
        explicit BlinkController(FileSelectItem* pItem);

        void exeOpen();
        void exeShut();
        void exeSleep();
        void exeBlink();
        void shut();
        void open();
        void sleep();

        /* 0x8 */ FileSelectItem* mItem = nullptr;
        /* 0xC */ s32 _C = 0;
        /* 0x10 */ s32 _10 = 0;
    };
}  // namespace FileSelectItemSub

class FileSelectItem : public LiveActor {
public:
    FileSelectItem(s32 file_no, bool is_new);
    FileSelectItem(s32 file_no, bool is_new, const FileSelectIconID& rIconId, const char* pName = "ファイルセレクトアイテム");
    ~FileSelectItem() override;

    void init(const JMapInfoIter& rIter) override;
    void movement() override;
    void appear() override;
    void makeActorDead() override;
    void forceChange(bool is_new);
    void forceChange(bool is_new, const FileSelectIconID& rIconId);
    void forceChange(const FileSelectIconID& rIconId, bool isComplete);
    void change(bool is_new);
    void change(bool is_new, const FileSelectIconID& rIconId);
    void change(const FileSelectIconID& rIconId, bool isComplete);
    void copyIconID(FileSelectIconID* pIconID) const;
    void format();
    void exeNewWait();
    void exeExistWait();
    void exeFormat();
    void exeChangeFellow();
    void invalidateSelect();
    void validateSelect();
    void appearIndex();
    void disappearIndex();
    void validateRotate();
    void onPointing();
    void offPointing();
    void clearPointing();
    void turnToFront(s32 frameCount);
    void setBasePosition(const smgpc::game::CameraParamVec3& base_position);
    void setSelectDelegator(FileSelectItemDelegatorBase* pDelegator);

    [[nodiscard]] s32 getFileNo() const;
    [[nodiscard]] bool isExist() const;
    [[nodiscard]] bool isAppeared() const;
    [[nodiscard]] bool isNew() const;
    [[nodiscard]] bool isSelectInvalid() const;
    [[nodiscard]] bool isRotateInvalid() const;
    [[nodiscard]] bool isPointing() const;
    [[nodiscard]] bool wasPointed() const;
    [[nodiscard]] bool wasPointingCleared() const;
    [[nodiscard]] bool didTurnToFront() const;
    [[nodiscard]] s32 getTurnToFrontFrameCount() const;
    [[nodiscard]] const TVec3f& getPosition() const;
    [[nodiscard]] const smgpc::game::CameraParamVec3& getBasePosition() const;

private:
    friend class FileSelectItemSub::BlinkController;

    static constexpr s32 cFellowModelCount = 5;

    void createNew();
    void createFellows();
    void createNumber();
    void killAllModels();
    void appearFellowModel();
    void emitOpen();
    void emitVanish();
    void emitCopy();
    void emitCompleteEffect();
    void deleteCompleteEffect();
    void playPointedME();
    void playPointedNotUsingME();
    [[nodiscard]] s32 getFellowModelIndex() const;
    void updatePointing();
    void updateRotate();
    void updateModelMatrix();

    s32 mFileNo = 0;
    FileSelectIconID mIconID;
    bool mIsNew = true;
    bool mIsAppeared = false;
    bool mIsSelectInvalid = false;
    bool mIsRotateInvalid = true;
    bool mIsPointing = false;
    bool mWasPointed = false;
    bool mPointingCleared = false;
    bool mTurnedToFront = false;
    s32 mTurnToFrontFrameCount = 0;
    s32 mTurnToFrontDuration = 0;
    s32 mTurnToFrontStep = 0;
    f32 mRotationVelocityY = 0.0F;
    bool mNeedsPointerScreenReset = true;
    bool mPointerStrokeHit = false;
    TVec2f mPreviousPointerScreen{};
    bool mShouldEmitCompleteEffect = false;
    bool mShouldEmitCopyEffect = false;
    FileSelectItemDelegatorBase* mDelegator = nullptr;
    FileSelectItemSub::ScaleController* mScaleCtrl = nullptr;
    FileSelectItemSub::BlinkController* mBlinkCtrl = nullptr;
    smgpc::game::CameraParamVec3 mBasePosition{};
    Mtx mPlanetMatrix{};
    Mtx mFellowMatrix{};
    PartsModel* mPlanetMapObj = nullptr;
    std::array< FileSelectModel*, cFellowModelCount > mModels{};
    FileSelectNumber* mNumber = nullptr;
};
