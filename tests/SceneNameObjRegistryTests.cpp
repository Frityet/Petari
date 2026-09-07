#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include <aurora/exception.hpp>

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
    void require(bool value, const char *message) {
        if (!value)
            aurora::throw_host_exception<std::runtime_error>(message);
    }
}  // namespace

int main() {
    try {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        const auto root_free = heaps->root_heap().getFreeSize();
        const auto registered = smgpc::compat::name_obj_runtime_state_count();
        for (int generation = 0; generation < 16; ++generation) {
            auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 256U << 10);
            {
                smgpc::scene::SceneNameObjRegistry registry(domain);
                require(JKRHeap::findFromRoot(&registry.holder()) == &domain->heap(),
                        "the scene registry must own an actual NameObjHolder in its retained Game heap");
                std::array<std::unique_ptr<NameObj>, 20> objects;
                for (std::size_t index = 0; index < objects.size(); ++index) {
                    const auto name = "Registry " + std::to_string(index);
                    const smgpc::compat::JkrAllocationScope game(domain);
                    objects[index] = std::make_unique<NameObj>(name.c_str());
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
