#include "compat/MarioAnimatorLifetime.hpp"

#include "Game/Animation/XanimeCore.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ModelManagerOwner.hpp"
#include "compat/ResourceHolderCompat.hpp"

#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        struct CapturedHashTable {
            HashSortTable* table = nullptr;
            u32* hashes = nullptr;
            u32* indices = nullptr;
            u16* bucket_starts = nullptr;
            u16* bucket_counts = nullptr;

            void capture(HashSortTable* value) noexcept {
                table = value;
                if (!table) return;
                hashes = table->mHashCodes;
                indices = table->_8;
                bucket_starts = table->_C;
                bucket_counts = table->_10;
            }

            void destroy() noexcept {
                delete table;
                delete[] hashes;
                delete[] indices;
                delete[] bucket_starts;
                delete[] bucket_counts;
            }
        };

        struct CapturedPlayer {
            XanimePlayer* player = nullptr;
            XanimeCore* core = nullptr;
            XanimeTrack* tracks = nullptr;
            XjointInfo* joints = nullptr;
            XjointTransform* transforms = nullptr;
            XanimeGroupInfo* simple_group = nullptr;

            void capture(XanimePlayer* value) noexcept {
                player = value;
                if (!player) return;
                core = player->mCore;
                simple_group = player->mSimpleGroup;
                if (!core) return;
                tracks = core->mTrackList;
                joints = core->mJointList;
                transforms = core->mTransformList;
            }
        };

        // Retained by the actual ModelManager owner, not by the mutable Game
        // pointers. Original upper/lower cores share joints and transforms;
        // each core has its own tracks. No player here owns/deletes the model.
        struct MarioAnimatorLifetime {
            std::shared_ptr<JkrAllocationDomain> domain;
            std::shared_ptr<const ResourceArchiveOwner> resources;
            MarioAnimator* animator = nullptr;
            XanimeResourceTable* resource_table = nullptr;
            XanimeGroupInfo* simple_groups = nullptr;
            CapturedHashTable resource_hash;
            CapturedHashTable callbacks;
            CapturedPlayer lower;
            CapturedPlayer upper;

            void capture(MarioAnimator& value, bool complete) noexcept {
                animator = complete ? &value : nullptr;
                resource_table = value.mResourceTable;
                if (resource_table) {
                    simple_groups = resource_table->mSimpleGroupInfos;
                    resource_hash.capture(resource_table->mSortTable);
                }
                callbacks.capture(value._120);
                lower.capture(value.mXanimePlayer);
                upper.capture(value.mXanimePlayerUpper);
            }

            ~MarioAnimatorLifetime() {
                JkrHostAllocationScope host;
                // Original getSimpleGroup allocates lazily after init. These
                // owned player identities remain valid until the deletes below.
                if (lower.player) lower.simple_group = lower.player->mSimpleGroup;
                if (upper.player) upper.simple_group = upper.player->mSimpleGroup;
                // Destruct the independent objects before retiring their shared
                // arrays. Every address is the identity captured at construction.
                if (upper.core != lower.core) delete upper.core;
                delete lower.core;
                if (upper.player != lower.player) delete upper.player;
                delete lower.player;
                if (upper.tracks != lower.tracks) delete[] upper.tracks;
                delete[] lower.tracks;
                if (upper.joints != lower.joints) delete[] upper.joints;
                delete[] lower.joints;
                if (upper.transforms != lower.transforms) delete[] upper.transforms;
                delete[] lower.transforms;
                if (upper.simple_group != lower.simple_group) delete upper.simple_group;
                delete lower.simple_group;
                if (callbacks.table != resource_hash.table) callbacks.destroy();
                resource_hash.destroy();
                delete[] simple_groups;
                delete resource_table;
                // Core transform offsets may point into MarioAnimator matrices,
                // so its own object remains until the entire captured graph ends.
                delete animator;
            }
        };
    }

    struct MarioAnimatorConstructionScope::Storage {
        MarioAnimator& animator;
        std::shared_ptr<ModelManagerOwner> owner;
        std::shared_ptr<MarioAnimatorLifetime> lifetime;
        std::optional<JkrAllocationScope> heap;
        std::optional<J3dCommandScope> commands;
        J3DSys system;
        XanimePlayer* previous_player;
        OSThread* thread;
        int exceptions;
        int mutex_count;

        Storage(MarioAnimator& value, std::shared_ptr<ModelManagerOwner> model_owner)
            : animator(value), owner(std::move(model_owner)), lifetime(std::make_shared<MarioAnimatorLifetime>()),
              system(j3dSys), previous_player(owner->manager().mXanimePlayer), thread(OSGetCurrentThread()),
              exceptions(std::uncaught_exceptions()),
              mutex_count(MR::MutexHolder<0>::sMutex.thread == thread ? MR::MutexHolder<0>::sMutex.count : 0) {
            auto* service = ResourceHolderService::active();
            if (!service || service->allocation_domain() != owner->allocation_domain()) {
                throw std::logic_error("MarioAnimator requires its model's retained scene resource cohort");
            }
            lifetime->domain = owner->allocation_domain();
            lifetime->resources = service->retain(*MR::getResourceHolder(animator.mActor));
            // Original init assigns each pointer only after a successful complete
            // child construction. Known null slots permit capture during unwind.
            animator.mResourceTable = nullptr;
            animator.mXanimePlayer = nullptr;
            animator.mXanimePlayerUpper = nullptr;
            animator._120 = nullptr;
            // Reserve/store host metadata before entering the original init body;
            // the scope destructor never allocates or throws during capture.
            owner->retain_lifetime_dependency(lifetime);

        }

        ~Storage() {
            const bool complete = std::uncaught_exceptions() == exceptions;
            lifetime->capture(animator, complete);
            if (!complete) {
                owner->manager().mXanimePlayer = previous_player;
                auto& mutex = MR::MutexHolder<0>::sMutex;
                while (mutex.thread == thread && mutex.count > mutex_count) OSUnlockMutex(&mutex);
            }
            j3dSys = system;
            commands.reset();
            heap.reset();
        }
    };

    MarioAnimatorConstructionScope::MarioAnimatorConstructionScope(MarioAnimator& animator) {
        {
            JkrHostAllocationScope host;
            auto owner = retain_actor_model_owner(animator.mActor);
            if (!owner || &owner->manager() != animator.mActor->mModelManager) {
                throw std::logic_error("MarioAnimator requires the actor's actual ModelManager owner");
            }
            _storage = std::make_unique<Storage>(animator, std::move(owner));
        }
        // Enter persistent scopes after the host escape has restored routing.
        // Inline optionals need no heap allocation of their own scope objects.
        _storage->heap.emplace(_storage->lifetime->domain);
        _storage->commands.emplace();
    }

    MarioAnimatorConstructionScope::~MarioAnimatorConstructionScope() {
        // Storage retires the scopes before releasing host metadata. Wrapping
        // this reset in a host escape would overwrite restored routing on exit.
        _storage.reset();
    }
}
