#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/Util/LiveActorUtil.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void test_npc_capability_allocates_and_tracks_lifecycle() {
        auto actor = NPCActor("lod-capability-test");
        auto caps = NPCActorCaps("unused");
        caps.mLodCtrl = true;
        caps.mMakeActor = true;
        actor.initialize(JMapInfoIter{}, caps);

        require(actor.mLodCtrl != nullptr, "the NPC LOD capability must allocate a controller");
        require(actor.mLodCtrl->mActor == &actor && actor.mLodCtrl->_8 == &actor,
                "the controller must retain the NPC as its high-detail host");
        require(actor.mLodCtrl->_1B != 0, "NPC controllers must use camera-Z distance like the original factory");
        require(!actor.isDead(), "mMakeActor must still appear an NPC after creating its LOD controller");

        actor.mLodCtrl->invalidate();
        require(actor.mLodCtrl->_18 == 0, "the guide-sequence invalidate call must suspend LOD updates");
        actor.mLodCtrl->validate();
        actor.control();
        require(actor.mLodCtrl->_18 != 0 && actor.mLodCtrl->_8 == &actor,
                "the guide-sequence validate call must safely restore a high-only NPC controller");

        actor.makeActorDead();
        require(actor.isDead(), "NPC death must retain the base actor lifecycle");
        require(actor.mLodCtrl->mActorLightCtrl == nullptr,
                "NPC death must run the LOD kill lifecycle before killing the host");

        actor.makeActorAppeared();
        require(!actor.isDead(), "NPC appearance must retain the base actor lifecycle");
        require(actor.mLodCtrl->_8 == &actor && !MR::isHiddenModel(&actor),
                "NPC appearance must restore the high-detail host model");

        delete actor.mLodCtrl;
        actor.mLodCtrl = nullptr;
    }

    void test_disabled_capability_does_not_allocate() {
        auto actor = NPCActor("lod-disabled-test");
        auto caps = NPCActorCaps("unused");
        caps.mMakeActor = true;
        actor.initialize(JMapInfoIter{}, caps);

        require(actor.mLodCtrl == nullptr, "NPCs without the LOD capability must retain a null controller");
        require(!actor.isDead(), "the LOD capability must not alter ordinary NPC appearance");
    }

    void test_controller_preserves_original_model_transition_order() {
        auto host = LiveActor("lod-high");
        auto middle = ModelObj("lod-middle", "", nullptr, -1, -1, -1, false);
        host.makeActorAppeared();
        middle.makeActorDead();
        host.mPosition.set(10.0F, 20.0F, 30.0F);
        host.mRotation.set(40.0F, 50.0F, 60.0F);
        host.mScale.set(2.0F, 3.0F, 4.0F);

        auto ctrl = LodCtrl(&host, JMapInfoIter{});
        ctrl._10 = &middle;
        ctrl.validate();

        auto high = false;
        auto forceMiddle = true;
        auto low = false;
        auto hidden = false;
        ctrl.setViewCtrlPtr(&high, &forceMiddle, &low, &hidden);
        ctrl.update();
        require(ctrl._8 == &middle && !middle.isDead(),
                "the first middle-detail update must appear the replacement model");
        require(!MR::isHiddenModel(&host),
                "the original two-step transition keeps the host visible while the replacement appears");
        require(middle.mPosition.x == host.mPosition.x && middle.mRotation.y == host.mRotation.y && middle.mScale.z == host.mScale.z,
                "the active replacement model must follow the host transform");

        ctrl.update();
        require(MR::isHiddenModel(&host) && !middle.isDead(),
                "the second middle-detail update must hide the high-detail model");

        high = true;
        forceMiddle = false;
        ctrl.update();
        require(!MR::isHiddenModel(&host) && !middle.isDead(),
                "the first high-detail update must restore the hidden host before retiring the replacement");
        ctrl.update();
        require(middle.isDead() && ctrl._8 == &host,
                "the second high-detail update must retire the replacement model");

        high = false;
        hidden = true;
        ctrl.update();
        require(MR::isHiddenModel(&host) && ctrl._8 == nullptr,
                "the view-control hidden flag must suppress every LOD model");

        hidden = false;
        ctrl.update();
        require(!MR::isHiddenModel(&host) && ctrl._8 == &host,
                "clearing the hidden flag must restore the available high-detail model");
    }
}  // namespace

int main() {
    try {
        constexpr auto tests = std::array{
            std::pair{"NPC capability allocates and tracks lifecycle", &test_npc_capability_allocates_and_tracks_lifecycle},
            std::pair{"disabled capability does not allocate", &test_disabled_capability_does_not_allocate},
            std::pair{"controller preserves original transition order", &test_controller_preserves_original_model_transition_order},
        };
        for (const auto &[name, test] : tests) {
            test();
            std::cout << "[ok] " << name << '\n';
        }
        std::cout << tests.size() << " LOD compatibility test(s) passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[failed] " << error.what() << '\n';
        return 1;
    }
}
