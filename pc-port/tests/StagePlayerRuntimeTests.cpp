#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "compat/StagePlayerRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "render/J3dAnimation.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance, std::string_view message) {
        if (std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     ";expected=" + std::to_string(expected));
        }
    }

    void require_unit(const TVec3f &value, std::string_view message) {
        require_near(value.length(), 1.0F, 0.0001F, message);
    }

    void test_basis_tracks_arbitrary_gravity() {
        const auto ordinary = smgpc::compat::calculate_stage_player_basis({0.0F, -1.0F, 0.0F},
                                                                          {0.0F, 0.0F, 1.0F});
        require_near(ordinary.side.x, 1.0F, 0.0001F, "ordinary gravity should retain the start side");
        require_near(ordinary.up.y, 1.0F, 0.0001F, "ordinary gravity should produce world up");
        require_near(ordinary.front.z, 1.0F, 0.0001F, "ordinary gravity should retain the start front");

        const auto sideways = smgpc::compat::calculate_stage_player_basis({-1.0F, 0.0F, 0.0F},
                                                                          {0.0F, 0.0F, 1.0F});
        require_unit(sideways.side, "sideways-gravity side must stay normalized");
        require_unit(sideways.up, "sideways-gravity up must stay normalized");
        require_unit(sideways.front, "sideways-gravity front must stay normalized");
        require_near(sideways.up.x, 1.0F, 0.0001F, "player up should oppose sideways gravity");
        require_near(sideways.side.dot(sideways.up), 0.0F, 0.0001F, "side and up should be orthogonal");
        require_near(sideways.front.dot(sideways.up), 0.0F, 0.0001F, "front should be tangent to gravity");
    }

    void test_camera_relative_input_is_tangent() {
        auto pose = smgpc::camera::CameraPose{};
        pose.eye = {0.0F, 300.0F, -900.0F};
        pose.watch = {0.0F, 100.0F, 0.0F};

        const auto forward = smgpc::compat::calculate_camera_relative_stage_player_input(
            0.0F, 1.0F, {0.0F, -1.0F, 0.0F}, pose, {0.0F, 0.0F, 1.0F});
        require_near(forward.x, 0.0F, 0.0001F, "camera forward should not add side movement");
        require_near(forward.y, 0.0F, 0.0001F, "camera forward should be tangent to gravity");
        require_near(forward.z, 1.0F, 0.0001F, "camera forward should follow the projected view");

        const auto right = smgpc::compat::calculate_camera_relative_stage_player_input(
            1.0F, 0.0F, {0.0F, -1.0F, 0.0F}, pose, {0.0F, 0.0F, 1.0F});
        require_near(right.x, 1.0F, 0.0001F, "right input should use camera-relative side");
        require_near(right.y, 0.0F, 0.0001F, "right input should be tangent to gravity");

        const auto sideways = smgpc::compat::calculate_camera_relative_stage_player_input(
            0.7F, 0.7F, {-1.0F, 0.0F, 0.0F}, pose, {0.0F, 0.0F, 1.0F});
        require_near(sideways.dot(TVec3f{-1.0F, 0.0F, 0.0F}), 0.0F, 0.0001F,
                     "arbitrary-gravity input must remain on the gravity tangent plane");

        const auto dead = smgpc::compat::calculate_camera_relative_stage_player_input(
            0.05F, 0.05F, {0.0F, -1.0F, 0.0F}, pose, {0.0F, 0.0F, 1.0F});
        require_near(dead.length(), 0.0F, 0.0001F, "input inside the dead zone should be zero");
    }

    void test_velocity_handles_ground_jump_and_terminal_speed() {
        const auto grounded = smgpc::compat::calculate_stage_player_velocity(
            {}, {0.0F, 0.0F, 1.0F}, {0.0F, -1.0F, 0.0F}, true, false);
        require_near(grounded.z, 14.0F * 0.22F, 0.0001F, "ground movement should accelerate toward maximum speed");
        require_near(grounded.y, -1.5F, 0.0001F, "ground movement should retain a collision contact probe");

        const auto jump = smgpc::compat::calculate_stage_player_velocity(
            {}, {}, {0.0F, -1.0F, 0.0F}, true, true);
        require_near(jump.y, 20.0F, 0.0001F, "jump should launch opposite gravity");

        const auto rising = smgpc::compat::calculate_stage_player_velocity(
            {0.0F, 10.0F, 0.0F}, {}, {0.0F, -1.0F, 0.0F}, false, false);
        require_near(rising.y, 8.8F, 0.0001F, "air gravity should reduce upward speed each frame");

        const auto terminal = smgpc::compat::calculate_stage_player_velocity(
            {0.0F, -80.0F, 0.0F}, {}, {0.0F, -1.0F, 0.0F}, false, false);
        require_near(terminal.y, -40.0F, 0.0001F, "fall speed should clamp to the configured terminal speed");
    }

    void test_animation_playback_modes() {
        require_near(smgpc::render::j3d_animation_frame(0U, 20, 25.0F), 19.999F, 0.0001F,
                     "one-shot animation should clamp just below its exclusive end frame");
        require(smgpc::render::j3d_animation_stopped(0U, 20, 25.0F),
                "one-shot animation should report stopped after its duration");
        require_near(smgpc::render::j3d_animation_frame(1U, 20, 25.0F), 0.0F, 0.0001F,
                     "one-shot-reset animation should return to its first frame");
        require_near(smgpc::render::j3d_animation_frame(2U, 20, 25.0F), 5.0F, 0.0001F,
                     "loop animation should repeat from its local origin");
        require(!smgpc::render::j3d_animation_stopped(2U, 20, 200.0F),
                "loop animation should not report stopped");
        require_near(smgpc::render::j3d_animation_frame(3U, 20, 25.0F), 15.0F, 0.0001F,
                     "one-shot reverse animation should run back toward its first frame");
        require_near(smgpc::render::j3d_animation_frame(3U, 20, 40.0F), 0.0F, 0.0001F,
                     "one-shot reverse animation should reach frame zero at the end of its reverse leg");
        require(!smgpc::render::j3d_animation_stopped(3U, 20, 40.0F),
                "one-shot reverse animation should not stop until the update after reaching frame zero");
        require(smgpc::render::j3d_animation_stopped(3U, 20, 41.0F),
                "one-shot reverse animation should stop when the next unit-rate update underflows");
        require_near(smgpc::render::j3d_animation_frame(3U, 20, 41.0F), 1.0F, 0.0001F,
                     "one-shot reverse animation should reflect the underflow once before stopping");
        require_near(smgpc::render::j3d_animation_frame(4U, 20, 45.0F), 7.0F, 0.0001F,
                     "loop-reverse animation should reflect at end minus one like J3DFrameCtrl");

        auto btk = smgpc::render::J3dBtkAnimationSummary{};
        btk.attribute = 0U;
        btk.frame_max = 20;
        btk.translation_values = {0.0F, 0.0F, 1.0F, 20.0F, 20.0F, 1.0F};
        auto material = smgpc::render::J3dBtkMaterialAnimationSummary{};
        material.material_name = "RegressionMaterial";
        material.tex_matrix_id = 0U;
        material.tracks[0U].translation = {.max_frame = 2U, .offset = 0U, .type = 0U};
        btk.materials.push_back(material);
        const auto late_btk = smgpc::render::j3d_evaluate_btk_texture_srt(btk, "RegressionMaterial", 0U, 25.0F);
        require(late_btk.has_value(), "BTK regression fixture should resolve its material track");
        require_near(late_btk->translate_s, 5.0F, 0.0001F,
                     "BTK should retain its independent modulo timeline when the renderer frame is already advanced");
    }

    void test_follow_camera_preserves_pose_and_tracks_translation() {
        const auto position = TVec3f{20.0F, 30.0F, 40.0F};
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        const auto initial = smgpc::compat::make_stage_player_fallback_camera(position, gravity,
                                                                              {0.0F, 0.0F, 1.0F});
        const auto follow = smgpc::compat::make_stage_player_follow_camera(initial, position, gravity,
                                                                           {0.0F, 0.0F, 1.0F});
        const auto reconstructed = smgpc::compat::calculate_stage_player_follow_camera_pose(follow, position, gravity);
        require_near(reconstructed.eye.x, initial.eye.x, 0.0001F, "follow camera should preserve initial eye X");
        require_near(reconstructed.eye.y, initial.eye.y, 0.0001F, "follow camera should preserve initial eye Y");
        require_near(reconstructed.eye.z, initial.eye.z, 0.0001F, "follow camera should preserve initial eye Z");

        const auto delta = TVec3f{100.0F, -50.0F, 25.0F};
        const auto translated = smgpc::compat::calculate_stage_player_follow_camera_pose(follow, position + delta, gravity);
        require_near(translated.eye.x - initial.eye.x, delta.x, 0.0001F, "camera eye should follow player X");
        require_near(translated.eye.y - initial.eye.y, delta.y, 0.0001F, "camera eye should follow player Y");
        require_near(translated.eye.z - initial.eye.z, delta.z, 0.0001F, "camera eye should follow player Z");
        require_near(translated.watch.z - initial.watch.z, delta.z, 0.0001F, "camera watch should follow player Z");

        const auto sideways = smgpc::compat::calculate_stage_player_follow_camera_pose(
            follow, position, {-1.0F, 0.0F, 0.0F});
        require_near(sideways.up.x, 1.0F, 0.0001F, "follow camera up should adapt to changed gravity");
        require(std::isfinite(sideways.eye.x) && std::isfinite(sideways.eye.y) && std::isfinite(sideways.eye.z),
                "follow camera should remain finite when gravity rotates");
    }

    void test_player_service_synchronizes_actor_state() {
        auto service = smgpc::runtime::PlayerSystemService{};
        const auto initial_revision = service.base_matrix_revision();
        service.reset_stage_state();
        require(service.base_matrix_revision() > initial_revision, "stage reset should invalidate external matrix consumers");
        require(service.is_control_enabled() && service.is_swing_permitted() && !service.is_player_hidden(),
                "stage reset should restore control, swing, and visibility");

        auto actor = LiveActor("test-player");
        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor.mVelocity.set(1.0F, 2.0F, 3.0F);
        actor.mGravity.set(-1.0F, 0.0F, 0.0F);
        actor.mBindedGround = true;
        actor.calcAndSetBaseMtx();
        service.attach_actor(actor);
        require(service.attached_actor() == &actor && service.has_base_matrix(),
                "attaching should expose a concrete player and matrix");
        require_near(service.position()[2U], 30.0F, 0.0001F, "position should synchronize from the actor");
        require_near(service.velocity()[1U], 2.0F, 0.0001F, "velocity should synchronize from the actor");
        require_near(service.gravity()[0U], -1.0F, 0.0001F, "gravity should synchronize from the actor");
        require(service.is_on_ground(), "grounded state should synchronize from the actor binder");

        {
            const auto player_context = smgpc::compat::ScopedPlayerSystemServiceOverride{service};
            require(MR::getPlayerPos() == &actor.mPosition,
                    "the MR position API should retain the original direct actor-pointer semantics");
            require(MR::getPlayerVelocity() == &actor.mVelocity,
                    "the MR velocity API should expose the attached actor velocity");
            require(MR::getPlayerGravity() == &actor.mGravity,
                    "the MR gravity API should expose the attached actor gravity");
            require_near(MR::getPlayerPos()->z, 30.0F, 0.0001F,
                         "the MR position API should return synchronized player state");
            require_near(MR::getPlayerVelocity()->y, 2.0F, 0.0001F,
                         "the MR velocity API should not return the old zero placeholder");
            require_near(MR::getPlayerGravity()->x, -1.0F, 0.0001F,
                         "the MR gravity API should preserve non-world gravity");
            require(MR::isOnGroundPlayer(), "the MR grounded API should use the same player service state");

            require(MR::tryStartDemoMarioPuppetable(&actor, "player-control-test"),
                    "puppetable demo acquisition should succeed");
            require(!service.is_control_enabled(),
                    "puppetable demos should own and disable player control");
            service.reset_stage_state();
            require(!service.is_control_enabled(),
                    "puppetable control ownership should survive stage actor reset");
            MR::endDemo(&actor, "player-control-test");
            require(service.is_control_enabled(),
                    "ending a puppetable demo should restore prior player control");
            service.attach_actor(actor);

            actor.startBck("DemoPeachCastleGate", nullptr);
            actor.mVelocity.set(7.0F, -9.0F, 11.0F);
            actor.mBindedGround = true;
            actor.mBindedWall = true;
            actor.mBindedRoof = true;
            actor.mFlag.mIsNoBind = true;
            require(MR::tryStartDemoMarioPuppetable(&actor, "opening-demo-lifecycle"),
                    "opening demo should acquire puppetable player control");
            Mtx terminal_demo_matrix{
                {1.0F, 0.0F, 0.0F, 410.0F},
                {0.0F, 1.0F, 0.0F, 520.0F},
                {0.0F, 0.0F, 1.0F, 630.0F},
            };
            service.set_base_matrix(terminal_demo_matrix);
            MR::endDemo(&actor, "opening-demo-lifecycle");
            MR::initPlayerAfterOpeningDemo();
            require(actor.currentBckName() == "Wait",
                    "opening-demo teardown should restore the gameplay Wait animation");
            require(service.is_control_enabled() && !service.has_forced_base_matrix(),
                    "opening-demo teardown should release player control and the puppet matrix override");
            require_near(actor.mPosition.x, 410.0F, 0.0001F,
                         "releasing the puppet override should retain its final restored position");
            require_near(actor.mVelocity.length(), 0.0F, 0.0001F,
                         "opening-demo teardown should clear transient velocity");
            require(!actor.mBindedGround && !actor.mBindedWall && !actor.mBindedRoof && !actor.mFlag.mIsNoBind,
                    "opening-demo teardown should clear transient binder state");

            {
                auto teardown_owner = std::make_unique<LiveActor>("demo-owner-teardown");
                require(MR::tryStartDemoMarioPuppetable(teardown_owner.get(), "owner-teardown-demo"),
                        "teardown regression demo should acquire ownership");
                require(MR::isDemoActive("owner-teardown-demo") && !service.is_control_enabled(),
                        "owned demo state should be active before its actor is destroyed");
                teardown_owner.reset();
                require(!MR::isDemoActive() && service.is_control_enabled(),
                        "destroying the demo owner should clear global demo state and restore player control");
            }
        }

        service.hide_player();
        require(service.is_player_hidden() && actor.mFlag.mIsHiddenModel,
                "hide should immediately affect the attached model");
        service.show_player();
        require(!actor.mFlag.mIsHiddenModel, "show should immediately restore the attached model");

        service.set_swing_permission(false);
        require(!service.is_swing_permitted(), "swing permission should be independently suppressible");
        service.set_swing_permission(true);
        service.disable_control();
        require(service.is_swing_permitted() && !service.is_control_enabled(),
                "swing and control state should be retained by the player boundary");
        service.enable_control(true);
        require(service.is_control_enabled() && service.consume_reset_condition_request() &&
                    !service.consume_reset_condition_request(),
                "control reset requests should be consumed exactly once");

        Mtx matrix{
            {1.0F, 0.0F, 0.0F, 100.0F},
            {0.0F, 1.0F, 0.0F, 200.0F},
            {0.0F, 0.0F, 1.0F, 300.0F},
        };
        const auto revision_before_matrix = service.base_matrix_revision();
        service.set_base_matrix(matrix);
        require(service.base_matrix_revision() > revision_before_matrix,
                "external matrices should have a consumable revision");
        require_near(actor.mPosition.x, 100.0F, 0.0001F, "external matrix should update actor position X");
        require_near(actor.mPosition.y, 200.0F, 0.0001F, "external matrix should update actor position Y");
        require_near(actor.mPosition.z, 300.0F, 0.0001F, "external matrix should update actor position Z");
        require_near(actor.getBaseMatrix().m[0U], 1.0F, 0.0001F, "external matrix should preserve side axis");
        require_near(actor.getBaseMatrix().m[5U], 1.0F, 0.0001F, "external matrix should preserve up axis");
        service.clear_stage_state();
        require(service.attached_actor() == nullptr, "stage teardown should release the actor pointer");
        require(!service.has_base_matrix(), "stage teardown should clear the prior player matrix");
        require_near(service.position()[0U], 0.0F, 0.0001F, "stage teardown should clear cached player position");
    }

    void test_wpad_swing_edge_is_frame_stable() {
        auto wpad = smgpc::runtime::WpadService{};
        wpad.set_connected(WPAD_CHAN0, true);

        wpad.begin_frame();
        wpad.set_swing(WPAD_CHAN0, true, false);
        require(wpad.is_core_swing_triggered(WPAD_CHAN0), "the first core swing frame should trigger");

        wpad.begin_frame();
        wpad.set_swing(WPAD_CHAN0, true, false);
        require(!wpad.is_core_swing_triggered(WPAD_CHAN0), "a held core swing should not retrigger");

        wpad.begin_frame();
        wpad.set_swing(WPAD_CHAN0, false, false);
        wpad.begin_frame();
        wpad.set_swing(WPAD_CHAN0, true, false);
        require(wpad.is_core_swing_triggered(WPAD_CHAN0), "a new core swing edge should retrigger");
    }

    void test_optional_real_disc_player_resources() {
        const auto *disc_path = std::getenv("SMGPC_REAL_DISC");
        if (disc_path == nullptr || disc_path[0] == '\0') {
            std::cout << "[skip] real-disc player resource test (set SMGPC_REAL_DISC)\n";
            return;
        }

        aurora_dvd_close();
        require(aurora_dvd_open(disc_path), "SMGPC_REAL_DISC should point to a readable SMG image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        const auto start = smgpc::scene::resolve_stage_start_info(dvd, "HeavensDoorGalaxy", 1, 0, 0);
        require(start.has_value(), "real stage data should resolve its requested StartInfo");
        require(dvd.find_object_archive("Mario").has_value(),
                "the compatibility player model should resolve through the generic ObjectData lookup");
        const auto animation_path = dvd.find_object_archive("MarioAnime");
        require(animation_path.has_value(), "the separate player animation archive should resolve generically");
        const auto &animation_archive = dvd.archive_for_path(*animation_path);
        const auto *wait_entry = animation_archive.find_by_basename("Wait.bck");
        require(wait_entry != nullptr, "the requested Wait animation should be selected by basename");
        const auto wait_animation = smgpc::render::inspect_j3d_animation(animation_archive.file_data(*wait_entry));
        require(wait_animation.bck.has_value() && wait_animation.bck->joint_count != 0U,
                "the requested Wait resource should decode as a skeletal animation");
        require(wait_animation.bck->attribute == 2U && wait_animation.bck->frame_max == 180,
                "Wait should retain its original looping 180-frame playback mode");
        const auto *arrival_entry = animation_archive.find_by_basename("DemoPeachCastleGate.bck");
        require(arrival_entry != nullptr, "the post-picturebook player arrival animation should resolve by name");
        const auto arrival_animation = smgpc::render::inspect_j3d_animation(animation_archive.file_data(*arrival_entry));
        require(arrival_animation.bck.has_value() && arrival_animation.bck->attribute == 0U &&
                    arrival_animation.bck->frame_max == 299,
                "the arrival animation should retain its original one-shot 299-frame playback mode");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"basis tracks arbitrary gravity", test_basis_tracks_arbitrary_gravity},
        TestCase{"camera-relative input is tangent", test_camera_relative_input_is_tangent},
        TestCase{"velocity handles ground, jump, and terminal speed", test_velocity_handles_ground_jump_and_terminal_speed},
        TestCase{"animation playback modes honor local lifecycle", test_animation_playback_modes},
        TestCase{"follow camera preserves pose and follows translation", test_follow_camera_preserves_pose_and_tracks_translation},
        TestCase{"player service synchronizes actor state", test_player_service_synchronizes_actor_state},
        TestCase{"wpad swing edge is frame-stable", test_wpad_swing_edge_is_frame_stable},
        TestCase{"optional real-disc player resources", test_optional_real_disc_player_resources},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " stage-player runtime test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " stage-player runtime test(s) passed\n";
    return 0;
}
