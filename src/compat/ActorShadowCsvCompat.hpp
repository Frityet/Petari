#pragma once

#include <string_view>

class LiveActor;

namespace smgpc::resource {
    class RarcArchive;
}

namespace smgpc::compat {
    // The archive overload is the deterministic provider boundary used by
    // focused tests and by callers that already own an exact object archive.
    // The model overload resolves the actor's mounted model archive.
    void initialize_actor_shadow_from_archive(LiveActor* actor, const smgpc::resource::RarcArchive& archive, std::string_view definition_name);
    void initialize_actor_shadow_from_model_archive(LiveActor* actor, std::string_view definition_name);
}  // namespace smgpc::compat
