#include "Game/Screen/MiiSelect.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

namespace {
    NEW_NERVE(MiiSelectNrvAppear, MiiSelect, Appear);
    NEW_NERVE(MiiSelectNrvWait, MiiSelect, Wait);
    NEW_NERVE(MiiSelectNrvScrollRight, MiiSelect, ScrollRight);
    NEW_NERVE(MiiSelectNrvScrollLeft, MiiSelect, ScrollLeft);
    NEW_NERVE(MiiSelectNrvSelected, MiiSelect, Selected);
    NEW_NERVE(MiiSelectNrvDisappear, MiiSelect, Disappear);
    NEW_NERVE(MiiSelectNrvDummySelected, MiiSelect, DummySelected);

    constexpr std::array< const wchar_t*, 5 > cFellowNames{
        L"Mario", L"Luigi", L"Yoshi", L"Kinopio", L"Peach",
    };
    constexpr auto cIconsPerPage = 8;
    constexpr std::array< TVec2f, cIconsPerPage > cIconPaneCenters{
        TVec2f{-168.0F, 57.0F},  TVec2f{-56.0F, 57.0F},  TVec2f{56.0F, 57.0F},  TVec2f{168.0F, 57.0F},
        TVec2f{-168.0F, -57.0F}, TVec2f{-56.0F, -57.0F}, TVec2f{56.0F, -57.0F}, TVec2f{168.0F, -57.0F},
    };

    [[nodiscard]] std::wstring widen_ascii(std::string_view value) {
        auto result = std::wstring();
        result.reserve(value.size());
        for (const auto ch : value) {
            result.push_back(static_cast< wchar_t >(static_cast< unsigned char >(ch)));
        }
        return result;
    }
}  // namespace

MiiSelect::MiiSelect(const char* pName) : LayoutActor(pName, true) {
    validateAllSpecialMii();
    rebuildIconList();
}

void MiiSelect::init(const JMapInfoIter&) {
    initLayoutManager("MiiSelect", 1);
    MR::connectToSceneLayout(this);
    initNerve(&MiiSelectNrvAppear::sInstance);
    kill();
}

void MiiSelect::initWithoutIter() {
    init(JMapInfoIter());
}

void MiiSelect::appear() {
    LayoutActor::appear();
    setNerve(&MiiSelectNrvAppear::sInstance);
    mCurrentPageStart = 0;
    mSelectedIndex = 0;
    mSelectedTexMap = nullptr;
    refresh();
    MR::setTextBoxMessageRecursive(this, "TxtName", L"");
}

void MiiSelect::control() {
    if (!isNerve(&MiiSelectNrvWait::sInstance)) {
        return;
    }

    const auto icon_count = getIconNum();
    if (icon_count <= 0) {
        return;
    }

    auto pointed_index = -1;
    for (auto i = 0; i < cIconsPerPage; ++i) {
        const auto icon_index = mCurrentPageStart + i;
        if (icon_index >= icon_count) {
            break;
        }

        auto pane_name = std::array< char, 8U >{};
        std::snprintf(pane_name.data(), pane_name.size(), "Mii%02d", i + 1);
        if (MR::isStarPointerPointingPane(this, pane_name.data(), 0, true, "弱")) {
            pointed_index = icon_index;
            break;
        }
    }

    if (pointed_index < 0 && MR::isCorePadPointInScreen(WPAD_CHAN0)) {
        auto pointer = TVec2f{};
        MR::getCorePadPointingPosBasedOnScreen(&pointer, WPAD_CHAN0);
        for (auto i = 0; i < cIconsPerPage; ++i) {
            const auto icon_index = mCurrentPageStart + i;
            if (icon_index >= icon_count) {
                break;
            }

            const auto center_x = 320.0F + cIconPaneCenters[static_cast< std::size_t >(i)].x;
            const auto center_y = 228.0F + cIconPaneCenters[static_cast< std::size_t >(i)].y;
            if (std::fabs(pointer.x - center_x) <= 64.0F && std::fabs(pointer.y - center_y) <= 64.0F) {
                pointed_index = icon_index;
                break;
            }
        }
    }

    if (pointed_index >= 0 && static_cast< std::size_t >(pointed_index) < mIconNames.size()) {
        MR::setTextBoxMessageRecursive(this, "TxtName", mIconNames[static_cast< std::size_t >(pointed_index)].c_str());
        if (MR::testDPDMenuPadDecideTrigger()) {
            onSelect(pointed_index, nullptr);
            return;
        }
    } else {
        MR::setTextBoxMessageRecursive(this, "TxtName", L"");
    }

    if (icon_count > cIconsPerPage && (MR::testCorePadTriggerRight(WPAD_CHAN0) || MR::testSubPadStickTriggerRight(WPAD_CHAN0) ||
                                       (MR::isStarPointerPointingPane(this, "Right", 0, true, "弱") && MR::testDPDMenuPadDecideTrigger()))) {
        setNerve(&MiiSelectNrvScrollRight::sInstance);
    } else if (icon_count > cIconsPerPage && (MR::testCorePadTriggerLeft(WPAD_CHAN0) || MR::testSubPadStickTriggerLeft(WPAD_CHAN0) ||
                                              (MR::isStarPointerPointingPane(this, "Left", 0, true, "弱") && MR::testDPDMenuPadDecideTrigger()))) {
        setNerve(&MiiSelectNrvScrollLeft::sInstance);
    }
}

void MiiSelect::disappear() {
    setNerve(&MiiSelectNrvDisappear::sInstance);
}

bool MiiSelect::isAppearing() const {
    return isNerve(&MiiSelectNrvAppear::sInstance);
}

bool MiiSelect::isSelected() const {
    return isNerve(&MiiSelectNrvSelected::sInstance);
}

bool MiiSelect::isDummySelected() const {
    return isNerve(&MiiSelectNrvDummySelected::sInstance);
}

void MiiSelect::getSelectedID(FileSelectIconID* pSelectedID) const {
    getIconID(pSelectedID, mSelectedIndex);
}

nw4r::lyt::TexMap* MiiSelect::getSelectedMiiTexMap() {
    return mSelectedTexMap;
}

void MiiSelect::admitIcon() {
    mHasProhibitedIcon = false;
    rebuildIconList();
}

void MiiSelect::prohibitIcon(const FileSelectIconID& rIconID) {
    mHasProhibitedIcon = true;
    mProhibitedIcon.set(rIconID);
    rebuildIconList();
}

void MiiSelect::invalidateSpecialMii(FileSelectIconID::EFellowID fellowID) {
    const auto index = static_cast< std::size_t >(fellowID);
    if (index < mSpecialMiiValid.size()) {
        mSpecialMiiValid[index] = false;
    }
    rebuildIconList();
}

void MiiSelect::validateAllSpecialMii() {
    mSpecialMiiValid.fill(true);
    rebuildIconList();
}

void MiiSelect::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "Appear", 0);
        MR::startSystemSE("SE_SY_FILE_SEL_MIISEL_OPEN", -1, -1);
        MR::setTextBoxNumberRecursive(this, "TxtPage", getIconNum() > 0 ? 1 : 0);
    }

    if (MR::isAnimStopped(this, 0)) {
        setNerve(&MiiSelectNrvWait::sInstance);
    }
}

void MiiSelect::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "Wait", 0);
        MR::setTextBoxNumberRecursive(this, "TxtPage", getIconNum() > 0 ? (mCurrentPageStart / cIconsPerPage) + 1 : 0);
    }
}

void MiiSelect::exeScrollRight() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "NextMii", 0);
        if (getIconNum() > cIconsPerPage) {
            mCurrentPageStart += cIconsPerPage;
            if (mCurrentPageStart >= getIconNum()) {
                mCurrentPageStart = 0;
            }
        }
        MR::startSystemSE("SE_SY_FILE_SEL_MIISEL_SCRL", -1, -1);
        refresh();
    }

    if (MR::isAnimStopped(this, 0)) {
        setNerve(&MiiSelectNrvWait::sInstance);
    }
}

void MiiSelect::exeScrollLeft() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "PreviousMii", 0);
        if (getIconNum() > cIconsPerPage) {
            mCurrentPageStart -= cIconsPerPage;
            if (mCurrentPageStart < 0) {
                mCurrentPageStart = ((getIconNum() - 1) / cIconsPerPage) * cIconsPerPage;
            }
        }
        MR::startSystemSE("SE_SY_FILE_SEL_MIISEL_SCRL", -1, -1);
        refresh();
    }

    if (MR::isAnimStopped(this, 0)) {
        setNerve(&MiiSelectNrvWait::sInstance);
    }
}

void MiiSelect::exeSelected() {
}

void MiiSelect::exeDisappear() {
    if (MR::isFirstStep(this)) {
        MR::startAnim(this, "End", 0);
    }

    if (MR::isAnimStopped(this, 0)) {
        kill();
    }
}

void MiiSelect::exeDummySelected() {
}

void MiiSelect::collectValidMiiIndex() {
    ++mCollectValidMiiIndexCount;
    rebuildIconList();
}

void MiiSelect::refresh() {
    rebuildIconList();
    if (mCurrentPageStart >= getIconNum()) {
        mCurrentPageStart = getIconNum() > 0 ? ((getIconNum() - 1) / cIconsPerPage) * cIconsPerPage : 0;
    }
    if (mCurrentPageStart < 0) {
        mCurrentPageStart = 0;
    }
}

void MiiSelect::getIconID(FileSelectIconID* pIconID, s32 index) const {
    if (pIconID == nullptr) {
        return;
    }

    if (index < 0 || static_cast< std::size_t >(index) >= mIconIds.size()) {
        pIconID->setFellowID(FileSelectIconID::Mario);
        return;
    }

    pIconID->set(mIconIds[static_cast< std::size_t >(index)]);
}

void MiiSelect::onSelect(s32 index, nw4r::lyt::TexMap* pTexMap) {
    if (index < 0 || static_cast< std::size_t >(index) >= mIconIds.size()) {
        return;
    }

    mSelectedIndex = index;
    mSelectedTexMap = pTexMap;
    setNerve(&MiiSelectNrvSelected::sInstance);
}

void MiiSelect::onSelectDummy() {
    setNerve(&MiiSelectNrvDummySelected::sInstance);
}

s32 MiiSelect::getIconNum() const {
    return static_cast< s32 >(mIconIds.size());
}

s32 MiiSelect::getCollectValidMiiIndexCount() const {
    return mCollectValidMiiIndexCount;
}

void MiiSelect::rebuildIconList() {
    mIconIds.clear();
    mIconNames.clear();

    for (auto i = std::size_t{}; i < mSpecialMiiValid.size(); ++i) {
        if (!mSpecialMiiValid[i]) {
            continue;
        }

        auto icon_id = FileSelectIconID();
        icon_id.setFellowID(static_cast< FileSelectIconID::EFellowID >(i));
        if (mHasProhibitedIcon && icon_id == mProhibitedIcon) {
            continue;
        }

        mIconIds.push_back(icon_id);
        mIconNames.emplace_back(cFellowNames[i]);
    }

    const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return;
    }

    for (const auto& entry : runtime->rfl().valid_miis()) {
        auto icon_id = FileSelectIconID();
        icon_id.setMiiIndex(static_cast< u16 >(std::clamp(entry.index, 0, 0xffff)));
        if (mHasProhibitedIcon && icon_id == mProhibitedIcon) {
            continue;
        }

        mIconIds.push_back(icon_id);
        mIconNames.push_back(widen_ascii(entry.name));
    }
}
