#pragma once

#include "Game/Screen/LayoutActor.hpp"

class ButtonPaneController;
class GalaxyMapGalaxyPlain;

namespace MR {
    class FunctorBase;
}

class FileSelectButton : public LayoutActor {
public:
    explicit FileSelectButton(const char* pName);
    ~FileSelectButton() override;

    void init(const JMapInfoIter& rIter) override;
    void appear() override;
    void kill() override;
    void control() override;

    void disappear();
    void setCallbackFunctor(const MR::FunctorBase& rStartFunctor, const MR::FunctorBase& rCopyFunctor, const MR::FunctorBase& rMiiFunctor,
                            const MR::FunctorBase& rDeleteFunctor, const MR::FunctorBase& rManualFunctor);
    void shiftSelect();
    void exeSelect();
    void exeWait();
    void exeDisappear();
    void createPaneControl();
    void createButtonController();
    void createOthers();

    [[nodiscard]] const ButtonPaneController* getButtonController(s32 index) const;

private:
    /* 0x20 */ ButtonPaneController* mButtonCtrl[5];
    /* 0x34 */ MR::FunctorBase* mCallbackFunctor[5];
    /* 0x48 */ GalaxyMapGalaxyPlain* _48;
};
