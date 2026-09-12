#pragma once

class LiveActor;
class ClippingJudge;

namespace smgpc::compat {
    void update_actor_clipping(LiveActor& actor, const ClippingJudge& judge);
}  // namespace smgpc::compat
