#pragma once

#include "Game/Screen/LayoutActor.hpp"

namespace nw4r::lyt {
    class TexMap;
}

class MiiConfirmIcon : public LayoutActor {
public:
    explicit MiiConfirmIcon(const char* pName);

    void init(const JMapInfoIter& rIter) override;
    void appear() override;

    void appear(nw4r::lyt::TexMap* pTexMap, const wchar_t* pName);
    void disappear();
    void exeAppear();
    void exeWait();
    void exeDisappear();
    bool isDisappear() const;
};
