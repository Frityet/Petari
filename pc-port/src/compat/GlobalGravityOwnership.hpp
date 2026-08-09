#pragma once

#include <cstddef>
#include <memory>

class GlobalGravityObj;
class NameObj;
class SceneObjHolder;

namespace smgpc::compat {

    struct GlobalGravityOwnershipTotals final {
        std::size_t adopted_creators = 0U;
        std::size_t rejected_adoptions = 0U;
        std::size_t transactional_creator_reclaims = 0U;
        std::size_t reclaimed_creators = 0U;
        std::size_t reclaimed_fields = 0U;
        std::size_t reclaimed_followers = 0U;
        std::size_t reclaimed_link_infos = 0U;
        std::size_t reclaimed_follow_targets = 0U;
        std::size_t reclaimed_wire_rail_riders = 0U;
        std::size_t reclaimed_wire_bezier_rails = 0U;
        std::size_t reclaimed_wire_rail_parts = 0U;
    };

    class GlobalGravityOwnership final {
    public:
        explicit GlobalGravityOwnership(SceneObjHolder &holder);
        ~GlobalGravityOwnership();

        GlobalGravityOwnership(const GlobalGravityOwnership &) = delete;
        GlobalGravityOwnership &operator=(const GlobalGravityOwnership &) = delete;

        void adopt(GlobalGravityObj &actor);
        [[nodiscard]] bool owns(const GlobalGravityObj &actor) const noexcept;
        void prepare_init(GlobalGravityObj &actor);
        void capture_after_init(GlobalGravityObj &actor,
                                bool allow_missing_target);
        void reclaim() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

    // NameObjFactory calls this immediately after the retail creator returns,
    // before GlobalGravityObj::init can allocate or register any children.
    void adopt_global_gravity_children(GlobalGravityObj &actor);

    // NameObjLifecycleService brackets the exact init call with these hooks.
    // Capture inspects only the synchronously appended follower-holder suffix;
    // it never dereferences older holder entries whose external owners may have
    // already retired.
    void prepare_global_gravity_init(NameObj &object);
    void capture_global_gravity_children(NameObj &object);
    void capture_failed_global_gravity_children(NameObj &object) noexcept;

    [[nodiscard]] GlobalGravityOwnershipTotals global_gravity_ownership_totals() noexcept;

}  // namespace smgpc::compat
