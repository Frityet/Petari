#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Util/HashUtil.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include <aurora/exception.hpp>

#include <array>
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
    void require(bool value, const char *message) {
        if (!value)
            aurora::throw_host_exception<std::runtime_error>(message);
    }
    class PostpassObject final : public NameObj {
    public:
        PostpassObject(const char *name, std::vector<std::string> &calls,
                       std::unique_ptr<PostpassObject> *append = nullptr)
            : NameObj(name), _calls(calls), _append(append) {}
        void initAfterPlacement() override {
            _calls.emplace_back(getName());
            if (_append && !*_append)
                *_append = std::make_unique<PostpassObject>("Appended during postpass", _calls);
        }
    private:
        std::vector<std::string> &_calls;
        std::unique_ptr<PostpassObject> *_append;
    };

    void attribute_group_native_registry_lifetime() {
        // Repeated arenas exercise original table storage and child destruction.
        for (int cycle = 0; cycle < 2; ++cycle) {
            auto heaps = smgpc::compat::JkrHeapRuntime::create(1U << 20);
            // A reclaiming heap lets this test prove individual child frees;
            // the later scene cases retain original bulk-release SolidHeap semantics.
            auto domain = smgpc::compat::JkrAllocationDomain::retain_heap(heaps, heaps->root_heap(), heaps);
            const auto retired = std::weak_ptr(domain);
            {
                smgpc::scene::SceneNameObjRegistry registry(domain);
                const smgpc::compat::JkrAllocationScope game(domain);
                const auto free_before_group = domain->heap().getFreeSize();
                {
                    GroupCheckManager manager("Original group owner");
                    NameObj member("Aa");
                    NameObj same_name("Aa");
                    NameObj collision("BB");
                    NameObj absent("Absent attribute member with another long name");
                    require(JKRHeap::findFromRoot(manager.mGroups[0]) == &domain->heap() &&
                                JKRHeap::findFromRoot(manager.mGroups[1]) == &domain->heap() &&
                                smgpc::compat::name_obj_runtime_owner(manager.mGroups[0]) == &manager &&
                                smgpc::compat::name_obj_runtime_owner(manager.mGroups[1]) == &manager,
                            "original attribute groups must retain Game child allocation and ownership");
                    const auto free_before = domain->heap().getFreeSize();
                    manager.add(&member, 0);
                    manager.add(&same_name, 0);
                    manager.add(&collision, 0);
                    require(!manager.mGroups[0]->mHashTable->mHasBeenSorted &&
                                manager.mGroups[0]->mHashTable->mCurrentLength == 1,
                            "insertion must retain original unique hashes until the placement postpass");
                    registry.holder().callMethodAllObj(&NameObj::initAfterPlacement);
                    require(manager.mGroups[0]->mHashTable->mHasBeenSorted,
                            "the original NameObj placement phase must sort group hashes");
                    require(manager.isExist(&same_name, 0) && manager.isExist(&collision, 0) &&
                                !manager.isExist(&absent, 0) &&
                                !manager.isExist(&member, 1) &&
                                manager.mGroups[0]->mHashTable->mCurrentLength == 1,
                            "original hash identity must preserve collisions, duplicates and group isolation");
                    require(domain->heap().getFreeSize() == free_before,
                            "insertion and sorting must use the preallocated original table storage");
                    auto* original = new int(7);
                    require(JKRHeap::findFromRoot(original) == &domain->heap(),
                            "native group operations must restore original caller allocation routing");
                    delete original;
                }
                require(domain->heap().getFreeSize() == free_before_group,
                        "individual group destruction must return all child and hash storage to the Game heap");
            }
            domain.reset();
            require(retired.expired(),
                    "actual group destruction must release table allocations before Game arena retirement");
            heaps.reset();
        }
    }
}  // namespace

int main() {
    try {
        attribute_group_native_registry_lifetime();
        auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        const auto root_free = heaps->root_heap().getFreeSize();
        const auto registered = smgpc::compat::name_obj_runtime_state_count();
        for (int generation = 0; generation < 16; ++generation) {
            auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 256U << 10);
            {
                smgpc::scene::SceneNameObjRegistry registry(domain);
                require(JKRHeap::findFromRoot(&registry.holder()) == &domain->heap(),
                        "the scene registry must own an actual NameObjHolder in its retained Game heap");
                std::array<std::string, 20> names;
                std::array<std::unique_ptr<NameObj>, 20> objects;
                for (std::size_t index = 0; index < objects.size(); ++index) {
                    names[index] = "Registry " + std::to_string(index);
                    const smgpc::compat::JkrAllocationScope game(domain);
                    objects[index] = std::make_unique<NameObj>(names[index].c_str());
                }
                require(objects[0]->mExecutorIdx == -1 && registry.holder().find("Registry 0") == objects[0].get(),
                        "the scene holder must include NameObjs without an execution category");
                for (std::size_t index = 0; index < objects.size(); ++index) {
                    const auto name = "Registry " + std::to_string(index);
                    require(registry.holder().find(name.c_str()) == objects[index].get(),
                            "original lookup must resolve names across its sixteen-entry cache capacity");
                }
                require(registry.holder().find("Registry 0") == objects[0].get() && !registry.holder().find("Missing"),
                        "cache eviction must retain original primary membership and missing-name behavior");
                objects[0]->setName("Renamed");
                require(!registry.holder().find("Registry 0") && registry.holder().find("Renamed") == objects[0].get(),
                        "cached lookup must compare each object's current name");
                objects[0]->requestSuspend();
                require(!smgpc::compat::name_obj_is_suspended(objects[0].get()),
                        "a requested suspension must wait for original flag synchronization");
                registry.holder().syncWithFlags();
                require(smgpc::compat::name_obj_is_suspended(objects[0].get()), "the original holder must apply pending suspension");
                objects[0]->requestResume();
                require(smgpc::compat::name_obj_is_suspended(objects[0].get()), "resume must also wait for synchronization");
                registry.holder().syncWithFlags();
                require(!smgpc::compat::name_obj_is_suspended(objects[0].get()), "the original holder must apply pending resume");
                bool rejected = false;
                try {
                    registry.add(*objects[0]);
                } catch (const std::logic_error &) {
                    rejected = true;
                }
                require(rejected, "the native constructor boundary must reject duplicate holder membership");
                objects[0].reset();
                require(!registry.holder().find("Renamed") && registry.holder().find("Registry 19") == objects[19].get(),
                        "individual retirement must remove both original membership and cached identity");
                {
                    smgpc::scene::SceneNameObjRegistry nested(domain);
                    NameObj child("Nested");
                    require(nested.holder().find("Nested") == &child && !registry.holder().find("Nested"),
                            "construction must register only with the current actual scene holder");
                    objects[19].reset();
                    require(!registry.holder().find("Registry 19"), "retirement must remove an older bound scene identity");
                }
                require(smgpc::scene::current_scene_name_obj_registry() == &registry,
                        "nested scope retirement must restore the prior actual scene holder");
                {
                    std::vector<std::string> calls;
                    std::unique_ptr<PostpassObject> appended;
                    PostpassObject first("First postpass", calls, &appended);
                    PostpassObject second("Second postpass", calls);
                    const auto captured = registry.snapshot();
                    registry.holder().callMethodAllObj(&NameObj::initAfterPlacement);
                    require(calls == std::vector<std::string>{"First postpass", "Second postpass"} &&
                                appended && std::ranges::find(captured, appended.get()) == captured.end() &&
                                registry.holder().find("Appended during postpass") == appended.get() &&
                                registry.snapshot().size() == captured.size() + 1,
                            "the original global postpass must preserve registration order and its captured end while callbacks append");
                }
            }
            domain.reset();
            require(!smgpc::scene::current_scene_name_obj_registry() && heaps->root_heap().getFreeSize() == root_free &&
                        smgpc::compat::name_obj_runtime_state_count() == registered,
                    "each scene generation must retire the actual arrays, registrations and complete Game heap");
        }
        std::cout << "Original scene NameObjHolder lookup cache, deferred flags, individual retirement and 16 Game heap lifetimes passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
