#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/TalkUtil.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_unavailable(const std::function<void()>& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }
}

int main() {
    auto passed = 0;
    auto actor = LiveActor("talk-absence-test");
    auto ctrl = TalkMessageCtrl(&actor, TVec3f{}, nullptr);

    require_unavailable([&] { (void)ctrl.requestTalk(); },
                        "normal talk requests must expose an absent TalkDirector");
    require_unavailable([&] { (void)ctrl.requestTalkForce(); },
                        "forced talk requests must expose an absent TalkDirector");
    require_unavailable([&] { (void)ctrl.inMessageArea(); },
                        "message-area queries must expose absent node and AreaObj ownership");
    require_unavailable([&] { (void)MR::tryTalkNearPlayer(&ctrl); },
                        "normal talk helpers must not turn an absent TalkDirector into false");
    require_unavailable([&] { (void)MR::tryTalkTimeKeepDemoMarioPuppetable(&ctrl); },
                        "puppetable talk helpers must not turn an absent TalkDirector into false");
    require_unavailable([&] { (void)MR::tryTalkTimeKeepDemoWithoutPauseMarioPuppetable(&ctrl); },
                        "without-pause talk helpers must not turn an absent TalkDirector into false");
    ++passed;

    require_unavailable([&] { ctrl.startTalk(); }, "talk start must fail explicitly without TalkDirector");
    require_unavailable([&] { ctrl.startTalkForce(); }, "forced talk start must fail explicitly without TalkDirector");
    require_unavailable([&] { ctrl.endTalk(); }, "talk end must fail explicitly without TalkDirector");
    require_unavailable([&] { MR::forwardNode(&ctrl); }, "node traversal must fail explicitly without a real message node tree");
    ++passed;

    require_unavailable([&] { (void)ctrl.getMessageID(); }, "missing placement message data must not become message zero");
    ++passed;

    require_unavailable([&] { MR::registerDemoSimpleCastAll(&actor); },
                        "simple-cast registration must not be a no-op without a real DemoDirector");
    require(MR::joinToGroupArray(&actor, JMapInfoIter{}, nullptr, 32) == nullptr,
            "the retail invalid-placement case must remain an ordinary absent group");
    require_unavailable([&] { (void)MR::joinToGroupArray(nullptr, JMapInfoIter{}, nullptr, 32); },
                        "group registration must not silently accept a missing actor");
    ++passed;

    std::cout << "Talk real-or-absent tests passed: " << passed << "/4\n";
    return 0;
}
