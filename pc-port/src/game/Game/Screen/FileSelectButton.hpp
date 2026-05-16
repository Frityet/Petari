#pragma once

#include "Game/Screen/LayoutActor.hpp"

class ButtonPaneController;
class GalaxyMapGalaxyPlain;
class JMapInfoIter;

namespace MR {
class FunctorBase;
}

class FileSelectButton : public LayoutActor {
public:
    FileSelectButton(const char *pName);
    ~FileSelectButton() override;

    void init(const JMapInfoIter &rIter);
    void initWithoutIter();
    void appear() override;
    void kill() override;
    void control() override;

    void disappear();
    void setCallbackFunctor(const MR::FunctorBase &, const MR::FunctorBase &, const MR::FunctorBase &, const MR::FunctorBase &, const MR::FunctorBase &);
    void shiftSelect();
    void exeSelect();
    void exeWait();
    void exeDisappear();
    void createPaneControl();
    void createButtonController();
    void createOthers();

private:
    /* 0x20 */ ButtonPaneController *mButtonCtrl[5];
    /* 0x34 */ MR::FunctorBase *mCallbackFunctor[5];
    /* 0x48 */ GalaxyMapGalaxyPlain *_48;
};
