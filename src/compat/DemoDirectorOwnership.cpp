#include "compat/DemoDirectorOwnership.hpp"

#include "Game/Demo/DemoActionKeeper.hpp"
#include "Game/Demo/DemoCameraKeeper.hpp"
#include "Game/Demo/DemoCastGroup.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoExecutor.hpp"
#include "Game/Demo/DemoPlayerKeeper.hpp"
#include "Game/Demo/DemoSimpleCastHolder.hpp"
#include "Game/Demo/DemoSoundKeeper.hpp"
#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/Demo/DemoSubPartKeeper.hpp"
#include "Game/Demo/DemoTalkAnimCtrl.hpp"
#include "Game/Demo/DemoTimeKeeper.hpp"
#include "Game/Demo/DemoWipeKeeper.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <exception>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace smgpc::compat {
    namespace {
        std::shared_ptr<JkrAllocationDomain> capture_domain() {
            auto domain = current_jkr_allocation_domain();
            if (!domain) aurora::throw_host_exception<std::logic_error>(
                "Original Demo ownership must be captured inside its Game allocation scope");
            return domain;
        }

        template<class Vector, class Predicate>
        void erase_borrowed(Vector& values, Predicate remove) noexcept {
            auto* end = values.end();
            auto* retained = std::remove_if(values.begin(), end, remove);
            std::fill(retained, end, typename Vector::Item{});
            values.mCount = static_cast<s32>(retained - values.begin());
        }

        void destroy_talk(DemoTalkAnimCtrl* controller) noexcept {
            if (!controller) return;
            delete controller->mCameraInfo;
            delete controller; // The actual NerveExecutor destructor owns Spine.
        }

        void destroy_actions(DemoActionKeeper* keeper) noexcept {
            if (!keeper) return;
            for (s32 i = 0; i < keeper->mNumInfos; ++i) {
                auto* info = keeper->mInfoArray[i];
                if (!info) continue;
                for (s32 cast = 0; cast < info->_18; ++cast) delete info->mFunctors[cast];
                delete[] info->mNerves;
                delete[] info->mFunctors;
                delete[] info->mCastList;
                delete info;
            }
            delete[] keeper->mInfoArray;
            delete keeper;
        }

        void destroy_cameras(DemoCameraKeeper* keeper) noexcept {
            if (!keeper) return;
            for (s32 i = 0; i < keeper->_4; ++i) {
                delete[] keeper->_8[i]._1C; // Generated executor[part] name.
                delete keeper->_8[i]._20;
            }
            delete[] keeper->_8;
            delete keeper;
        }

        void destroy_switch(StageSwitchCtrl* controller) noexcept {
            if (!controller) return;
            for (auto* info : {controller->mSW_A, controller->mSW_B,
                               controller->mSW_Appear, controller->mSW_Dead}) {
                if (!info) continue;
                delete info->mIDInfo;
                delete info;
            }
            delete controller;
        }

        bool start_info_borrows(const DemoStartInfo& info, const NameObj* object) noexcept {
            return info._0 == object || info._4 == object || info._C == object ||
                   info._10 == object || info._14 == object;
        }

        void release_requests(DemoStartRequestHolder* holder, const NameObj* object) noexcept {
            if (!holder) return;
            std::array<const DemoStartInfo*, 16> retained{};
            std::size_t count = 0;
            auto& queue = holder->mRequestBuffer;
            if (queue.mCount < 0 || queue.mCount > 16) std::terminate();
            auto cursor = queue.mHead;
            for (s32 i = 0; i < queue.mCount; ++i, ++cursor) {
                const auto* info = *cursor.mHead;
                if (info && !start_info_borrows(*info, object)) retained[count++] = info;
            }
            for (s32 i = 0; i < holder->mNumInfos; ++i) {
                if (start_info_borrows(*holder->mStartInfos[i], object))
                    *holder->mStartInfos[i] = DemoStartInfo{};
            }
            queue.mHead = decltype(queue.mHead)(queue.mBuffer, queue.mBuffer);
            queue.mEnd = queue.mHead;
            queue.mCount = 0;
            std::fill(std::begin(queue.mBuffer), std::end(queue.mBuffer), nullptr);
            for (std::size_t i = 0; i < count; ++i) queue.push_back(retained[i]);
        }
    }

    class DemoDirectorOwnership::Impl final {
    public:
        struct Identity {
            NameObj* object = nullptr;
            std::uint64_t generation = 0;
            std::shared_ptr<JkrAllocationDomain> domain;
            bool prepared = false;
            bool live() const noexcept {
                return object && name_obj_runtime_generation(object) == generation;
            }
        };
        struct Director : Identity {
            DemoSimpleCastHolder* simple = nullptr;
            DemoStartRequestHolder* requests = nullptr;
        } director;
        struct Group : Identity {
            DemoExecutor* executor = nullptr;
            JMapIdInfo* id = nullptr;
            DemoTimeKeeper* time = nullptr;
            DemoSubPartKeeper* subpart = nullptr;
            DemoPlayerKeeper* player = nullptr;
            DemoCameraKeeper* camera = nullptr;
            DemoActionKeeper* action = nullptr;
            DemoWipeKeeper* wipe = nullptr;
            DemoSoundKeeper* sound = nullptr;
            StageSwitchCtrl* switches = nullptr;
            std::array<DemoTalkAnimCtrl*, 8> talk{};
        };
        std::vector<Group> groups;
        bool reclaiming = false;

        static void identify(Identity& record, NameObj& object) {
            const auto generation = name_obj_runtime_generation(&object);
            if (!generation) aurora::throw_host_exception<std::logic_error>(
                "Original Demo ownership requires a registered NameObj");
            record.domain = capture_domain();
            record.object = &object;
            record.generation = generation;
        }
        Group& group(DemoCastGroup& object) {
            auto found = std::find_if(groups.begin(), groups.end(), [&](const auto& value) {
                return value.object == &object && value.generation == name_obj_runtime_generation(&object);
            });
            if (found != groups.end()) {
                if (found->prepared) aurora::throw_host_exception<std::logic_error>(
                    "Cannot recapture a Demo cast group during retirement");
                return *found;
            }
            Group record;
            identify(record, object);
            record.id = object.mInfo;
            groups.push_back(std::move(record));
            return groups.back();
        }
        static void snapshot(Group& record) noexcept {
            if (record.prepared) return;
            if (record.live()) {
                record.id = static_cast<DemoCastGroup*>(record.object)->mInfo;
                if (auto* executor = record.executor) {
                    record.time = executor->mTimeKeeper;
                    record.subpart = executor->mSubPartKeeper;
                    record.player = executor->mPlayerKeeper;
                    record.camera = executor->mCameraKeeper;
                    record.action = executor->mActionKeeper;
                    record.wipe = executor->mWipeKeeper;
                    record.sound = executor->mSoundKeeper;
                    record.switches = executor->_40;
                    const auto count = executor->mTalkAnimCtrl.size();
                    if (count < 0 || count > static_cast<s32>(record.talk.size())) std::terminate();
                    record.talk.fill(nullptr);
                    std::copy_n(executor->mTalkAnimCtrl.begin(), count, record.talk.begin());
                }
            }
            record.prepared = true;
        }
        static void retire(Group& record) noexcept {
            for (auto* talk : record.talk) destroy_talk(talk);
            destroy_actions(record.action);
            destroy_cameras(record.camera);
            if (record.time) { delete[] record.time->mMainPartInfos; delete record.time; }
            if (record.subpart) { delete[] record.subpart->mSubPartInfos; delete record.subpart; }
            if (record.player) { delete[] record.player->mPlayerInfos; delete record.player; }
            // These generated destructors own their AssignableArray storage.
            delete record.sound;
            delete record.wipe;
            destroy_switch(record.switches);
            delete record.id;
        }
    };

    DemoDirectorOwnership::DemoDirectorOwnership() {
        JkrHostAllocationScope host;
        _impl = std::make_unique<Impl>();
    }
    DemoDirectorOwnership::~DemoDirectorOwnership() {
        prepare_retirement();
        reclaim();
    }
    void DemoDirectorOwnership::capture(DemoDirector& director) {
        JkrHostAllocationScope host;
        auto& record = _impl->director;
        if (record.object) {
            if (record.object != &director || !record.live() || record.prepared)
                aurora::throw_host_exception<std::logic_error>("The scene already captured its original DemoDirector");
            return;
        }
        Impl::identify(record, director);
        record.simple = director._20;
        record.requests = director.mStartRequestHolder;
    }
    void DemoDirectorOwnership::capture_cast_group(DemoCastGroup& group) {
        JkrHostAllocationScope host;
        (void)_impl->group(group);
    }
    void DemoDirectorOwnership::capture_executor(DemoExecutor& executor) {
        JkrHostAllocationScope host;
        auto& record = _impl->group(executor);
        record.executor = &executor;
        // Capture the stable children now; the inline talk array is snapshotted
        // immediately before the original Executor's destructor is allowed to run.
        Impl::snapshot(record);
        record.prepared = false;
    }
    void DemoDirectorOwnership::prepare_retirement() noexcept {
        for (auto& record : _impl->groups) Impl::snapshot(record);
        if (_impl->director.object) _impl->director.prepared = true;
    }
    void DemoDirectorOwnership::prepare_rollback(NameObjRuntimeRegistrationMarker marker) noexcept {
        for (auto& record : _impl->groups)
            if (name_obj_runtime_object_was_registered_since(record.object, marker)) Impl::snapshot(record);
        auto& director = _impl->director;
        if (name_obj_runtime_object_was_registered_since(director.object, marker)) director.prepared = true;
    }
    void DemoDirectorOwnership::reclaim() noexcept {
        if (_impl->reclaiming) return;
        _impl->reclaiming = true;
        for (auto& record : _impl->groups) {
            if (!record.prepared) continue;
            JkrAllocationScope original(record.domain);
            Impl::retire(record);
        }
        {
            JkrHostAllocationScope host;
            std::erase_if(_impl->groups, [](const auto& record) { return record.prepared; });
        }
        auto& director = _impl->director;
        if (director.prepared) {
            JkrAllocationScope original(director.domain);
            if (director.requests) {
                for (s32 i = 0; i < director.requests->mNumInfos; ++i) delete director.requests->mStartInfos[i];
                // The request proxy is a registered, scene-owned NameObj.
                delete director.requests;
            }
            delete director.simple;
            director = {};
        }
        _impl->reclaiming = false;
    }
    void DemoDirectorOwnership::release_name_obj(const NameObj* object) noexcept {
        if (!object || _impl->reclaiming) return;
        auto& director = _impl->director;
        if (!director.prepared && director.simple) {
            const auto matches = [object](const auto* value) { return value == object; };
            erase_borrowed(director.simple->mLiveActors, matches);
            erase_borrowed(director.simple->mLayoutActors, matches);
            erase_borrowed(director.simple->mNameObjs, matches);
            release_requests(director.requests, object);
            if (director.live() && director.object != object) {
                auto* original = static_cast<DemoDirector*>(director.object);
                if (original->_2C == object) original->_2C = nullptr;
                if (original->mExecutor == object) original->mExecutor = nullptr;
            }
        }
        for (auto& record : _impl->groups) {
            if (record.prepared || !record.live() || record.object == object || !record.executor) continue;
            JkrAllocationScope original(record.domain);
            auto& executor = *record.executor;
            erase_borrowed(executor.mActor, [object](auto* actor) { return actor == object; });
            erase_borrowed(executor.mTalkMessageCtrl, [object](const auto& info) { return info.mActor == object; });
            erase_borrowed(executor.mTalkAnimCtrl, [object](auto* controller) {
                if (controller->mActor != object) return false;
                destroy_talk(controller);
                return true;
            });
            if (executor._44 == object) executor._44 = nullptr;
            if (auto* keeper = executor.mCameraKeeper)
                for (s32 i = 0; i < keeper->_4; ++i)
                    if (keeper->_8[i]._24 == object) keeper->_8[i]._24 = nullptr;
            if (auto* keeper = executor.mActionKeeper) {
                for (s32 i = 0; i < keeper->mNumInfos; ++i) {
                    auto& info = *keeper->mInfoArray[i];
                    s32 retained = 0;
                    for (s32 cast = 0; cast < info.mCastCount; ++cast) {
                        if (info.mCastList[cast] == object) { delete info.mFunctors[cast]; continue; }
                        info.mCastList[retained] = info.mCastList[cast];
                        info.mFunctors[retained] = info.mFunctors[cast];
                        info.mNerves[retained++] = info.mNerves[cast];
                    }
                    for (s32 cast = retained; cast < info.mCastCount; ++cast) {
                        info.mCastList[cast] = nullptr; info.mFunctors[cast] = nullptr; info.mNerves[cast] = nullptr;
                    }
                    info.mCastCount = retained;
                }
            }
        }
    }
    std::size_t DemoDirectorOwnership::simple_cast_registration_count(const NameObj* object) const noexcept {
        const auto& record = _impl->director;
        if (record.prepared || !record.simple) return 0;
        std::size_t count = 0;
        const auto add = [&](const auto& values) {
            for (const auto* value : values) if (!object || value == object) ++count;
        };
        add(record.simple->mLiveActors);
        add(record.simple->mLayoutActors);
        add(record.simple->mNameObjs);
        return count;
    }
} // namespace smgpc::compat
