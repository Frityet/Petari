#include "compat/GlobalGravityOwnership.hpp"

#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Gravity/GraviryFollower.hpp"
#include "Game/Gravity/GravityCreator.hpp"
#include "Game/Map/BezierRail.hpp"
#include "Game/Map/RailPart.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace smgpc::compat {
    namespace {

        constexpr auto cMaxOwnedGravities = std::size_t{128U};

        enum class GravityCreatorKind {
            Cube,
            Cone,
            Disk,
            DiskTorus,
            Plane,
            PlaneInBox,
            PlaneInCylinder,
            Point,
            Segment,
            Wire,
            Unknown,
        };

        GlobalGravityOwnershipTotals sTotals{};

        [[nodiscard]] GravityCreatorKind creator_kind(const GravityCreator *creator) noexcept {
            if (dynamic_cast<const CubeGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Cube;
            }
            if (dynamic_cast<const ConeGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Cone;
            }
            if (dynamic_cast<const DiskGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Disk;
            }
            if (dynamic_cast<const DiskTorusGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::DiskTorus;
            }
            if (dynamic_cast<const PlaneGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Plane;
            }
            if (dynamic_cast<const PlaneInBoxGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::PlaneInBox;
            }
            if (dynamic_cast<const PlaneInCylinderGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::PlaneInCylinder;
            }
            if (dynamic_cast<const PointGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Point;
            }
            if (dynamic_cast<const SegmentGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Segment;
            }
            if (dynamic_cast<const WireGravityCreator *>(creator) != nullptr) {
                return GravityCreatorKind::Wire;
            }
            return GravityCreatorKind::Unknown;
        }

        void destroy_wire_rail_graph(RailRider *rail_rider,
                                     GlobalGravityOwnershipTotals &totals) noexcept {
            if (rail_rider == nullptr) {
                return;
            }

            auto *bezier = rail_rider->mBezierRail;
            if (bezier != nullptr) {
                if (bezier->mRailParts != nullptr && bezier->mNumRailParts > 0) {
                    for (auto index = bezier->mNumRailParts; index > 0; --index) {
                        auto &part = bezier->mRailParts[index - 1];
                        delete part.mRailPartLinear;
                        part.mRailPartLinear = nullptr;
                        delete part.mRailPartBezier;
                        part.mRailPartBezier = nullptr;
                        ++totals.reclaimed_wire_rail_parts;
                    }
                }
                delete[] bezier->mRailParts;
                bezier->mRailParts = nullptr;
                delete[] bezier->mPointCoords;
                bezier->mPointCoords = nullptr;
                delete bezier->mIter;
                bezier->mIter = nullptr;
                delete bezier;
                ++totals.reclaimed_wire_bezier_rails;
            }
            rail_rider->mBezierRail = nullptr;
            delete rail_rider;
            ++totals.reclaimed_wire_rail_riders;
        }

        template <typename Creator, typename Field>
        void destroy_creator_and_field(GravityCreator *base_creator,
                                       Field *Creator::*field_member,
                                       GlobalGravityOwnershipTotals &totals) noexcept {
            auto *creator = static_cast<Creator *>(base_creator);
            auto *field = creator->*field_member;
            creator->*field_member = nullptr;
            if (field != nullptr) {
                delete field;
                ++totals.reclaimed_fields;
            }
            delete creator;
            ++totals.reclaimed_creators;
        }

        bool destroy_creator_tree(GravityCreatorKind kind, GravityCreator *creator,
                                  GlobalGravityOwnershipTotals &totals) noexcept {
            if (creator == nullptr) {
                return true;
            }

            switch (kind) {
            case GravityCreatorKind::Cube:
                destroy_creator_and_field<CubeGravityCreator>(
                    creator, &CubeGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::Cone:
                destroy_creator_and_field<ConeGravityCreator>(
                    creator, &ConeGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::Disk:
                destroy_creator_and_field<DiskGravityCreator>(
                    creator, &DiskGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::DiskTorus:
                destroy_creator_and_field<DiskTorusGravityCreator>(
                    creator, &DiskTorusGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::Plane:
                destroy_creator_and_field<PlaneGravityCreator>(
                    creator, &PlaneGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::PlaneInBox:
                destroy_creator_and_field<PlaneInBoxGravityCreator>(
                    creator, &PlaneInBoxGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::PlaneInCylinder:
                destroy_creator_and_field<PlaneInCylinderGravityCreator>(
                    creator, &PlaneInCylinderGravityCreator::mGravityInstance,
                    totals);
                return true;
            case GravityCreatorKind::Point:
                destroy_creator_and_field<PointGravityCreator>(
                    creator, &PointGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::Segment:
                destroy_creator_and_field<SegmentGravityCreator>(
                    creator, &SegmentGravityCreator::mGravityInstance, totals);
                return true;
            case GravityCreatorKind::Wire: {
                auto *wire_creator = static_cast<WireGravityCreator *>(creator);
                destroy_wire_rail_graph(wire_creator->mRailRider, totals);
                wire_creator->mRailRider = nullptr;
                destroy_creator_and_field<WireGravityCreator>(
                    creator, &WireGravityCreator::mGravityInstance, totals);
                return true;
            }
            case GravityCreatorKind::Unknown:
                return false;
            }
            return false;
        }

        [[nodiscard]] BaseMatrixFollowTargetHolder *follow_holder(
            SceneObjHolder &holder) noexcept {
            return static_cast<BaseMatrixFollowTargetHolder *>(
                holder.getObj(SceneObj_BaseMatrixFollowTargetHolder));
        }

        template <typename T>
        void append_unique(std::vector<T *> &items, T *item) noexcept {
            if (item == nullptr || std::ranges::find(items, item) != items.end()) {
                return;
            }
            // Every vector reserves the retail manager's exact 128-entry
            // ceiling during owner construction. Capture therefore performs
            // no allocation while an actor init is unwinding.
            if (items.size() < items.capacity()) {
                items.push_back(item);
            }
        }

    }  // namespace

    class GlobalGravityOwnership::Impl final {
    public:
        struct Record final {
            GlobalGravityObj *actor = nullptr;
            GravityCreator *creator = nullptr;
            GravityCreatorKind kind = GravityCreatorKind::Unknown;
            BaseMatrixFollowTargetHolder *starting_holder = nullptr;
            std::size_t starting_follower_count = 0U;
            std::size_t starting_target_count = 0U;
            bool capture_open = true;
        };

        explicit Impl(SceneObjHolder &holder) : holder(&holder) {
            records.reserve(cMaxOwnedGravities);
            followers.reserve(cMaxOwnedGravities);
            link_infos.reserve(cMaxOwnedGravities);
            follow_targets.reserve(cMaxOwnedGravities);
        }

        [[nodiscard]] Record *find(GlobalGravityObj &actor) noexcept {
            const auto found = std::ranges::find_if(records, [&](const auto &record) {
                return record.actor == &actor &&
                       record.creator == actor.mGravityCreator;
            });
            return found != records.end() ? &*found : nullptr;
        }

        [[nodiscard]] const Record *find(const GlobalGravityObj &actor) const noexcept {
            const auto found = std::ranges::find_if(records, [&](const auto &record) {
                return record.actor == &actor &&
                       record.creator == actor.mGravityCreator;
            });
            return found != records.end() ? &*found : nullptr;
        }

        void set_suffix_start(Record &record) noexcept {
            record.starting_holder = follow_holder(*holder);
            record.starting_follower_count =
                record.starting_holder != nullptr ?
                    static_cast<std::size_t>(record.starting_holder->mFollowers.size()) :
                    0U;
            record.starting_target_count =
                record.starting_holder != nullptr ?
                    static_cast<std::size_t>(record.starting_holder->mTargets.size()) :
                    0U;
        }

        SceneObjHolder *holder;
        std::vector<Record> records;
        std::vector<GraviryFollower *> followers;
        std::vector<JMapLinkInfo *> link_infos;
        std::vector<BaseMatrixFollowTarget *> follow_targets;
        bool reclaimed = false;
    };

    GlobalGravityOwnership::GlobalGravityOwnership(SceneObjHolder &holder)
        : _impl(std::make_unique<Impl>(holder)) {
    }

    GlobalGravityOwnership::~GlobalGravityOwnership() {
        reclaim();
    }

    void GlobalGravityOwnership::adopt(GlobalGravityObj &actor) {
        if (_impl->reclaimed) {
            throw std::logic_error(
                "GlobalGravity ownership cannot adopt after scene reclamation.");
        }
        if (actor.mGravityCreator == nullptr) {
            throw std::invalid_argument(
                "GlobalGravity ownership requires the retail GravityCreator.");
        }
        if (creator_kind(actor.mGravityCreator) == GravityCreatorKind::Unknown) {
            throw std::invalid_argument(
                "GlobalGravity ownership rejects an unknown GravityCreator type.");
        }
        if (_impl->find(actor) != nullptr ||
            std::ranges::any_of(_impl->records, [&](const auto &record) {
                return record.creator == actor.mGravityCreator;
            })) {
            throw std::logic_error(
                "GlobalGravity ownership rejects duplicate creator adoption.");
        }
        if (std::ranges::any_of(_impl->records, [](const auto &record) {
                return record.capture_open;
            })) {
            throw std::logic_error(
                "GlobalGravity ownership rejects overlapping uninitialized actors.");
        }
        if (_impl->records.size() == cMaxOwnedGravities) {
            throw std::length_error(
                "GlobalGravity ownership exceeded the retail manager capacity.");
        }

        auto record = Impl::Record{
            .actor = &actor,
            .creator = actor.mGravityCreator,
            .kind = creator_kind(actor.mGravityCreator),
        };
        _impl->set_suffix_start(record);
        _impl->records.push_back(record);
        ++sTotals.adopted_creators;
    }

    bool GlobalGravityOwnership::owns(const GlobalGravityObj &actor) const noexcept {
        return _impl->find(actor) != nullptr;
    }

    void GlobalGravityOwnership::prepare_init(GlobalGravityObj &actor) {
        auto *record = _impl->find(actor);
        if (record == nullptr) {
            throw std::logic_error(
                "GlobalGravity init does not belong to the active scene owner.");
        }
        if (!record->capture_open) {
            throw std::logic_error(
                "GlobalGravity init cannot reopen a captured ownership record.");
        }
        _impl->set_suffix_start(*record);
    }

    void GlobalGravityOwnership::capture_after_init(
        GlobalGravityObj &actor, bool allow_missing_target) {
        auto *record = _impl->find(actor);
        if (record == nullptr) {
            throw std::logic_error(
                "GlobalGravity follower capture does not belong to the active scene owner.");
        }
        if (!record->capture_open) {
            return;
        }
        // Closing first makes every later destroy/capture idempotent even if
        // a suffix invariant reports a partially initialized retail object.
        record->capture_open = false;

        auto *current_holder = follow_holder(*_impl->holder);
        if (record->starting_holder != nullptr &&
            current_holder != record->starting_holder) {
            throw std::logic_error(
                "GlobalGravity follower holder changed during actor init.");
        }
        if (record->starting_holder == nullptr &&
            (record->starting_follower_count != 0U ||
             record->starting_target_count != 0U)) {
            throw std::logic_error(
                "GlobalGravity lazy follower holder has a nonzero baseline.");
        }
        if (current_holder == nullptr) {
            return;
        }

        const auto follower_count =
            static_cast<std::size_t>(current_holder->mFollowers.size());
        const auto target_count =
            static_cast<std::size_t>(current_holder->mTargets.size());
        if (follower_count < record->starting_follower_count ||
            target_count < record->starting_target_count) {
            throw std::logic_error(
                "GlobalGravity follower/target holder shrank during actor init.");
        }
        const auto follower_delta =
            follower_count - record->starting_follower_count;
        const auto target_delta = target_count - record->starting_target_count;
        if (follower_delta > 1U || target_delta > 1U ||
            (follower_delta == 0U && target_delta != 0U)) {
            throw std::logic_error(
                "GlobalGravity actor init appended an unexpected follower/target suffix.");
        }

        auto *field = record->creator->getGravity();
        for (auto index = record->starting_follower_count;
             index < follower_count; ++index) {
            // These entries were appended synchronously by the still-live
            // actor init. Older entries are deliberately never inspected.
            auto *follower = dynamic_cast<GraviryFollower *>(
                current_holder->mFollowers[static_cast<int>(index)]);
            if (follower == nullptr || follower->mFollowerObj != &actor ||
                follower->mGravity != field || follower->mLinkInfo == nullptr ||
                (!allow_missing_target && follower->mFollowTarget == nullptr)) {
                throw std::logic_error(
                    "GlobalGravity actor init appended foreign follower state.");
            }
            append_unique(_impl->followers, follower);
            append_unique(_impl->link_infos, follower->mLinkInfo);
            if (target_delta == 1U) {
                auto *owned_target = current_holder->mTargets[static_cast<int>(record->starting_target_count)];
                if (owned_target == nullptr ||
                    owned_target != follower->mFollowTarget) {
                    throw std::logic_error(
                        "GlobalGravity actor init appended foreign target state.");
                }
                append_unique(_impl->follow_targets, owned_target);
            }
            // Delta zero means either target allocation was the original init
            // failure, or this follower borrowed a target from the untouched
            // prefix. Neither target is owned by this gravity record.
        }
    }

    void GlobalGravityOwnership::reclaim() noexcept {
        if (_impl == nullptr || _impl->reclaimed) {
            return;
        }
        _impl->reclaimed = true;

        for (auto iter = _impl->followers.rbegin();
             iter != _impl->followers.rend(); ++iter) {
            delete *iter;
            ++sTotals.reclaimed_followers;
        }
        _impl->followers.clear();

        // A holder-created target aliases one follower-owned JMapLinkInfo and
        // can be shared by several followers. Retire each target exactly once
        // before any of those link records.
        for (auto iter = _impl->follow_targets.rbegin();
             iter != _impl->follow_targets.rend(); ++iter) {
            delete *iter;
            ++sTotals.reclaimed_follow_targets;
        }
        _impl->follow_targets.clear();

        for (auto iter = _impl->link_infos.rbegin();
             iter != _impl->link_infos.rend(); ++iter) {
            delete *iter;
            ++sTotals.reclaimed_link_infos;
        }
        _impl->link_infos.clear();

        for (auto iter = _impl->records.rbegin(); iter != _impl->records.rend();
             ++iter) {
            (void)destroy_creator_tree(iter->kind, iter->creator, sTotals);
        }
        _impl->records.clear();
    }

    void adopt_global_gravity_children(GlobalGravityObj &actor) {
        auto *owner = smgpc::scene::current_global_gravity_ownership();
        if (owner == nullptr) {
            ++sTotals.rejected_adoptions;
            const auto kind = creator_kind(actor.mGravityCreator);
            if (destroy_creator_tree(kind, actor.mGravityCreator, sTotals)) {
                actor.mGravityCreator = nullptr;
                ++sTotals.transactional_creator_reclaims;
            }
            throw std::logic_error(
                "GlobalGravity creation requires the active SceneObjHolder owner.");
        }

        try {
            owner->adopt(actor);
        } catch (...) {
            ++sTotals.rejected_adoptions;
            if (!owner->owns(actor)) {
                const auto kind = creator_kind(actor.mGravityCreator);
                if (destroy_creator_tree(kind, actor.mGravityCreator, sTotals)) {
                    actor.mGravityCreator = nullptr;
                    ++sTotals.transactional_creator_reclaims;
                }
            }
            throw;
        }
    }

    void prepare_global_gravity_init(NameObj &object) {
        auto *actor = dynamic_cast<GlobalGravityObj *>(&object);
        if (actor == nullptr) {
            return;
        }
        auto *owner = smgpc::scene::current_global_gravity_ownership();
        if (owner == nullptr) {
            throw std::logic_error(
                "GlobalGravity init requires its active scene owner.");
        }
        owner->prepare_init(*actor);
    }

    void capture_global_gravity_children(NameObj &object) {
        auto *actor = dynamic_cast<GlobalGravityObj *>(&object);
        if (actor == nullptr) {
            return;
        }
        auto *owner = smgpc::scene::current_global_gravity_ownership();
        if (owner == nullptr) {
            throw std::logic_error(
                "GlobalGravity follower capture requires its active scene owner.");
        }
        owner->capture_after_init(*actor, false);
    }

    void capture_failed_global_gravity_children(NameObj &object) noexcept {
        auto *actor = dynamic_cast<GlobalGravityObj *>(&object);
        if (actor == nullptr) {
            return;
        }
        auto *owner = smgpc::scene::current_global_gravity_ownership();
        if (owner == nullptr) {
            return;
        }
        try {
            owner->capture_after_init(*actor, true);
        } catch (...) {
            // The original retail init failure remains authoritative. The
            // creator and any field still remain owned by scene teardown.
        }
    }

    GlobalGravityOwnershipTotals global_gravity_ownership_totals() noexcept {
        return sTotals;
    }

}  // namespace smgpc::compat
