#include "Application.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "render/J3dModelRenderer.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"

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
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    constexpr auto cGatewayCollisionSource =
        std::string_view{"HeavensDoorMysteriousPlanet.arc/heavensdoormysteriousplanet.kcl"};

    enum class ShowcaseRoute {
        Title,
        Gateway,
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

    class DvdCloseGuard final {
    public:
        DvdCloseGuard() = default;
        DvdCloseGuard(const DvdCloseGuard&) = delete;
        DvdCloseGuard& operator=(const DvdCloseGuard&) = delete;
        ~DvdCloseGuard() {
            aurora_dvd_close();
        }
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
            "usage: smg-pc-showcase <title|gateway> --disc PATH "
            "[--width N] [--height N] [--max-frames N] [--screenshot PATH] "
            "[--screenshot-frame N] [--exit-after-screenshot] [--smoke (gateway only)]");
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
        if (options.smoke && options.route != ShowcaseRoute::Gateway) {
            throw std::runtime_error("--smoke is only available for the gateway showcase");
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
        return {
            .window_width = options.window_width,
            .window_height = options.window_height,
            .window_title = options.route == ShowcaseRoute::Title
                                ? "Super Mario Galaxy PC - retail title showcase"
                                : "SMG PC Gateway - F9 mouse | WASD fly | + physics sphere",
            .arguments = arguments,
        };
    }

    [[nodiscard]] int run_title_showcase(const ShowcaseOptions& options,
                                         const smgpc::app::BootstrapConfiguration& configuration) {
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

        {
            auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
            auto title_sequence = TitleSequenceProduct{};
            title_sequence.appear();
            logger->info(smgpc::logging::Category::APP,
                         smgpc::logging::Message{"Showing the exact retail TitleSequenceProduct with original disc resources"});
            logger->info(smgpc::logging::Category::APP,
                         smgpc::logging::Message{"Hold keyboard A+B or Enter+Backspace when prompted"});

            while (window.poll_events()) {
                auto frame_context = renderer.begin_frame();
                frame_index = frame_context.frame_index;
                {
                    const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
                    runtime.begin_frame(frame_context);
                    title_sequence.updateNerve();
                    runtime.draw_2d_normal();
                }
                renderer.end_frame();

                if (capture_requested_frame(renderer, options, frame_index, captured)) {
                    window.close();
                }
                if (!title_sequence.isActive()) {
                    logger->info(smgpc::logging::Category::APP,
                                 smgpc::logging::Message{"Retail title component completed at frame {}"}, frame_index);
                    window.close();
                }
                if (options.max_frames != 0U && frame_index >= options.max_frames) {
                    window.close();
                }
            }
        }
        return 0;
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

    [[nodiscard]] smgpc::camera::CameraParamVec3 camera_vector(const TVec3f& value) {
        return {.x = value.x, .y = value.y, .z = value.z};
    }

    [[nodiscard]] smgpc::camera::CameraPose gateway_camera(
        const smgpc::scene::GatewayDemoStartContact& contact) {
        auto side = TVec3f{-contact.gravity.z, 0.0F, contact.gravity.x};
        if (length(side) <= 0.0001F) {
            side.set(1.0F, 0.0F, 0.0F);
        }
        side = normalized(side);

        auto eye = contact.collision.position;
        eye.add(scaled(contact.gravity, -900.0F));
        eye.add(scaled(side, 240.0F));
        return {
            .eye = camera_vector(eye),
            .watch = camera_vector(contact.collision.position),
            .up = {0.0F, 1.0F, 0.0F},
            .fovy_degrees = 55.0F,
            .aspect_ratio = 608.0F / 456.0F,
            .near_clip = 5.0F,
            .far_clip = 800000.0F,
        };
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
            if (!surface.has_value() || surface->source_name != cGatewayCollisionSource ||
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
        const smgpc::app::BootstrapConfiguration& configuration) {
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
        auto gravity_velocity_change_seen = false;
        auto real_kcl_contact_seen = false;

        {
            auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
            runtime.set_current_stage_name("HeavensDoorGalaxy");
            auto scene = smgpc::scene::GatewayDemoScene(runtime.dvd());
            auto gravity_requester = NameObj{"Gateway development physics probes"};
            const auto start_contact = scene.prove_start_contact(gravity_requester);
            const auto initial_camera = gateway_camera(start_contact);
            runtime.camera_system().set_game_camera_pose(initial_camera);
            runtime.set_freecam_enabled(true);

            auto planet_model = smgpc::render::J3dModelRenderer{};
            auto sphere_renderer = DebugSphereRenderer(renderer);
            {
                const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
                planet_model.load(renderer, scene.planet_bdl());
            }
            if (!planet_model.is_loaded() || planet_model.mesh_count() == 0U ||
                planet_model.render_packets().empty()) {
                throw std::runtime_error(
                    "the real Gateway planet BDL did not produce renderable J3D packets");
            }
            const auto planet_matrix = smgpc::render::J3dMatrix3x4{
                smgpc::scene::stage_collision_matrix(scene.planet_placement())};

            auto probes = std::vector<GatewayPhysicsProbe>{};
            auto next_probe_id = std::uint64_t{1U};
            if (options.smoke) {
                probes.push_back(spawn_gateway_probe(next_probe_id++, initial_camera));
            }
            auto spawn_key_held = false;

            logger->info(
                smgpc::logging::Category::APP,
                smgpc::logging::Message{
                    "Gateway freecam: mouse look; WASD move; Space/LeftShift rise/fall; F9 release/toggle mouse; =/numpad + spawn a physics sphere; Esc quit"});
            logger->info(
                smgpc::logging::Category::APP,
                smgpc::logging::Message{
                    "Loaded real RMGK01 HeavensDoorMysteriousPlanet: {} J3D meshes, {} KCL triangles; development probes use its exact point gravity and KCL"},
                planet_model.mesh_count(), scene.collision().stats().triangle_count);
            if (options.smoke) {
                logger->info(smgpc::logging::Category::APP,
                             smgpc::logging::Message{
                                 "Gateway smoke automatically spawned physics probe 1 in front of the free camera"});
            }

            while (window.poll_events()) {
                auto frame_context = renderer.begin_frame();
                frame_index = frame_context.frame_index;
                {
                    const auto renderer_context =
                        smgpc::render::ScopedAuroraRendererContext(renderer);
                    runtime.begin_frame(frame_context);
                    const auto& camera = runtime.scene_camera_pose().value_or(initial_camera);
                    // Retain the latest development camera pose when F9 releases
                    // and later re-captures the mouse.
                    runtime.camera_system().set_game_camera_pose(camera);

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

                    planet_model.draw(
                        renderer, camera, planet_matrix, frame_index,
                        smgpc::render::J3dModelRendererDrawOptions{
                            .translucent_filter = false,
                        });
                    sphere_renderer.draw(renderer, camera, probes);
                    planet_model.draw(
                        renderer, camera, planet_matrix, frame_index,
                        smgpc::render::J3dModelRendererDrawOptions{
                            .translucent_filter = true,
                        });
                    ++rendered_frames;
                }
                renderer.end_frame();

                if (const auto* stats = aurora_get_stats(); stats != nullptr) {
                    gpu_draw_seen = gpu_draw_seen ||
                                    (stats->drawCallCount != 0U && stats->lastVertSize != 0U);
                }
                if (capture_requested_frame(renderer, options, frame_index, captured)) {
                    window.close();
                }
                if (options.smoke && rendered_frames >= 2U && gpu_draw_seen &&
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
        }

        if (options.smoke) {
            if (rendered_frames < 2U || !gpu_draw_seen || !gravity_velocity_change_seen ||
                !real_kcl_contact_seen) {
                throw std::runtime_error(
                    "Gateway smoke proof incomplete: frames=" + std::to_string(rendered_frames) +
                    ";gpu_draw=" + std::to_string(gpu_draw_seen) +
                    ";gravity_velocity_change=" +
                    std::to_string(gravity_velocity_change_seen) +
                    ";real_kcl_contact=" + std::to_string(real_kcl_contact_seen));
            }
            logger->info(
                smgpc::logging::Category::APP,
                smgpc::logging::Message{
                    "Gateway smoke passed: {} rendered frames, GPU draw submission, probe gravity acceleration, and exact planet KCL contact"},
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
    if (options.route == ShowcaseRoute::Gateway) {
        return run_gateway_showcase(options, configuration);
    }
    return run_title_showcase(options, configuration);
} catch (const std::exception& error) {
    auto logger = smgpc::logging::create_default_logger();
    logger->fatal(smgpc::logging::Category::APP,
                  smgpc::logging::Message{"Showcase failed: {}"}, error.what());
    return 1;
}
