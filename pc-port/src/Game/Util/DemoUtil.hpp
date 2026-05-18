#pragma once

class LiveActor;

namespace MR {
    bool tryStartDemoWithoutCinemaFrame(const LiveActor *pActor, const char *pDemoName);
    bool tryStartDemoMarioPuppetable(const LiveActor *pActor, const char *pDemoName);
    void endDemo(const LiveActor *pActor, const char *pDemoName);
}  // namespace MR
