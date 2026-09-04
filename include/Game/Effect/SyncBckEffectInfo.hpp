#pragma once

#include "Game/Util/Array.hpp"

class J3DAnmTransform;
class XanimePlayer;

class SyncBckEffectInfo {
public:
    class BckResourceInfo {
    public:
        BckResourceInfo(const XanimePlayer*, const char*);

        bool isLoop() const;

        /* 0x00 */ const char* mName;
        /* 0x04 */ J3DAnmTransform* mResource;
    };

    SyncBckEffectInfo(const XanimePlayer*, const char*, s32, f32, f32, bool);

    void addBck(const XanimePlayer*, const char*);
    bool isRegisteredBck(const char*) const;
    bool isBckLoop(const char*) const;

    /* 0x00 */ MR::Vector< MR::AssignableArray< BckResourceInfo* > > mBckResources;
    /* 0x0C */ f32 mStartFrame;
    /* 0x10 */ f32 mEndFrame;
    /* 0x14 */ bool mContinueBckEnd;
};

namespace MR {
    namespace Effect {
        bool isExistSyncBckDeleteFrame(const SyncBckEffectInfo*);
    }
}
