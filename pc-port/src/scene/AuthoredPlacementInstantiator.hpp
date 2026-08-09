#pragma once

#include "scene/StageAuthoredData.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::scene {

    class NameObjLifecycleService;

    enum class AuthoredPlacementMode {
        Strict,
        // This mode exists only for bounded development scenes. Production
        // stage hosts must use Strict and reject every active blocked row.
        SupportedSubsetForDevelopment,
    };

    enum class AuthoredPlacementSupportKind {
        Ready,
        Ignored,
        Blocked,
    };

    enum class AuthoredPlacementOutcome {
        Pending,
        Created,
        InitializedAfterPlacement,
        Ignored,
        Blocked,
        Destroyed,
        Failed,
    };

    enum class AuthoredPlacementRuntimeState {
        Prepared,
        Instantiated,
        InitializedAfterPlacement,
        Cleared,
        Failed,
    };

    struct AuthoredPlacementSupport final {
        AuthoredPlacementSupportKind kind =
            AuthoredPlacementSupportKind::Blocked;
        std::string reason;
    };

    using AuthoredPlacementSupportResolver =
        std::function<AuthoredPlacementSupport(const StagePlacementObject &)>;
    using AuthoredPlacementActorNameResolver =
        std::function<std::optional<std::string>(const StagePlacementObject &)>;

    struct AuthoredPlacementInstantiationOptions final {
        AuthoredPlacementMode mode = AuthoredPlacementMode::Strict;
        AuthoredPlacementSupportResolver support_resolver{};
        // NameObj retains this pointer. The instantiator copies the returned
        // string into its stable report storage before constructing the actor.
        AuthoredPlacementActorNameResolver actor_name_resolver{};
    };

    struct AuthoredPlacementReportEntry final {
        std::size_t source_index = 0U;
        int retail_phase = 0;
        const StagePlacementObject *placement = nullptr;
        AuthoredPlacementSupport support{};
        std::optional<std::string> actor_name{};
        AuthoredPlacementOutcome outcome = AuthoredPlacementOutcome::Pending;
        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> archives{};
        NameObj *actor = nullptr;
        std::string failure_detail{};
    };

    struct AuthoredPlacementInstantiationReport final {
        AuthoredPlacementMode mode = AuthoredPlacementMode::Strict;
        AuthoredPlacementRuntimeState state =
            AuthoredPlacementRuntimeState::Prepared;
        bool preflight_accepted = false;
        std::size_t ready_count = 0U;
        std::size_t ignored_count = 0U;
        std::size_t blocked_count = 0U;
        std::size_t created_count = 0U;
        std::size_t initialized_after_placement_count = 0U;
        std::size_t destroyed_count = 0U;
        std::vector<AuthoredPlacementReportEntry> entries{};
    };

    struct AuthoredPlacementInstance final {
        const StagePlacementObject *placement = nullptr;
        NameObj *actor = nullptr;
    };

    // Injection boundary used by synthetic tests and by the ordinary
    // NameObjLifecycleService adapter. It deliberately contains no stage or
    // object-name policy.
    class AuthoredPlacementLifecycle {
    public:
        virtual ~AuthoredPlacementLifecycle() = default;

        virtual std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
        preload_archives(std::string_view object_name,
                         const NameObjPlacementContext &placement) = 0;
        [[nodiscard]] virtual std::unique_ptr<NameObj> construct_and_init(
            std::string_view object_name, const char *actor_name,
            const NameObjPlacementContext &placement) = 0;
        virtual void init_after_placement(NameObj &object) = 0;
        virtual void destroy(NameObj &object) = 0;
    };

    [[nodiscard]] int authored_placement_retail_phase(
        const StagePlacementObject &placement) noexcept;
    [[nodiscard]] AuthoredPlacementSupport classify_authored_placement(
        const StagePlacementObject &placement);

    // Creates every accepted active placement through one shared archive and
    // lifecycle path. StageAuthoredData and the lifecycle must outlive this
    // object. Call init_after_placement only after SceneObj and collision owners
    // have completed their pre-pass.
    class AuthoredPlacementInstantiator final {
    public:
        AuthoredPlacementInstantiator(
            const StageAuthoredData &data, AuthoredPlacementLifecycle &lifecycle,
            AuthoredPlacementInstantiationOptions options = {});
        AuthoredPlacementInstantiator(
            const StageAuthoredData &data, NameObjLifecycleService &lifecycle,
            AuthoredPlacementInstantiationOptions options = {});
        ~AuthoredPlacementInstantiator();

        AuthoredPlacementInstantiator(
            const AuthoredPlacementInstantiator &) = delete;
        AuthoredPlacementInstantiator &operator=(
            const AuthoredPlacementInstantiator &) = delete;
        AuthoredPlacementInstantiator(AuthoredPlacementInstantiator &&) = delete;
        AuthoredPlacementInstantiator &operator=(
            AuthoredPlacementInstantiator &&) = delete;

        const AuthoredPlacementInstantiationReport &instantiate();
        const AuthoredPlacementInstantiationReport &init_after_placement();
        void clear();

        [[nodiscard]] const AuthoredPlacementInstantiationReport &report() const
            noexcept;
        [[nodiscard]] std::span<const AuthoredPlacementInstance> instances() const
            noexcept;

    private:
        struct OwnedInstance final {
            std::size_t report_index = 0U;
            std::unique_ptr<NameObj> actor{};
        };

        void build_report();
        void preflight_or_throw();
        void clear_impl(bool propagate_errors);

        const StageAuthoredData &_data;
        std::unique_ptr<AuthoredPlacementLifecycle> _owned_lifecycle{};
        AuthoredPlacementLifecycle *_lifecycle = nullptr;
        AuthoredPlacementInstantiationOptions _options{};
        AuthoredPlacementInstantiationReport _report{};
        std::vector<OwnedInstance> _owned_instances{};
        std::vector<AuthoredPlacementInstance> _instance_views{};
    };

}  // namespace smgpc::scene
