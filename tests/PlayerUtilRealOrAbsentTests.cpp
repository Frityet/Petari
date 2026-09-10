#include "Game/Util/PlayerUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/RuntimeServices.hpp"

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

    require_unavailable([] { (void)MR::getPlayerPos(); }, "missing player must not manufacture an origin position");
    require_unavailable([] { (void)MR::getPlayerCenterPos(); }, "missing player must not manufacture a center position");
    require_unavailable([] { (void)MR::getPlayerRotate(); }, "missing player must not manufacture a rotation");
    require_unavailable([] { (void)MR::getPlayerVelocity(); }, "missing player must not manufacture zero velocity");
    require_unavailable([] { (void)MR::getPlayerGravity(); }, "missing player must not manufacture world-down gravity");
    require_unavailable([] { (void)MR::getPlayerBaseMtx(); }, "missing player must not manufacture an identity base matrix");
    ++passed;

    auto output = TVec3f{2.0F, 3.0F, 4.0F};
    require_unavailable([&] { MR::getPlayerUpVec(&output); }, "missing player must not manufacture an up axis");
    require_unavailable([&] { MR::getPlayerFrontVec(&output); }, "missing player must not manufacture a front axis");
    require_unavailable([&] { MR::getPlayerSideVec(&output); }, "missing player must not manufacture a side axis");
    require(output.x == 2.0F && output.y == 3.0F && output.z == 4.0F,
            "unavailable axis queries must preserve the caller's value");
    require_unavailable([] { (void)MR::calcDistanceToPlayer(TVec3f{}); },
                        "missing player must not manufacture a distance from the origin");
    ++passed;

    require_unavailable([] { MR::showPlayer(); }, "showPlayer must not report success without a player actor");
    require_unavailable([] { MR::hidePlayer(); }, "hidePlayer must not report success without a player actor");
    require_unavailable([] { MR::startBckPlayer("Wait", nullptr); }, "animation start must not succeed without a player model");
    require_unavailable([] { (void)MR::getBckFrameMaxPlayer("Wait"); }, "animation lookup must not invent a zero frame count");
    require_unavailable([] { (void)MR::isBckStoppedPlayer(); }, "animation state must not report stopped without a player model");
    require_unavailable([] { MR::incPlayerOxygen(1U); }, "oxygen mutation must not report success without player state");
    require_unavailable([] { (void)MR::isOnGroundPlayer(); },
                        "ground state must not become false without a player actor");
    require_unavailable([] { MR::setPlayerSwingPermission(true); },
                        "swing permission must not be stored through the Game API without a player actor");
    require_unavailable([] { MR::offPlayerControl(); },
                        "control disable must not silently succeed without a player actor");
    require_unavailable([] { MR::onPlayerControl(true); },
                        "control enable must not silently succeed without a player actor");
    require_unavailable([] { (void)MR::isOffPlayerControl(); },
                        "missing control state must not become false");
    require_unavailable([] { MR::initPlayerAfterOpeningDemo(); },
                        "opening-demo teardown must not report success without a player actor");
    ++passed;

    auto player = smgpc::runtime::PlayerSystemService{};
    auto attached_actor = LiveActor("host-player-snapshot-test");
    attached_actor.mPosition.set(10.0F, 20.0F, 30.0F);
    player.attach_actor(attached_actor);
    require(player.attached_actor() == &attached_actor && !player.has_base_matrix() &&
                player.position()[0] == 10.0F && player.position()[1] == 20.0F &&
                player.position()[2] == 30.0F,
            "the host snapshot must preserve the attached actor without manufacturing a model matrix");

    auto camera = smgpc::runtime::CameraSystemService{};
    const auto camera_pose = smgpc::camera::CameraPose{
        .eye = {100.0F, 200.0F, 300.0F},
        .watch = {10.0F, 20.0F, 30.0F},
        .up = {0.0F, 1.0F, 0.0F},
    };
    camera.set_game_camera_pose(camera_pose);
    player.clear_stage_state();
    require(player.attached_actor() == nullptr && !player.has_base_matrix() &&
                player.actor_center_position() == nullptr,
            "clearing a stage must leave host player state absent until an actor attaches");
    require_unavailable([] { (void)MR::getPlayerPos(); },
                        "a host snapshot must not substitute for the missing original Mario owner");
    require(player.position()[0] == 0.0F && player.position()[1] == 0.0F &&
                player.position()[2] == 0.0F && player.gravity()[0] == 0.0F &&
                player.gravity()[1] == 0.0F && player.gravity()[2] == 0.0F,
            "clearing a stage must discard the attached actor snapshot without inventing gravity");
    require(camera.game_camera_pose().has_value() && camera.game_camera_pose()->eye.x == camera_pose.eye.x,
            "a real stage camera must remain usable independently of absent player state");
    ++passed;

    std::cout << "Player real-or-absent tests passed: " << passed << "/4\n";
    return 0;
}
