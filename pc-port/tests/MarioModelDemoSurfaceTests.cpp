#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/live_actor/LiveActorModel.hpp"
#include "runtime/RuntimeContext.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    class ProofNerve final : public Nerve {
    public:
        void execute(Spine*) const override {
        }
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto* configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error, "the Mario model proof requires a readable working directory");
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        throw std::runtime_error(
            "the Mario model proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    struct FrameProof {
        std::size_t packet_count = 0U;
        std::size_t source_triangles = 0U;
        std::size_t parsed_display_list_bytes = 0U;
        std::size_t animated_joint_packets = 0U;
        float bck_frame = 0.0F;
        std::int16_t bck_frame_max = 0;
    };

#ifndef NDEBUG
    [[nodiscard]] FrameProof collect_mario_frame_proof(
        const smgpc::runtime::RuntimeContext& runtime, std::uint64_t frame_index) {
        auto proof = FrameProof{};
        for (const auto& packet : runtime.j3d_packet_trace()) {
            if (packet.model_name != "Mario" || packet.frame_index != frame_index) {
                continue;
            }
            ++proof.packet_count;
            proof.source_triangles += packet.state.source_triangle_count;
            proof.parsed_display_list_bytes += packet.state.parsed_display_list_bytes;
            proof.animated_joint_packets += packet.state.bck_active && packet.state.bck_joint_count != 0U;
            if (packet.state.bck_active) {
                proof.bck_frame = packet.state.bck_frame;
                proof.bck_frame_max = packet.state.bck_frame_max;
            }
        }
        return proof;
    }
#endif

    void test_real_mario_model_draw_and_wait_tick() {
        const auto disc_path = require_real_disc();
        aurora_dvd_close();
        const auto disc_string = disc_path.string();
        require(aurora_dvd_open(disc_string.c_str()),
                "the selected RMGK01 image must open through Aurora DVD");
        struct DiscCloseGuard final {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } disc_close_guard;
        DVDInit();

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC real Mario model/BCK proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto retry_model = smgpc::render::live_actor::LiveActorModel("Mario", "MarioAnime");
        auto early_load_failed = false;
        try {
            retry_model.requireLoaded();
        } catch (const std::runtime_error&) {
            early_load_failed = true;
        }
        require(early_load_failed,
                "Mario model loading must fail while RuntimeContext is absent");

        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        auto contextless_load_failed = false;
        try {
            retry_model.requireLoaded();
        } catch (const std::runtime_error&) {
            contextless_load_failed = true;
        }
        require(contextless_load_failed,
                "Mario model loading must fail while the renderer context is inactive");

        auto actor = LiveActor("Mario model demo surface");
        auto initial_nerve = ProofNerve{};
        auto replacement_nerve = ProofNerve{};
        const auto camera = smgpc::camera::CameraPose{
            .eye = {0.0F, 80.0F, 500.0F},
            .watch = {0.0F, 80.0F, 0.0F},
            .near_clip = 1.0F,
        };

        auto first = FrameProof{};
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 40U;
            const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            retry_model.requireLoaded();
            require(retry_model.isLoaded() && retry_model.joint_count() != 0U,
                    "an early model load attempt must not poison a later valid real-disc load");
            actor.initModelManagerWithAnm("Mario", "MarioAnime", true);
            actor.initNerve(&initial_nerve);
            actor.appear();
            smgpc::compat::require_actor_model(&actor);
            auto* model = smgpc::compat::actor_model(&actor);
            require(model != nullptr && model->isLoaded(),
                    "Mario.arc must produce a real loaded LiveActorModel");
            require(smgpc::compat::actor_model_joint_count(&actor) != 0U,
                    "Mario.bdl must expose real joints");
            const auto frame_max = smgpc::compat::require_actor_bck(&actor, "Wait", nullptr);
            require(frame_max == 180,
                    "MarioAnime.arc Wait.bck must retain its real 180-frame duration");
            runtime.register_live_actor_model(
                actor, MR::MovementType_Player, MR::CalcAnimType_Player,
                MR::DrawBufferType_Player, -1);
            runtime.scheduler().execute_draw_buffer_opa(camera, MR::DrawBufferType_Player);
            runtime.scheduler().execute_draw_buffer_xlu(camera, MR::DrawBufferType_Player);
#ifndef NDEBUG
            first = collect_mario_frame_proof(runtime, frame.frame_index);
#else
            throw std::runtime_error("the Mario packet proof requires a debug trace build");
#endif
            renderer.end_frame();
        }

        auto second = FrameProof{};
        {
            auto frame = renderer.begin_frame();
            frame.frame_index = 41U;
            const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
#ifndef NDEBUG
            runtime.set_j3d_packet_trace_frame(frame.frame_index);
#endif
            actor.setNerve(&replacement_nerve);
            runtime.scheduler().execute_movement();
            require(actor.isNerve(&replacement_nerve) && actor.getNerveStep() == 1,
                    "the scheduler proof must reset the actor nerve step before its second draw");
            runtime.scheduler().execute_draw_buffer_opa(camera, MR::DrawBufferType_Player);
            runtime.scheduler().execute_draw_buffer_xlu(camera, MR::DrawBufferType_Player);
#ifndef NDEBUG
            second = collect_mario_frame_proof(runtime, frame.frame_index);
#endif
            renderer.end_frame();
        }

        require(first.packet_count != 0U && first.source_triangles != 0U &&
                    first.parsed_display_list_bytes != 0U,
                "the first frame must submit parsed real Mario draw packets and triangles");
        require(first.animated_joint_packets != 0U && first.bck_frame_max == 180,
                "the first Mario draw must bind the real Wait joint animation");
        require(second.packet_count != 0U && second.source_triangles != 0U &&
                    second.parsed_display_list_bytes != 0U &&
                    second.animated_joint_packets != 0U,
                "the second frame must resubmit the real animated Mario model");
        require(second.bck_frame > first.bck_frame &&
                    std::fabs((second.bck_frame - first.bck_frame) - 1.0F) < 0.0001F,
                "Wait.bck must advance by one real runtime frame between rendered frames");

        std::cout << "[proof] disc=" << disc_path.string()
                  << "; packets=" << first.packet_count << '/' << second.packet_count
                  << "; triangles=" << first.source_triangles << '/' << second.source_triangles
                  << "; bck=Wait; frames=" << first.bck_frame << "->" << second.bck_frame
                  << "; frame_max=" << second.bck_frame_max << '\n';
    }
}  // namespace

int main() {
    try {
        test_real_mario_model_draw_and_wait_tick();
        std::cout << "[ok] real Mario archive/model/Wait.bck draw-and-tick surface\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] real Mario model demo surface: " << error.what() << '\n';
        return 1;
    }
}
