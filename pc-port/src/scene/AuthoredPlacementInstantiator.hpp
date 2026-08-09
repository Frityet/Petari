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
        Preloaded,
        Instantiated,
        InitializedAfterPlacement,
        Cleared,
        Failed,
    };

    // StageDataHolder::initPlacement walks these five PlacementInfoOrdered
    // holders after the Mario actor. Their declaration order is the retail
    // construction order, not the member-offset order in StageDataHolder.
    enum class AuthoredPlacementRetailPass {
        CommonHighPriority,
        ScenarioHighPriority,
        CommonNormal,
        ScenarioNormal,
        DeferredCommonNormal,
    };

    // PlacementInfoOrdered::sort places player archive loaders first, groups
    // whose archives are already available next, and groups that still read
    // from DVD last.
    enum class AuthoredPlacementGroupLoadOrder {
        PlayerArchiveLoader,
        ArchivesReady,
        ArchiveLoadRequired,
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
    using AuthoredPlacementDeferredArchiveResolver =
        std::function<bool(const StagePlacementObject &)>;
    using AuthoredPlacementGroupLoadOrderResolver =
        std::function<AuthoredPlacementGroupLoadOrder(
            const StagePlacementObject &)>;
    using AuthoredPlacementCreatorAvailabilityResolver =
        std::function<bool(const StagePlacementObject &)>;

    struct AuthoredPlacementInstantiationOptions final {
        AuthoredPlacementMode mode = AuthoredPlacementMode::Strict;
        AuthoredPlacementSupportResolver support_resolver{};
        // NameObj retains this pointer. The instantiator copies the returned
        // string into its stable report storage before constructing the actor.
        AuthoredPlacementActorNameResolver actor_name_resolver{};
        // Synthetic and alternate data providers can supply the retail
        // PlanetMapCreatorFunction decision without installing a scene-global
        // catalog. Production defaults to the active PlanetMapCatalog.
        AuthoredPlacementDeferredArchiveResolver
            deferred_archive_resolver{};
        // Defaults to the live NameObjFactory archive state used by retail's
        // PlacementInfoOrdered::sort.
        AuthoredPlacementGroupLoadOrderResolver
            group_load_order_resolver{};
        // Retail requestFileLoad checks the group's creator independently of
        // per-row support. Synthetic lifecycle providers must opt in here;
        // production defaults to the shared NameObjFactory creator lookup.
        AuthoredPlacementCreatorAvailabilityResolver
            creator_availability_resolver{};
    };

    struct AuthoredPlacementReportEntry final {
        std::size_t source_index = 0U;
        AuthoredPlacementRetailPass retail_pass =
            AuthoredPlacementRetailPass::CommonHighPriority;
        std::size_t retail_group_index = 0U;
        AuthoredPlacementGroupLoadOrder retail_group_load_order =
            AuthoredPlacementGroupLoadOrder::ArchivesReady;
        const StagePlacementObject *placement = nullptr;
        AuthoredPlacementSupport support{};
        std::optional<std::string> actor_name{};
        AuthoredPlacementOutcome outcome = AuthoredPlacementOutcome::Pending;
        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> archives{};
        NameObj *actor = nullptr;
        std::string failure_detail{};
    };

    // Retail's scene-wide initAfterPlacement pass includes raw NameObjs that
    // a placement root constructs from its constructor or init method. Keep
    // those identities visible without counting them as additional authored
    // rows. construction_ordinal is the zero-based registration position in
    // the complete placement construction suffix; the returned root normally
    // occupies ordinal zero.
    struct AuthoredPlacementDescendantReportEntry final {
        std::size_t parent_source_index = 0U;
        std::size_t parent_report_index = 0U;
        std::size_t construction_ordinal = 0U;
        const StagePlacementObject *parent_placement = nullptr;
        NameObj *object = nullptr;
        bool owned_by_placement = false;
        AuthoredPlacementOutcome outcome = AuthoredPlacementOutcome::Pending;
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
        std::vector<AuthoredPlacementDescendantReportEntry> descendants{};
    };

    struct AuthoredPlacementInstance final {
        const StagePlacementObject *placement = nullptr;
        NameObj *actor = nullptr;
    };

    // Injection boundary used by synthetic tests and by the ordinary
    // NameObjLifecycleService adapter. It deliberately contains no stage or
    // object-name policy.
    class AuthoredPlacementConstructionScope {
    public:
        virtual ~AuthoredPlacementConstructionScope() = default;
    };

    class AuthoredPlacementLifecycle {
    public:
        virtual ~AuthoredPlacementLifecycle() = default;

        // Production placement adapters can retain strict scene-local state
        // across construct+init. Synthetic lifecycles need no SceneObjHolder
        // and inherit this null RAII token.
        virtual std::unique_ptr<AuthoredPlacementConstructionScope>
        begin_construction_scope(
            const NameObjPlacementContext &) {
            return {};
        }

        virtual std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
        preload_archives(std::string_view object_name,
                         const NameObjPlacementContext &placement) = 0;
        virtual std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
        preload_model_changing_archive(
            std::string_view object_name, s32 shape_model_no,
            const NameObjPlacementContext &placement) = 0;
        [[nodiscard]] virtual std::unique_ptr<NameObj> construct(
            std::string_view object_name, const char *actor_name,
            const NameObjPlacementContext &placement) = 0;
        [[nodiscard]] virtual std::unique_ptr<NameObj>
        construct_model_changing(std::string_view object_name,
                                 s32 shape_model_no,
                                 const char *actor_name,
                                 const NameObjPlacementContext &placement) = 0;
        virtual void init(NameObj &object,
                          const NameObjPlacementContext &placement) = 0;
        virtual void init_after_placement(NameObj &object) = 0;
        virtual void destroy(NameObj &object) = 0;
    };

    [[nodiscard]] std::string_view authored_placement_identifier(
        const StagePlacementObject &placement) noexcept;
    [[nodiscard]] AuthoredPlacementRetailPass authored_placement_retail_pass(
        const StagePlacementObject &placement);
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

        // Runs the complete support audit without touching the lifecycle. This
        // lets production scenes reject a Strict stage before constructing a
        // StartInfo actor while still preserving retail's start-before-object
        // construction order once the audit is green.
        const AuthoredPlacementInstantiationReport &preflight();
        // Resolves the exact five-holder order and completes every archive
        // request without constructing an actor. Retail inserts Mario between
        // this phase and instantiate().
        const AuthoredPlacementInstantiationReport &preload();
        // Constructs the already-preloaded plan. This never ranks or requests
        // archives and therefore requires a successful preload() first.
        const AuthoredPlacementInstantiationReport &instantiate();
        const AuthoredPlacementInstantiationReport &init_after_placement();
        void clear();

        [[nodiscard]] const AuthoredPlacementInstantiationReport &report() const
            noexcept;
        [[nodiscard]] std::span<const AuthoredPlacementInstance> instances() const
            noexcept;
        [[nodiscard]] std::span<
            const AuthoredPlacementDescendantReportEntry>
        descendants() const noexcept;

    private:
        struct OwnedObject final {
            NameObj *object = nullptr;
            std::unique_ptr<NameObj> ownership{};
            std::optional<std::size_t> descendant_report_index{};
            bool delegated_postpass = false;
        };

        struct OwnedInstance final {
            std::size_t report_index = 0U;
            NameObj *actor = nullptr;
            std::vector<OwnedObject> objects{};
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
