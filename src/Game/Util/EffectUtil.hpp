#pragma once

class LiveActor;

namespace MR {
    void emitEffect(LiveActor*, const char*);
    void emitEffectHit(LiveActor*, const TVec3f&, const TVec3f&, const char*);
    void deleteEffect(LiveActor*, const char*);
    void deleteEffectAll(LiveActor*);
    void forceDeleteEffect(LiveActor*, const char*);
    void forceDeleteEffectAll(LiveActor*);
    bool isRegisteredEffect(const LiveActor*, const char*);
}  // namespace MR
