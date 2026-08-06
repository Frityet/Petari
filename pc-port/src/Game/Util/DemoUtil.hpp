#pragma once

class LiveActor;
class JMapInfoIter;

namespace MR {
    class FunctorBase;
}

namespace MR {
    bool tryRegisterDemoCast(LiveActor* pActor, const JMapInfoIter& rIter);
    bool tryRegisterDemoActionFunctor(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pActionName);
    void registerDemoSimpleCastAll(LiveActor* pActor);
    bool tryStartDemoWithoutCinemaFrame(const LiveActor *pActor, const char *pDemoName);
    bool tryStartDemoMarioPuppetable(const LiveActor *pActor, const char *pDemoName);
    void endDemo(const LiveActor *pActor, const char *pDemoName);
}  // namespace MR
