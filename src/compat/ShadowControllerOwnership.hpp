#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class LiveActor;
class ShadowController;
class ShadowControllerList;
class ShadowControllerHolder;

namespace smgpc::compat {
    class JkrAllocationDomain;
    struct ActorShadowRuntimeState;
    struct ActorShadowControllerRuntimeState;

    // Owns the original controller graph. Parsed shadow definitions remain
    // resource metadata; runtime projection and flags live in these objects.
    class ShadowControllerOwnership final {
    public:
        ShadowControllerOwnership(LiveActor& actor, const ActorShadowRuntimeState& definitions);
        ~ShadowControllerOwnership();
        ShadowControllerOwnership(const ShadowControllerOwnership&) = delete;
        ShadowControllerOwnership& operator=(const ShadowControllerOwnership&) = delete;

        void publish();
        void add(const ActorShadowControllerRuntimeState& definition);
        void invalidate_joint_matrices() noexcept;

    private:
        struct Entry;
        void remove_from_holder(ShadowController*) noexcept;
        std::shared_ptr<JkrAllocationDomain> _domain;
        LiveActor* _actor;
        ShadowControllerHolder* _holder;
        std::uint64_t _holder_generation;
        std::unique_ptr<ShadowControllerList> _list;
        std::vector<std::unique_ptr<Entry>> _entries;
    };
}
