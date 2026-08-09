#pragma once

class LiveActor;

namespace MR {
    void emitEffect(LiveActor*, const char*);
    void deleteEffect(LiveActor*, const char*);
    void deleteEffectAll(LiveActor*);
    void forceDeleteEffectAll(LiveActor*);
}  // namespace MR
