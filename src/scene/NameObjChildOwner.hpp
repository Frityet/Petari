#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace smgpc::scene {

    // Explicit ownership for scene compositions whose retail factories return
    // raw NameObj children. Children are retired in reverse construction order
    // so dependants can be adopted immediately after the object they follow.
    class NameObjChildOwner final {
    public:
        NameObjChildOwner() = default;
        ~NameObjChildOwner();

        NameObjChildOwner(const NameObjChildOwner &) = delete;
        NameObjChildOwner &operator=(const NameObjChildOwner &) = delete;
        NameObjChildOwner(NameObjChildOwner &&) = delete;
        NameObjChildOwner &operator=(NameObjChildOwner &&) = delete;

        template <typename Child>
            requires std::derived_from<Child, NameObj>
        Child &adopt(std::unique_ptr<Child> child) {
            auto *identity = child.get();
            validate_candidate(identity);
            smgpc::compat::claim_name_obj_runtime_ownership(
                identity, this);
            _children.emplace_back(std::move(child));
            return *identity;
        }

        template <typename Child>
            requires std::derived_from<Child, NameObj>
        Child &adopt(Child *child) {
            validate_candidate(child);
            return adopt(std::unique_ptr<Child>(child));
        }

        // Runs a legacy product constructor and adopts every still-live
        // NameObj it registered, in construction order. If construction
        // throws after allocating children, those children are adopted before
        // the original exception continues unwinding. Capture is exclusive to
        // the scene-construction thread: overlapping/nested capture is
        // rejected. The callable must not independently retain ownership of a
        // NameObj it constructs.
        template <typename Construction>
            requires std::invocable<Construction> &&
                     (!std::is_reference_v<std::invoke_result_t<Construction>>)
        std::invoke_result_t<Construction> capture_construction_children(
            Construction &&construction) {
            using Result = std::invoke_result_t<Construction>;
            const auto capture =
                smgpc::compat::NameObjRuntimeRegistrationCapture{};
            const auto marker = capture.marker();

            auto invoke_and_preserve_failure = [&]() -> Result {
                try {
                    return std::invoke(
                        std::forward<Construction>(construction));
                } catch (...) {
                    const auto construction_failure =
                        std::current_exception();
                    try {
                        adopt_registered_since(marker);
                    } catch (...) {
                        rollback_unowned_registered_since(marker);
                    }
                    std::rethrow_exception(construction_failure);
                }
            };

            auto adopt_or_rollback = [&] {
                try {
                    adopt_registered_since(marker);
                } catch (...) {
                    rollback_unowned_registered_since(marker);
                    throw;
                }
            };

            if constexpr (std::is_void_v<Result>) {
                invoke_and_preserve_failure();
                adopt_or_rollback();
            } else {
                auto result = invoke_and_preserve_failure();
                adopt_or_rollback();
                return result;
            }
        }

        void clear() noexcept;

        // Captures the complete registration suffix for an externally-owned
        // root. Unclaimed descendants are stored here, while SceneObj- or
        // service-owned identities remain non-owning ordered entries with
        // their one scene-wide postpass delegated to this graph.
        void adopt_root_registration_suffix(
            smgpc::compat::NameObjRuntimeRegistrationMarker marker,
            NameObj &root, const void *root_owner);
        void init_registration_suffix_after_placement();
        static void rollback_registration_suffix(
            smgpc::compat::NameObjRuntimeRegistrationMarker marker) noexcept;

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::size_t size() const noexcept;

    private:
        void validate_candidate(const NameObj *child) const;
        void adopt_registered_since(
            smgpc::compat::NameObjRuntimeRegistrationMarker marker);
        static void rollback_unowned_registered_since(
            smgpc::compat::NameObjRuntimeRegistrationMarker marker) noexcept;

        struct Registration final {
            NameObj *object = nullptr;
            bool delegated_postpass = false;
        };

        std::vector<std::unique_ptr<NameObj>> _children{};
        std::vector<Registration> _registrations{};
        std::size_t _next_registration_postpass_index = 0U;
    };

}  // namespace smgpc::scene
