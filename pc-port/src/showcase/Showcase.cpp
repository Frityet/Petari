#include "Application.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/MarioCameraTarget.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/GatewaySpinCheckpoint.hpp"
#include "scene/TitleFileSelectRoute.hpp"

#include <aurora/dvd.h>
#include <aurora/gfx.h>
#include <aurora/main.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    constexpr auto cGatewayCollisionSource =
        std::string_view{"HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl"};

    enum class ShowcaseRoute {
        Title,
        Gateway,
        GatewaySpin,
    };

    struct ShowcaseOptions {
        ShowcaseRoute route = ShowcaseRoute::Title;
        int window_width = 960;
        int window_height = 720;
        std::uint64_t max_frames = 0U;
        std::optional<std::filesystem::path> screenshot_path;
        std::optional<std::uint64_t> screenshot_frame;
        bool exit_after_screenshot = false;
        bool smoke = false;
    };

    enum class TitleShowcaseDisposition : std::uint8_t {
        Exit,
        LaunchGateway,
    };

    struct TitleShowcaseOutcome final {
        TitleShowcaseDisposition disposition =
            TitleShowcaseDisposition::Exit;
        std::optional<smgpc::scene::TitleFileSelectRouteSelection>
            selection{};
        std::unique_ptr<smgpc::compat::GameDataSession> game_data_session{};
    };

    class DvdCloseGuard final {
    public:
        DvdCloseGuard() = default;
        DvdCloseGuard(const DvdCloseGuard&) = delete;
        DvdCloseGuard& operator=(const DvdCloseGuard&) = delete;
        ~DvdCloseGuard() {
            aurora_dvd_close();
        }
    };

    class GatewayMarioOwner final {
    public:
        explicit GatewayMarioOwner(
            smgpc::runtime::PlayerSystemService& player_system)
            : _player_system(&player_system),
              _owned(createNameObj<MarioActor>("MarioActor")) {
            _actor = dynamic_cast<MarioActor*>(_owned.get());
            if (_actor == nullptr) {
                throw std::runtime_error("the typed Gateway MarioActor creator returned the wrong object");
            }
            _player_system->attach_actor(
                *_actor,
                smgpc::runtime::PlayerActorBridge{
                    .set_swing_permission = &GatewayMarioOwner::set_swing_permission,
                    .read_element_mode = &GatewayMarioOwner::read_element_mode,
                    .read_camera_target = &GatewayMarioOwner::read_camera_target,
                });
        }

        GatewayMarioOwner(const GatewayMarioOwner&) = delete;
        GatewayMarioOwner& operator=(const GatewayMarioOwner&) = delete;

        ~GatewayMarioOwner() {
            if (_player_system != nullptr &&
                _player_system->attached_actor() == _actor) {
                _player_system->detach_actor(_actor);
            }
            if (auto* holder = MR::getMarioHolder();
                holder != nullptr && holder->getMarioActor() == _actor) {
                holder->setMarioActor(nullptr);
            }
            _owned.reset();
            _actor = nullptr;
        }

        [[nodiscard]] MarioActor& actor() const {
            return *_actor;
        }

    private:
        static void set_swing_permission(LiveActor& actor, bool permitted) {
            auto* mario = dynamic_cast<MarioActor*>(&actor);
            if (mario == nullptr) {
                throw std::logic_error(
                    "Gateway player entitlement bridge requires MarioActor");
            }
            mario->_EEB = permitted;
        }

        static s32 read_element_mode(const LiveActor& actor) {
            const auto* mario = dynamic_cast<const MarioActor*>(&actor);
            if (mario == nullptr) {
                throw std::logic_error(
                    "Gateway player element-mode bridge requires MarioActor");
            }
            return mario->mPlayerMode;
        }

        static smgpc::camera::StageCameraTargetState read_camera_target(const LiveActor& actor) {
            return smgpc::compat::mario_camera_target(static_cast<const MarioActor&>(actor));
        }

        smgpc::runtime::PlayerSystemService* _player_system = nullptr;
        std::unique_ptr<NameObj> _owned;
        MarioActor* _actor = nullptr;
    };

    [[nodiscard]] std::vector<std::string> copy_arguments(int argc, char* argv[]) {
        auto arguments = std::vector<std::string>{};
        arguments.reserve(argc > 0 ? static_cast<std::size_t>(argc) : 0U);
        for (auto index = 0; index < argc; ++index) {
            arguments.emplace_back(argv[index] != nullptr ? argv[index] : "");
        }
        return arguments;
    }

    [[nodiscard]] std::optional<std::string_view> option_value(std::span<const std::string> arguments,
                                                                std::string_view name) {
        const auto prefix = std::string(name) + "=";
        for (auto index = std::size_t{2U}; index < arguments.size(); ++index) {
            const auto& argument = arguments[index];
            if (argument == name) {
                if (index + 1U >= arguments.size() || arguments[index + 1U].empty()) {
                    throw std::runtime_error(std::string(name) + " requires a value");
                }
                return arguments[index + 1U];
            }
            if (argument.starts_with(prefix)) {
                const auto value = std::string_view(argument).substr(prefix.size());
                if (value.empty()) {
                    throw std::runtime_error(std::string(name) + " requires a value");
                }
                return value;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool has_option(std::span<const std::string> arguments, std::string_view name) {
        for (auto index = std::size_t{2U}; index < arguments.size(); ++index) {
            if (arguments[index] == name) {
                return true;
            }
        }
        return false;
    }

    template <typename Integer>
    [[nodiscard]] Integer parse_integer(std::string_view text, std::string_view option_name) {
        auto value = Integer{};
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
            throw std::runtime_error(std::string(option_name) + " requires an integer");
        }
        return value;
    }

    [[noreturn]] void throw_usage() {
        throw std::runtime_error(
            "usage: smg-pc-showcase <title|gateway|gateway-spin> --disc PATH "
            "[--width N] [--height N] [--max-frames N] [--screenshot PATH] "
            "[--screenshot-frame N] [--exit-after-screenshot] [--smoke (title/gateway)]");
    }

    [[nodiscard]] ShowcaseOptions parse_options(std::span<const std::string> arguments) {
        if (arguments.size() < 2U) {
            throw_usage();
        }

        auto options = ShowcaseOptions{};
        if (arguments[1] == "title") {
            options.route = ShowcaseRoute::Title;
        } else if (arguments[1] == "gateway") {
            options.route = ShowcaseRoute::Gateway;
        } else if (arguments[1] == "gateway-spin") {
            options.route = ShowcaseRoute::GatewaySpin;
        } else {
            throw_usage();
        }

        if (const auto value = option_value(arguments, "--width")) {
            options.window_width = parse_integer<int>(*value, "--width");
        }
        if (const auto value = option_value(arguments, "--height")) {
            options.window_height = parse_integer<int>(*value, "--height");
        }
        if (const auto value = option_value(arguments, "--max-frames")) {
            options.max_frames = parse_integer<std::uint64_t>(*value, "--max-frames");
        }
        if (const auto value = option_value(arguments, "--screenshot")) {
            options.screenshot_path = std::filesystem::path(*value);
        }
        if (const auto value = option_value(arguments, "--screenshot-frame")) {
            options.screenshot_frame = parse_integer<std::uint64_t>(*value, "--screenshot-frame");
        }
        options.exit_after_screenshot = has_option(arguments, "--exit-after-screenshot");
        options.smoke = has_option(arguments, "--smoke");

        if (options.window_width <= 0 || options.window_height <= 0) {
            throw std::runtime_error("showcase window dimensions must be positive");
        }
        if (options.smoke && options.route == ShowcaseRoute::GatewaySpin) {
            throw std::runtime_error("--smoke is available for the title and gateway showcases");
        }
        if (options.smoke && options.max_frames == 0U) {
            options.max_frames = 360U;
        }
        if (options.screenshot_path.has_value() && !options.screenshot_frame.has_value()) {
            options.screenshot_frame = options.route == ShowcaseRoute::Title ? 210U : 30U;
        }
        return options;
    }

    void prepare_screenshot_path(const ShowcaseOptions& options) {
        if (!options.screenshot_path.has_value() || options.screenshot_path->parent_path().empty()) {
            return;
        }
        auto error = std::error_code{};
        std::filesystem::create_directories(options.screenshot_path->parent_path(), error);
        if (error) {
            throw std::runtime_error("could not create screenshot directory: " + error.message());
        }
    }

    [[nodiscard]] bool capture_requested_frame(smgpc::render::AuroraRenderer& renderer,
                                                const ShowcaseOptions& options,
                                                std::uint64_t frame_index,
                                                bool& captured) {
        if (captured || !options.screenshot_path.has_value() || !options.screenshot_frame.has_value() ||
            frame_index < *options.screenshot_frame) {
            return false;
        }
        renderer.request_screenshot_png(*options.screenshot_path);
        captured = true;
        return options.exit_after_screenshot;
    }

    [[nodiscard]] smgpc::app::BootstrapConfiguration bootstrap_configuration(
        const ShowcaseOptions& options, const std::vector<std::string>& arguments) {
        auto window_title = std::string{
            "SMG PC Gateway - WASD walk | arrows camera | F9 freecam | + physics sphere"};
        if (options.route == ShowcaseRoute::Title) {
            window_title = "Super Mario Galaxy PC - retail title showcase";
        } else if (options.route == ShowcaseRoute::GatewaySpin) {
            window_title =
                "SMG PC Gateway spin checkpoint - A/Enter accepts | X checks spin";
        }
        return {
            .window_width = options.window_width,
            .window_height = options.window_height,
            .window_title = std::move(window_title),
            .arguments = arguments,
        };
    }

    [[nodiscard]] TitleShowcaseOutcome run_title_showcase(
        const ShowcaseOptions& options,
        const smgpc::app::BootstrapConfiguration& configuration) {
#ifdef NDEBUG
        if (options.smoke) {
            throw std::runtime_error(
                "Title --smoke requires a debug build for exact sky packet proof");
        }
#endif
        auto logger = smgpc::logging::create_default_logger();
        const auto disc_image = smgpc::app::required_disc_image(configuration);
        if (!aurora_dvd_open(disc_image.string().c_str())) {
            throw std::runtime_error("Aurora could not open disc image " + disc_image.string());
        }
        const auto dvd_guard = DvdCloseGuard{};

        auto window = smgpc::render::AuroraWindow({
            .width = options.window_width,
            .height = options.window_height,
            .title = configuration.window_title,
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto captured = false;
        auto frame_index = std::uint64_t{};
        auto rendered_frames = std::uint64_t{};
        auto gpu_draw_seen = false;
        auto sky_packet_submission_seen = false;
        auto outcome = TitleShowcaseOutcome{};

        {
            auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
            const auto scene_renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            auto route = smgpc::scene::TitleFileSelectRoute(runtime);
            logger->info(smgpc::logging::Category::APP,
                         smgpc::logging::Message{"Showing the exact retail title and retained-sky blank File Select route with original disc resources"});
            logger->info(smgpc::logging::Category::APP,
                         smgpc::logging::Message{"Hold Enter+Backspace at the title prompt; use arrows and a fresh Enter to select a blank file"});

            while (window.poll_events()) {
                auto frame_context = renderer.begin_frame();
                frame_index = frame_context.frame_index;
                {
                    const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
#ifndef NDEBUG
                    if (options.smoke) {
                        runtime.set_j3d_packet_trace_frame(frame_context.frame_index);
                    }
#endif
                    runtime.begin_frame(frame_context);
                    route.update();
                    runtime.draw_3d_normal();
                    runtime.draw_2d_normal();
#ifndef NDEBUG
                    if (options.smoke) {
                        sky_packet_submission_seen =
                            sky_packet_submission_seen ||
                            std::ranges::any_of(
                                runtime.j3d_packet_trace(),
                                [frame_index](const auto& packet) {
                                    return packet.model_name == "CometNearOrbitSky" &&
                                           packet.frame_index == frame_index &&
                                           packet.state.source_triangle_count != 0U &&
                                           packet.state.parsed_display_list_bytes != 0U &&
                                           packet.state.bck_active &&
                                           packet.state.btk_active &&
                                           packet.state.btk_material_count != 0U;
                                });
                    }
#endif
                    ++rendered_frames;
                }
                renderer.end_frame(runtime.wii_video().render_mode());

                if (const auto* stats = aurora_get_stats(); stats != nullptr) {
                    gpu_draw_seen = gpu_draw_seen ||
                                    (stats->drawCallCount != 0U &&
                                     stats->lastVertSize != 0U);
                }

                const auto screenshot_exit = capture_requested_frame(
                    renderer, options, frame_index, captured);
                const auto smoke_complete =
                    options.smoke && rendered_frames >= 2U && gpu_draw_seen &&
                    sky_packet_submission_seen;
                const auto max_frames_reached =
                    options.max_frames != 0U &&
                    frame_index >= options.max_frames;

                // Decide only after end_frame. Explicit session termination
                // always wins over a launch published on the same frame.
                if (screenshot_exit || smoke_complete ||
                    max_frames_reached) {
                    window.close();
                } else if (!options.smoke) {
                    if (const auto launch = route.launch_request();
                        launch.has_value()) {
                        outcome = TitleShowcaseOutcome{
                            .disposition =
                                TitleShowcaseDisposition::LaunchGateway,
                            .selection = launch,
                            .game_data_session = std::make_unique<
                                smgpc::compat::GameDataSession>(
                                static_cast<u16>(launch->file_number)),
                        };
                        logger->info(
                            smgpc::logging::Category::APP,
                            smgpc::logging::Message{
                                "Blank file {} selected; launching ordinary Gateway at the post-castle-rise story boundary after a full title session unwind"},
                            launch->file_number);
                        window.close();
                    }
                }
            }
        }

        if (options.smoke) {
            if (rendered_frames < 2U || !gpu_draw_seen ||
                !sky_packet_submission_seen) {
                throw std::runtime_error(
                    "Title sky smoke proof incomplete: frames=" +
                    std::to_string(rendered_frames) + ";gpu_draw=" +
                    std::to_string(gpu_draw_seen) + ";sky_packets=" +
                    std::to_string(sky_packet_submission_seen));
            }
            logger->info(
                smgpc::logging::Category::APP,
                smgpc::logging::Message{
                    "Title smoke passed: {} rendered frames with exact CometNearOrbitSky BCK/BTK packets before retail title layouts"},
                rendered_frames);
        }
        return outcome;
    }

    [[nodiscard]] float length(const TVec3f& value) {
        return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    }

    [[nodiscard]] float dot(const TVec3f& left, const TVec3f& right) {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    [[nodiscard]] TVec3f scaled(const TVec3f& value, float scale) {
        return TVec3f{value.x * scale, value.y * scale, value.z * scale};
    }

    [[nodiscard]] TVec3f normalized(const TVec3f& value) {
        const auto magnitude = length(value);
        if (magnitude <= 0.000001F) {
            throw std::runtime_error("Gateway showcase camera direction is degenerate");
        }
        return scaled(value, 1.0F / magnitude);
    }

    [[nodiscard]] TVec3f camera_vector(const smgpc::camera::CameraParamVec3& value) {
        return TVec3f{value.x, value.y, value.z};
    }

    struct GatewayPhysicsProbe {
        std::uint64_t id = 0U;
        TVec3f position{};
        TVec3f velocity{};
        float radius = 55.0F;
        std::uint64_t age_frames = 0U;
        std::uint64_t contact_count = 0U;
        std::uint32_t last_triangle = 0U;
        bool gravity_resolved = false;
        bool gravity_changed_velocity = false;
        bool real_kcl_contact_seen = false;
        bool settled = false;
    };

    [[nodiscard]] GatewayPhysicsProbe spawn_gateway_probe(
        std::uint64_t id, const smgpc::camera::CameraPose& camera) {
        const auto eye = camera_vector(camera.eye);
        const auto watch = camera_vector(camera.watch);
        const auto forward = normalized(watch - eye);
        auto probe = GatewayPhysicsProbe{.id = id};
        probe.position = eye + scaled(forward, 300.0F);
        probe.velocity = scaled(forward, 12.0F);
        return probe;
    }

    [[nodiscard]] bool update_gateway_probe(
        GatewayPhysicsProbe& probe, const smgpc::scene::GatewayDemoScene& scene,
        const NameObj& gravity_requester) {
        ++probe.age_frames;
        auto gravity = TVec3f{};
        const auto velocity_before_gravity = probe.velocity;
        probe.gravity_resolved =
            scene.resolve_gravity(gravity_requester, probe.position, &gravity);
        if (probe.gravity_resolved) {
            probe.velocity.add(scaled(gravity, 1.35F));
            probe.gravity_changed_velocity =
                probe.gravity_changed_velocity || length(probe.velocity - velocity_before_gravity) > 0.0001F;
        }

        const auto movement = scene.collision().move_sphere(probe.position, probe.velocity,
                                                            probe.radius, 16U);
        probe.position.add(movement.displacement);
        const auto first_real_contact = !probe.real_kcl_contact_seen;
        for (const auto& contact : movement.contacts) {
            const auto surface = scene.collision().surface(contact.triangle_index);
            if (!surface.has_value() ||
                !std::string_view(surface->source_name).ends_with(cGatewayCollisionSource) ||
                surface->attributes.empty()) {
                throw std::runtime_error(
                    "Gateway development probe contacted a surface without exact planet KCL/PA provenance");
            }
            probe.real_kcl_contact_seen = true;
            probe.last_triangle = contact.triangle_index;
            ++probe.contact_count;

            const auto normal_speed = dot(probe.velocity, contact.normal);
            if (normal_speed < 0.0F) {
                probe.velocity.add(scaled(contact.normal, -1.38F * normal_speed));
            }
        }

        if (!movement.contacts.empty()) {
            probe.velocity = scaled(probe.velocity, 0.985F);
            if (length(probe.velocity) < 0.8F) {
                probe.velocity.set(0.0F, 0.0F, 0.0F);
                probe.settled = true;
            }
        }
        return first_real_contact && probe.real_kcl_contact_seen;
    }

    class DebugSphereRenderer final {
    public:
        explicit DebugSphereRenderer(smgpc::render::AuroraRenderer& renderer) {
            constexpr auto white = std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U};
            _texture = renderer.create_rgba8_texture(1U, 1U, white);
            if (!_texture.is_valid()) {
                throw std::runtime_error("could not create the Gateway debug-sphere texture");
            }

            constexpr auto rings = std::uint16_t{10U};
            constexpr auto segments = std::uint16_t{16U};
            constexpr auto pi = 3.14159265358979323846F;
            _unit_vertices.reserve(static_cast<std::size_t>(rings + 1U) * (segments + 1U));
            for (auto ring = std::uint16_t{}; ring <= rings; ++ring) {
                const auto theta = pi * static_cast<float>(ring) / static_cast<float>(rings);
                const auto y = std::cos(theta);
                const auto radial = std::sin(theta);
                for (auto segment = std::uint16_t{}; segment <= segments; ++segment) {
                    const auto phi = 2.0F * pi * static_cast<float>(segment) /
                                     static_cast<float>(segments);
                    _unit_vertices.push_back({radial * std::cos(phi), y,
                                              radial * std::sin(phi)});
                }
            }
            _indices.reserve(static_cast<std::size_t>(rings) * segments * 6U);
            for (auto ring = std::uint16_t{}; ring < rings; ++ring) {
                for (auto segment = std::uint16_t{}; segment < segments; ++segment) {
                    const auto row = static_cast<std::uint16_t>(segments + 1U);
                    const auto top_left = static_cast<std::uint16_t>(ring * row + segment);
                    const auto top_right = static_cast<std::uint16_t>(top_left + 1U);
                    const auto bottom_left = static_cast<std::uint16_t>(top_left + row);
                    const auto bottom_right = static_cast<std::uint16_t>(bottom_left + 1U);
                    _indices.insert(_indices.end(), {top_left, bottom_left, top_right,
                                                    top_right, bottom_left, bottom_right});
                }
            }
        }

        void draw(smgpc::render::AuroraRenderer& renderer,
                  const smgpc::camera::CameraPose& camera,
                  std::span<const GatewayPhysicsProbe> probes) {
            _vertices.resize(_unit_vertices.size());
            for (const auto& probe : probes) {
                const auto base_color = probe.real_kcl_contact_seen
                                            ? std::array<std::uint8_t, 3U>{80U, 255U, 120U}
                                            : std::array<std::uint8_t, 3U>{80U, 190U, 255U};
                for (auto index = std::size_t{}; index < _unit_vertices.size(); ++index) {
                    const auto& unit = _unit_vertices[index];
                    const auto light = std::clamp(0.58F + 0.42F *
                                                              (unit.x * -0.35F + unit.y * 0.8F +
                                                               unit.z * 0.45F),
                                                  0.25F, 1.0F);
                    _vertices[index] = {
                        .x = probe.position.x + unit.x * probe.radius,
                        .y = probe.position.y + unit.y * probe.radius,
                        .z = probe.position.z + unit.z * probe.radius,
                        .u = 0.0F,
                        .v = 0.0F,
                        .color = {
                            static_cast<std::uint8_t>(static_cast<float>(base_color[0]) * light),
                            static_cast<std::uint8_t>(static_cast<float>(base_color[1]) * light),
                            static_cast<std::uint8_t>(static_cast<float>(base_color[2]) * light),
                            255U,
                        },
                    };
                }
                renderer.submit_textured_triangles_3d(
                    _texture,
                    smgpc::render::TexturedTriangleBatch2D{
                        .vertices = _vertices,
                        .indices = _indices,
                        .blend = false,
                        .depth_test = true,
                        .depth_write = true,
                        .depth_compare = smgpc::render::DepthCompare::LessEqual,
                        .cull_mode = smgpc::render::CullMode::None,
                    },
                    camera);
            }
        }

    private:
        smgpc::render::TextureHandle _texture{};
        std::vector<TVec3f> _unit_vertices{};
        std::vector<std::uint16_t> _indices{};
        std::vector<smgpc::render::TexturedVertex2D> _vertices{};
    };

    [[nodiscard]] int run_gateway_showcase(
        const ShowcaseOptions& options,
        const smgpc::app::BootstrapConfiguration& configuration,
        smgpc::compat::GameDataSession& game_data_session) {
        const auto is_spin_route =
            options.route == ShowcaseRoute::GatewaySpin;
        if (GameDataFunction::getCurrentGameDataHolder() !=
                &game_data_session.holder() ||
            GameDataFunction::getSceneStartGameDataHolder() !=
                &game_data_session.holder() ||
            smgpc::compat::game_data::holder_story_progress(
                game_data_session.holder()) != 5U) {
            throw std::logic_error(
                "Gateway requires one active selected-file session at exact story progress 5");
        }
#ifdef NDEBUG
        if (options.smoke) {
            throw std::runtime_error(
                "Gateway --smoke requires a debug build for Mario packet-trace proof");
        }
#endif
        auto logger = smgpc::logging::create_default_logger();
        const auto disc_image = smgpc::app::required_disc_image(configuration);
        if (!aurora_dvd_open(disc_image.string().c_str())) {
            throw std::runtime_error("Aurora could not open disc image " + disc_image.string());
        }
        const auto dvd_guard = DvdCloseGuard{};
        DVDInit();

        auto window = smgpc::render::AuroraWindow({
            .width = options.window_width,
            .height = options.window_height,
            .title = configuration.window_title,
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto captured = false;
        auto frame_index = std::uint64_t{};
        auto rendered_frames = std::uint64_t{};
        auto gpu_draw_seen = false;
        auto mario_packet_submission_seen = false;
        auto planet_packet_submission_seen = false;
        auto mario_center_on_screen_seen = false;
        auto gravity_velocity_change_seen = false;
        auto real_kcl_contact_seen = false;

        {
            auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
            runtime.set_current_stage_name("HeavensDoorGalaxy");
            // Authored scene visuals initialize and retire native model state
            // inside the Gateway scene lifetime. Keep the renderer binding
            // outside that lifetime; per-frame bindings nest and restore it.
            const auto scene_renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            auto ignored_audio = std::unique_ptr<smgpc::runtime::AudioEventService>{};
            auto ignored_audio_binding =
                std::unique_ptr<smgpc::compat::ScopedAudioEventServiceOverride>{};
            if (is_spin_route) {
                // Audio is outside the current port milestone. Retain exact
                // sound requests as logical events without requiring a host
                // playback device for the playable checkpoint.
                ignored_audio =
                    std::make_unique<smgpc::runtime::AudioEventService>();
                ignored_audio_binding = std::make_unique<
                    smgpc::compat::ScopedAudioEventServiceOverride>(
                    *ignored_audio);
            }
            auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
            const auto resolved_camera =
                smgpc::camera::resolve_stage_start_camera(runtime.dvd(), scene.start_info());
            if (resolved_camera.status !=
                    smgpc::camera::StageStartCameraResolveStatus::Resolved ||
                !resolved_camera.camera.has_value()) {
                throw std::runtime_error(
                    "Gateway exact StartInfo camera could not be resolved: " +
                    resolved_camera.detail);
            }
            const auto initial_camera = resolved_camera.camera->calculation.pose;
            const auto camera_owner = runtime.camera_system().set_authored_game_camera(
                *resolved_camera.camera);
            runtime.set_freecam_enabled(false);

            if (NameObjFactory::getCreator("Mario") != nullptr ||
                NameObjFactory::getCreator("MarioActor") != nullptr) {
                throw std::runtime_error(
                    "the production Mario factory was enabled before the Gateway slice was proven complete");
            }
            auto mario_owner = GatewayMarioOwner{runtime.player_system()};
            auto placement_lease =
                smgpc::scene::GatewayDemoScene::PlacementLease{};
            auto spin_checkpoint =
                std::unique_ptr<smgpc::scene::GatewaySpinCheckpoint>{};
            auto setup_frame = renderer.begin_frame();
            {
                const auto renderer_context =
                    smgpc::render::ScopedAuroraRendererContext(renderer);
                runtime.begin_frame(setup_frame);
                mario_owner.actor().init(scene.player_start_iter());
                placement_lease =
                    scene.finalize_placements(mario_owner.actor());
                if (is_spin_route) {
                    GameDataFunction::followStoryEventByName(
                        "チコガイドデモ終了");
                    if (smgpc::compat::game_data::holder_story_progress(
                            game_data_session.holder()) != 10U) {
                        throw std::logic_error(
                            "the Gateway spin development caller could not advance its selected-file holder from progress 5 to 10");
                    }
                    spin_checkpoint = std::make_unique<
                        smgpc::scene::GatewaySpinCheckpoint>(
                        runtime.dvd(), scene.placements(),
                        scene.general_positions(), game_data_session.holder(),
                        runtime.player_system(),
                        runtime.scene_wipe(), mario_owner.actor());
                }
                runtime.game_layout().activate_game_scene_draw_3d();
            }
            renderer.end_frame(runtime.wii_video().render_mode());

            auto gravity_requester =
                NameObj{"Gateway development physics probes"};
            (void)scene.prove_start_contact(gravity_requester);
            auto sphere_renderer = DebugSphereRenderer(renderer);
            auto *planet_actor = scene.planet();
            auto *planet_model = smgpc::compat::actor_model(planet_actor);
            if (planet_actor == nullptr || planet_model == nullptr) {
                throw std::runtime_error(
                    "the authored Gateway row did not create an ordinary PlanetMap model");
            }
            planet_model->requireLoaded();
            if (!planet_model->isLoaded()) {
                throw std::runtime_error(
                    "the ordinary Gateway PlanetMap did not load its real BDL");
            }
            const auto planet_collision_resources =
                smgpc::compat::actor_collision_parts_resources(planet_actor);
            if (planet_collision_resources.size() != 2U) {
                throw std::runtime_error(
                    "the ordinary Gateway PlanetMap did not retain its main and MoveLimit CollisionParts");
            }

            auto probes = std::vector<GatewayPhysicsProbe>{};
            auto next_probe_id = std::uint64_t{1U};
            if (options.smoke) {
                probes.push_back(spawn_gateway_probe(next_probe_id++, initial_camera));
            }
            auto spawn_key_held = false;
            auto spin_unlock_announced = false;

            if (is_spin_route) {
                logger->info(
                    smgpc::logging::Category::APP,
                    smgpc::logging::Message{
                        "Gateway spin checkpoint: walk into the exact Rosetta trigger to begin the 90-frame handoff and 1670-frame guide prelude; press Enter, Space, or left-click when the spin prompt appears; X tests the unlocked swing request"});
            } else {
                logger->info(
                    smgpc::logging::Category::APP,
                    smgpc::logging::Message{
                        "Gateway Mario: WASD walks; arrows control the game camera; C resets it when permitted; F9 toggles development freecam; =/numpad + spawns a physics sphere; Esc quits"});
            }
            logger->info(
                smgpc::logging::Category::APP,
                smgpc::logging::Message{
                    "Loaded real RMGK01 HeavensDoorMysteriousPlanet through ordinary PlanetMap with {} actor-owned CollisionParts (main KCL/PA {} / {} bytes; MoveLimit KCL/PA {} / {} bytes). Gateway scene total: {} KCL triangles across {} registered meshes; development probes use the mysterious planet's exact point gravity and main KCL"},
                planet_collision_resources.size(),
                planet_collision_resources[0].kcl_size,
                planet_collision_resources[0].attributes_size,
                planet_collision_resources[1].kcl_size,
                planet_collision_resources[1].attributes_size,
                scene.collision().stats().triangle_count,
                scene.collision().stats().mesh_count);
            if (options.smoke) {
                logger->info(smgpc::logging::Category::APP,
                             smgpc::logging::Message{
                                 "Gateway smoke automatically spawned physics probe 1 in front of the game camera"});
            }

            while (window.poll_events()) {
                auto frame_context = renderer.begin_frame();
                frame_index = frame_context.frame_index;
                {
                    const auto renderer_context =
                        smgpc::render::ScopedAuroraRendererContext(renderer);
#ifndef NDEBUG
                    if (options.smoke) {
                        runtime.set_j3d_packet_trace_frame(frame_context.frame_index);
                    }
#endif
                    runtime.camera_system().set_game_camera_target(
                        camera_owner,
                        smgpc::compat::mario_camera_target(mario_owner.actor()));
                    runtime.begin_frame(frame_context);
                    const auto& camera = runtime.scene_camera_pose().value_or(initial_camera);

                    const auto spawn_key =
                        window.is_input_pressed(smgpc::render::InputButton::CORE_PAD_PLUS);
                    if (spawn_key && !spawn_key_held) {
                        if (probes.size() == 64U) {
                            probes.erase(probes.begin());
                        }
                        probes.push_back(spawn_gateway_probe(next_probe_id++, camera));
                        const auto& probe = probes.back();
                        logger->info(
                            smgpc::logging::Category::APP,
                            smgpc::logging::Message{
                                "Spawned Gateway physics probe {} at ({}, {}, {}); active probes={}"},
                            probe.id, probe.position.x, probe.position.y, probe.position.z,
                            probes.size());
                    }
                    spawn_key_held = spawn_key;

                    for (auto& probe : probes) {
                        if (update_gateway_probe(probe, scene, gravity_requester)) {
                            logger->info(
                                smgpc::logging::Category::APP,
                                smgpc::logging::Message{
                                    "Gateway physics probe {} contacted real planet KCL triangle {} after {} frames"},
                                probe.id, probe.last_triangle, probe.age_frames);
                        }
                        gravity_velocity_change_seen =
                            gravity_velocity_change_seen || probe.gravity_changed_velocity;
                        real_kcl_contact_seen =
                            real_kcl_contact_seen || probe.real_kcl_contact_seen;
                    }

                    runtime.draw_3d_normal(camera);
#ifndef NDEBUG
                    if (options.smoke) {
                        mario_packet_submission_seen =
                            mario_packet_submission_seen ||
                            std::ranges::any_of(
                                runtime.j3d_packet_trace(),
                                [frame_index](const auto& packet) {
                                    return packet.model_name == "Mario" &&
                                           packet.frame_index == frame_index &&
                                           packet.state.source_triangle_count != 0U &&
                                           packet.state.parsed_display_list_bytes != 0U &&
                                           packet.state.bck_active &&
                                           packet.state.bck_joint_count != 0U;
                                });
                        planet_packet_submission_seen =
                            planet_packet_submission_seen ||
                            std::ranges::any_of(
                                runtime.j3d_packet_trace(),
                                [frame_index](const auto& packet) {
                                    return packet.model_name ==
                                               "HeavensDoorMysteriousPlanet" &&
                                           packet.frame_index == frame_index &&
                                           packet.state.source_triangle_count != 0U &&
                                           packet.state.parsed_display_list_bytes != 0U;
                                });
                    }
#endif
                    if (options.smoke) {
                        auto mario_center = mario_owner.actor().mPosition;
                        mario_center.add(scaled(mario_owner.actor().mMario->mHeadVec, 60.0F));
                        auto mario_screen = TVec2f{};
                        const auto logical_framebuffer = renderer.logical_framebuffer_size();
                        mario_center_on_screen_seen =
                            mario_center_on_screen_seen ||
                            (MR::calcScreenPosition(&mario_screen, mario_center) &&
                             mario_screen.x >= 0.0F && mario_screen.y >= 0.0F &&
                             mario_screen.x <=
                                 static_cast<float>(logical_framebuffer.width) &&
                             mario_screen.y <=
                                 static_cast<float>(logical_framebuffer.height));
                    }
                    sphere_renderer.draw(renderer, camera, probes);
                    runtime.draw_2d_normal();
                    ++rendered_frames;
                }
                renderer.end_frame(runtime.wii_video().render_mode());

                if (const auto* stats = aurora_get_stats(); stats != nullptr) {
                    gpu_draw_seen = gpu_draw_seen ||
                                    (stats->drawCallCount != 0U && stats->lastVertSize != 0U);
                }
                if (capture_requested_frame(renderer, options, frame_index, captured)) {
                    window.close();
                }
                if (is_spin_route && !spin_unlock_announced &&
                    runtime.player_system().is_swing_permitted()) {
                    spin_unlock_announced = true;
                    logger->info(
                        smgpc::logging::Category::APP,
                        smgpc::logging::Message{
                            "Gateway spin access granted by the exact InformationObserver flow; press X for the retained unlocked swing request (visible spin action remains outside this checkpoint)"});
                }
                if (options.smoke && rendered_frames >= 2U && gpu_draw_seen &&
                    mario_packet_submission_seen && planet_packet_submission_seen &&
                    mario_center_on_screen_seen &&
                    gravity_velocity_change_seen && real_kcl_contact_seen) {
                    window.close();
                }
                if (options.max_frames != 0U && frame_index >= options.max_frames) {
                    window.close();
                }
                if (!probes.empty() && frame_index % 120U == 0U) {
                    const auto contacting = std::ranges::count_if(
                        probes, [](const auto& probe) { return probe.real_kcl_contact_seen; });
                    const auto gravity_active = std::ranges::count_if(
                        probes, [](const auto& probe) { return probe.gravity_resolved; });
                    const auto settled = std::ranges::count_if(
                        probes, [](const auto& probe) { return probe.settled; });
                    logger->info(
                        smgpc::logging::Category::APP,
                        smgpc::logging::Message{
                            "Gateway physics probes: active={}, point-gravity-active={}, real-KCL-contact={}, settled={}"},
                        probes.size(), gravity_active, contacting, settled);
                }
            }
            runtime.camera_system().clear_stage_start_camera(camera_owner);
        }

        if (options.smoke) {
            if (rendered_frames < 2U || !gpu_draw_seen || !mario_packet_submission_seen ||
                !planet_packet_submission_seen || !mario_center_on_screen_seen ||
                !gravity_velocity_change_seen ||
                !real_kcl_contact_seen) {
                throw std::runtime_error(
                    "Gateway smoke proof incomplete: frames=" + std::to_string(rendered_frames) +
                    ";gpu_draw=" + std::to_string(gpu_draw_seen) +
                    ";mario_packets=" +
                    std::to_string(mario_packet_submission_seen) +
                    ";planet_packets=" +
                    std::to_string(planet_packet_submission_seen) +
                    ";mario_center_on_screen=" +
                    std::to_string(mario_center_on_screen_seen) +
                    ";gravity_velocity_change=" +
                    std::to_string(gravity_velocity_change_seen) +
                    ";real_kcl_contact=" + std::to_string(real_kcl_contact_seen));
            }
            logger->info(
                smgpc::logging::Category::APP,
                smgpc::logging::Message{
                    "Gateway smoke passed: {} rendered frames, ordinary PlanetMap and real animated Mario packet submission with an on-screen actor center, GPU draw submission, probe gravity acceleration, and exact planet KCL contact"},
                rendered_frames);
        }
        return 0;
    }

}  // namespace

int main(int argc, char* argv[]) try {
    const auto arguments = copy_arguments(argc, argv);
    const auto options = parse_options(arguments);
    prepare_screenshot_path(options);
    const auto configuration = bootstrap_configuration(options, arguments);
    if (options.route != ShowcaseRoute::Title) {
        auto game_data_session = smgpc::compat::GameDataSession{1U};
        return run_gateway_showcase(options, configuration, game_data_session);
    }

    const auto title_outcome = run_title_showcase(options, configuration);
    if (title_outcome.disposition == TitleShowcaseDisposition::Exit) {
        return 0;
    }
    if (!title_outcome.selection.has_value() ||
        title_outcome.game_data_session == nullptr) {
        throw std::logic_error(
            "the title showcase requested Gateway without a blank-file selection");
    }
    if (title_outcome.game_data_session->selected_file() !=
        static_cast<u16>(title_outcome.selection->file_number)) {
        throw std::logic_error(
            "the title showcase lost the selected file's game-data session identity");
    }

    // The title call has returned, so its route, RuntimeContext, renderer,
    // window and DVD guard are all gone. Start the bounded destination in a
    // completely fresh host session. Title capture/termination controls are
    // one-shot and must not overwrite or prematurely stop the destination.
    auto gateway_options = options;
    gateway_options.route = ShowcaseRoute::Gateway;
    gateway_options.max_frames = 0U;
    gateway_options.screenshot_path.reset();
    gateway_options.screenshot_frame.reset();
    gateway_options.exit_after_screenshot = false;
    gateway_options.smoke = false;
    const auto gateway_configuration =
        bootstrap_configuration(gateway_options, arguments);
    return run_gateway_showcase(gateway_options, gateway_configuration,
                                *title_outcome.game_data_session);
} catch (const std::exception& error) {
    auto logger = smgpc::logging::create_default_logger();
    logger->fatal(smgpc::logging::Category::APP,
                  smgpc::logging::Message{"Showcase failed: {}"}, error.what());
    return 1;
}
