#pragma once

#include <array>
#include <string>
#include <vector>

#include <revolution/types.h>

#include "Game/Map/FileSelectIconID.hpp"
#include "Game/Screen/LayoutActor.hpp"

namespace nw4r::lyt {
    class TexMap;
}

class MiiSelect : public LayoutActor {
public:
    MiiSelect(const char* pName);

    void init(const JMapInfoIter& rIter) override;
    void initWithoutIter();
    void appear() override;
    void control() override;

    void disappear();
    [[nodiscard]] bool isAppearing() const;
    [[nodiscard]] bool isSelected() const;
    [[nodiscard]] bool isDummySelected() const;
    void getSelectedID(FileSelectIconID* pSelectedID) const;
    nw4r::lyt::TexMap* getSelectedMiiTexMap();
    void admitIcon();
    void prohibitIcon(const FileSelectIconID& rIconID);
    void invalidateSpecialMii(FileSelectIconID::EFellowID fellowID);
    void validateAllSpecialMii();
    void exeAppear();
    void exeWait();
    void exeScrollRight();
    void exeScrollLeft();
    void exeSelected();
    void exeDisappear();
    void exeDummySelected();
    void collectValidMiiIndex();
    void refresh();
    void getIconID(FileSelectIconID* pIconID, s32 index) const;
    void onSelect(s32 index, nw4r::lyt::TexMap* pTexMap);
    void onSelectDummy();
    [[nodiscard]] s32 getIconNum() const;

    [[nodiscard]] s32 getCollectValidMiiIndexCount() const;

private:
    void rebuildIconList();

    static constexpr s32 cSpecialMiiCount = 5;

    s32 mCollectValidMiiIndexCount = 0;
    std::array< bool, cSpecialMiiCount > mSpecialMiiValid{};
    std::vector< FileSelectIconID > mIconIds;
    std::vector< std::wstring > mIconNames;
    FileSelectIconID mProhibitedIcon;
    bool mHasProhibitedIcon = false;
    s32 mSelectedIndex = 0;
    nw4r::lyt::TexMap* mSelectedTexMap = nullptr;
};
