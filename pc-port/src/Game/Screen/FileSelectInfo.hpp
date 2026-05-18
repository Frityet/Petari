#pragma once

#include "Game/Screen/LayoutActor.hpp"
#include "Game/System/NerveExecutor.hpp"

namespace FileSelectInfoSub {
    class SlideState;
    class CharaState;
}  // namespace FileSelectInfoSub

class FileSelectInfo : public LayoutActor {
public:
    FileSelectInfo(s32 nameBufferSize, const char* pName);
    ~FileSelectInfo() override;

    void init(const JMapInfoIter& rIter) override;
    void appear() override;
    void control() override;

    void disappear();
    void slide();
    void slideBack();
    void setInfo(u16* pName, s32 number, s32 starNum, s32 starPieceNum, bool isSelectedMario, bool isViewNormalEnding, bool isViewCompleteEnding,
                 const wchar_t* pDateMessage, const wchar_t* pTimeMessage, s32 missNum);
    void change();
    void forceChange();
    void exeAppear();
    void exeDisplay();
    void exeDisappear();
    void reflectInfo();

    [[nodiscard]] s32 getFileNumber() const;
    [[nodiscard]] s32 getStarNum() const;
    [[nodiscard]] s32 getStarPieceNum() const;
    [[nodiscard]] bool isSelectedMario() const;
#ifndef NDEBUG
    [[nodiscard]] s32 getMissNum() const;
    [[nodiscard]] const wchar_t* getDateMessage() const;
    [[nodiscard]] const wchar_t* getTimeMessage() const;
#endif

private:
    /* 0x20 */ s32 mNumber;
    /* 0x24 */ s32 mStarNum;
    /* 0x28 */ s32 mStarPieceNum;
    /* 0x2C */ s32 mNameBufferSize;
    /* 0x30 */ wchar_t* mName;
    /* 0x34 */ wchar_t mDateMessage[32];
    /* 0x74 */ wchar_t mTimeMessage[32];
    /* 0xB4 */ s32 mMissNum;
    /* 0xB8 */ bool mIsSelectedMarioPrev;
    /* 0xB9 */ bool mIsSelectedMario;
    /* 0xBA */ bool mIsViewNormalEnding;
    /* 0xBB */ bool mIsViewCompleteEnding;
    /* 0xBC */ FileSelectInfoSub::SlideState* mSlideState;
    /* 0xC0 */ FileSelectInfoSub::CharaState* mCharaState;
};

namespace FileSelectInfoSub {
    class SlideState : public NerveExecutor {
    public:
        explicit SlideState(FileSelectInfo* pHost);

        void exeNormalPos();
        void exeSliding();
        void exeSlidePos();
        void exeSlidingBack();

    private:
        /* 0x8 */ FileSelectInfo* mHost;
    };

    class CharaState : public NerveExecutor {
    public:
        explicit CharaState(FileSelectInfo* pHost);

        void exeMario();
        void exeToLuigi();
        void exeLuigi();
        void exeToMario();

    private:
        /* 0x8 */ FileSelectInfo* mHost;
    };
}  // namespace FileSelectInfoSub
