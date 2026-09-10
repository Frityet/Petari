#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneLifetimeBinding.hpp"
#include <aurora/exception.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
    void require(bool value, const char *message) {
        if (!value)
            aurora::throw_host_exception<std::runtime_error>(message);
    }
    struct Observer;
    struct DerivedScene final : Scene {
        explicit DerivedScene(Observer &observer);
        ~DerivedScene() override;
        Observer &observer;
    };
    struct Observer {
        Scene *scene = nullptr;
        std::vector<int> order;
        bool original_owners_live = false;
        std::unique_ptr<smgpc::scene::SceneLifetimeBinding> binding;
        static void retire(void *context) noexcept {
            auto &observer = *static_cast<Observer *>(context);
            observer.original_owners_live = observer.scene->mSceneObjHolder && observer.scene->mSpine && observer.scene->_C == 42;
            observer.order.push_back(2);
            // The retirement function must support deleting its own already
            // unpublished binding without accessing it on callback return.
            observer.binding.reset();
        }
    };
    DerivedScene::DerivedScene(Observer &observer) : Scene("Lifetime probe"), observer(observer) {
    }
    DerivedScene::~DerivedScene() {
        observer.order.push_back(1);
    }
    struct IdleNerve final : Nerve {
        void execute(Spine *) const override {
        }
    };
    struct ChildOwner {
        Observer &observer;
        std::unique_ptr<smgpc::scene::SceneLifetimeBinding> binding;
        static void retire(void *context) noexcept {
            auto &owner = *static_cast<ChildOwner *>(context);
            owner.observer.order.push_back(3);
            owner.binding.reset();
        }
    };
}  // namespace

int main() {
    try {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        const auto root_free = heaps->root_heap().getFreeSize();
        const IdleNerve nerve;
        for (int generation = 0; generation < 32; ++generation) {
            Observer observer;
            observer.order.reserve(3);
            auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 256U << 10);
            std::weak_ptr<smgpc::compat::JkrAllocationDomain> retired = domain;
            std::unique_ptr<DerivedScene> scene;
            {
                const smgpc::compat::JkrAllocationScope game(domain);
                scene = std::make_unique<DerivedScene>(observer);
                scene->initSceneObjHolder();
                scene->initNerve(&nerve);
                scene->_C = 42;
            }
            observer.scene = scene.get();
            require(JKRHeap::findFromRoot(scene.get()) == &domain->heap(), "actual derived Scene must belong to the retained Game heap");
            observer.binding = std::make_unique<smgpc::scene::SceneLifetimeBinding>(*scene, &Observer::retire, &observer);
            bool rejected = false;
            try {
                smgpc::scene::SceneLifetimeBinding duplicate(*scene, &Observer::retire, &observer);
            } catch (const std::logic_error &) {
                rejected = true;
            }
            require(rejected, "duplicate scene lifetime ownership must be rejected");
            ChildOwner children{observer};
            children.binding = std::make_unique<smgpc::scene::SceneLifetimeBinding>(
                *scene, &ChildOwner::retire, &children);
            scene.reset();
            require(observer.original_owners_live && observer.order == std::vector<int>{1, 3, 2} &&
                        !observer.binding && !children.binding,
                    "derived destruction must precede child retirement and service retirement while original owners remain alive");
            require(!retired.expired(), "the external Scene owner must retain the Game heap through final base destruction and delete");
            domain.reset();
            require(retired.expired() && heaps->root_heap().getFreeSize() == root_free,
                    "the retired Scene must return its complete Game heap to the original root");
        }
        // Unbinding a native owner cancels only its own retirement callback;
        // an unrelated Scene binding remains valid.
        Observer outer, inner;
        outer.order.reserve(2);
        inner.order.reserve(2);
        {
            DerivedScene a(outer), b(inner);
            outer.scene = &a;
            inner.scene = &b;
            outer.binding = std::make_unique<smgpc::scene::SceneLifetimeBinding>(a, &Observer::retire, &outer);
            inner.binding = std::make_unique<smgpc::scene::SceneLifetimeBinding>(b, &Observer::retire, &inner);
            outer.binding.reset();
        }
        require(outer.order == std::vector<int>{1} && inner.order == std::vector<int>{1, 2},
                "out-of-order unbinding must preserve other actual scene identities");
        std::cout << "Scene derived/base and dependent-service retirement ordering, self-removal, duplicate rejection and 32 original Game heap lifetimes passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
