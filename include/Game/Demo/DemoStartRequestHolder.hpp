#pragma once

#include "Game/Util/Array.hpp"

class DemoExecutor;
class LayoutActor;
class LiveActor;
class NameObj;
class Nerve;
class NerveExecutor;

class DemoStartInfo {
public:
    enum DemoType {};
    enum CinemaFrameType {};
    enum StarPointerType {};
    enum DeleteEffectType {};

    DemoStartInfo();
    DemoStartInfo& operator=(const DemoStartInfo&);

    LiveActor* _0;
    LayoutActor* _4;
    NerveExecutor* _8;
    NameObj* _C;
    NameObj* _10;
    DemoExecutor* _14;
    const char* mDemoName;  // 0x18
    const char* _1C;
    const Nerve* _20;
    u32 _24;
    u32 _28;
    u32 _2C;
    u32 _30;
    u32 _34;
};

class DemoStartRequestHolder {
public:
    DemoStartRequestHolder();

    void pushRequest(LiveActor*, const char*);
    void pushRequest(LayoutActor*, const char*);
    void pushRequest(NerveExecutor*, const char*);
    void pushRequest(NameObj*, const char*);
    void popRequest();
    bool isExistRequest() const;
    const DemoStartInfo* getCurrentInfo() const;
    void registerStartDemoInfo(const DemoStartInfo&);
    DemoStartInfo* find(const LiveActor*, const char*) const;
    DemoStartInfo* find(const LayoutActor*, const char*) const;
    DemoStartInfo* find(const NerveExecutor*, const char*) const;
    DemoStartInfo* find(const NameObj*, const char*) const;
    DemoStartInfo* findEmpty() const;

    DemoStartInfo* mStartInfos[0x10];                                // 0x0
    s32 mNumInfos;                                                   // 0x40
    MR::FixedRingBuffer< const DemoStartInfo*, 16 > mRequestBuffer;  // 0x44
    NameObj* mProxyObj;                                              // 0xA0
};

namespace MR {
    template <>
    FixedRingBuffer< const DemoStartInfo*, 16 >::iterator::iterator(const DemoStartInfo**, const DemoStartInfo**);

    template <>
    void FixedRingBuffer< const DemoStartInfo*, 16 >::push_back(const DemoStartInfo* const&);

    template <>
    void FixedRingBuffer< const DemoStartInfo*, 16 >::iterator::operator++();
};  // namespace MR
