#include "camera/CameraPose.hpp"
#include "render/effects/JpcBillboard.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
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

    void test_default_camera_matches_view_xy_billboard() {
        const auto pose = smgpc::camera::CameraPose{
            .eye = {0.0F, 0.0F, 0.0F},
            .watch = {0.0F, 0.0F, -1.0F},
            .up = {0.0F, 1.0F, 0.0F},
        };
        const auto vertices = smgpc::render::effects::jpc_billboard_world_vertices(
            pose,
            {
                .center = {10.0F, 20.0F, -100.0F},
                .half_size_x = 2.0F,
                .half_size_y = 3.0F,
            });

        require_near(vertices[0U].x, 8.0F, 0.0001F, "default billboard left edge");
        require_near(vertices[0U].y, 17.0F, 0.0001F, "default billboard bottom edge");
        require_near(vertices[2U].x, 12.0F, 0.0001F, "default billboard right edge");
        require_near(vertices[2U].y, 23.0F, 0.0001F, "default billboard top edge");
        for (const auto &vertex : vertices) {
            require_near(vertex.z, -100.0F, 0.0001F, "default billboard must remain on the center view plane");
        }
    }

    void test_translated_rotated_camera_preserves_view_offsets() {
        const auto host_matrix = std::array<float, 12U>{
            0.0F,
            -3.0F,
            0.0F,
            10.0F,
            2.0F,
            0.0F,
            0.0F,
            20.0F,
            0.0F,
            0.0F,
            4.0F,
            30.0F,
        };
        const auto center = smgpc::render::effects::jpc_transform_particle_center(
            host_matrix, {.x = 1.0F, .y = 2.0F, .z = -0.5F});
        require_near(center.x, 4.0F, 0.0001F, "host scale/rotation/translation center X");
        require_near(center.y, 22.0F, 0.0001F, "host scale/rotation/translation center Y");
        require_near(center.z, 28.0F, 0.0001F, "host scale/rotation/translation center Z");

        const auto pose = smgpc::camera::CameraPose{
            .eye = {100.0F, -40.0F, 25.0F},
            .watch = {140.0F, -10.0F, 105.0F},
            .up = {-0.35F, 0.9F, 0.2F},
        };
        const auto center_view = smgpc::camera::transform_world_to_camera(pose, center);
        const auto vertices = smgpc::render::effects::jpc_billboard_world_vertices(
            pose,
            {
                .center = center,
                .half_size_x = 12.5F,
                .half_size_y = 7.0F,
            });
        constexpr auto expected_x = std::array<float, 4U>{-12.5F, 12.5F, 12.5F, -12.5F};
        constexpr auto expected_y = std::array<float, 4U>{-7.0F, -7.0F, 7.0F, 7.0F};
        for (auto index = std::size_t{}; index < vertices.size(); ++index) {
            const auto view = smgpc::camera::transform_world_to_camera(
                pose, {.x = vertices[index].x, .y = vertices[index].y, .z = vertices[index].z});
            require_near(view.x - center_view.x, expected_x[index], 0.0001F,
                         "translated/rotated billboard view X");
            require_near(view.y - center_view.y, expected_y[index], 0.0001F,
                         "translated/rotated billboard view Y");
            require_near(view.z, center_view.z, 0.0001F,
                         "translated/rotated billboard vertices share center depth");
        }
    }

    void test_rotated_billboard_matches_oracle_matrix() {
        constexpr auto cHalfPi = 1.57079632679489661923F;
        const auto pose = smgpc::camera::CameraPose{
            .eye = {0.0F, 0.0F, 0.0F},
            .watch = {0.0F, 0.0F, -1.0F},
            .up = {0.0F, 1.0F, 0.0F},
        };
        const auto vertices = smgpc::render::effects::jpc_billboard_world_vertices(
            pose,
            {
                .center = {0.0F, 0.0F, -50.0F},
                .half_size_x = 2.0F,
                .half_size_y = 5.0F,
                .rotation_radians = cHalfPi,
            });

        require_near(vertices[0U].x, 5.0F, 0.0001F, "rotated oracle matrix X");
        require_near(vertices[0U].y, -2.0F, 0.0001F, "rotated oracle matrix Y");
        require_near(vertices[2U].x, -5.0F, 0.0001F, "rotated oracle opposite X");
        require_near(vertices[2U].y, 2.0F, 0.0001F, "rotated oracle opposite Y");
    }

    void test_packet_path_only_selects_implemented_billboards() {
        using smgpc::render::effects::jpc_particle_packet_path;
        using smgpc::render::effects::JpcParticlePacketPath;
        require(jpc_particle_packet_path(false, 2U) ==
                    std::optional{JpcParticlePacketPath::ScreenSpace},
                "2D draw groups must retain screen-space effects");
        require(jpc_particle_packet_path(true, 2U) ==
                    std::optional{JpcParticlePacketPath::WorldBillboard},
                "3D parent billboard packets must use the world camera");
        for (auto shape_type = std::uint8_t{}; shape_type < 16U; ++shape_type) {
            if (shape_type == 2U) {
                continue;
            }
            require(!jpc_particle_packet_path(false, shape_type).has_value(),
                    "unimplemented shape must not produce 2D geometry");
            require(!jpc_particle_packet_path(true, shape_type).has_value(),
                    "unimplemented shape must not produce 3D geometry");
        }
    }

    void test_duplicate_names_keep_distinct_effect_hosts() {
        auto service = smgpc::runtime::EffectService{};
        auto first_host = 1;
        auto second_host = 2;
        const auto first_matrix = std::array<float, 12U>{
            1.0F,
            0.0F,
            0.0F,
            100.0F,
            0.0F,
            1.0F,
            0.0F,
            200.0F,
            0.0F,
            0.0F,
            1.0F,
            300.0F,
        };
        const auto second_matrix = std::array<float, 12U>{
            1.0F,
            0.0F,
            0.0F,
            -400.0F,
            0.0F,
            1.0F,
            0.0F,
            500.0F,
            0.0F,
            0.0F,
            1.0F,
            -600.0F,
        };

        service.register_keeper(smgpc::runtime::EffectKeeperHostKind::LiveActor, "Steam", 1, "Steam", false,
                                &first_host);
        service.register_keeper(smgpc::runtime::EffectKeeperHostKind::LiveActor, "Steam", 1, "Steam", false,
                                &second_host);
        service.bind_host_transform(smgpc::runtime::EffectKeeperHostKind::LiveActor, "Steam",
                                    smgpc::runtime::EffectHostBindingSource::LiveActorBaseMatrix,
                                    first_matrix, false, &first_host);
        service.bind_host_transform(smgpc::runtime::EffectKeeperHostKind::LiveActor, "Steam",
                                    smgpc::runtime::EffectHostBindingSource::LiveActorBaseMatrix,
                                    second_matrix, false, &second_host);
        service.emit("Steam", "Steam", &first_host);
        service.emit("Steam", "Steam", &second_host);

        const auto active = service.active_effect_instances();
        require(active.size() == 2U, "same-name actors must create independent active effect instances");
        require(active[0U].host_identity != active[1U].host_identity,
                "same-name active effects must retain distinct host identities");
        require_near(active[0U].host_binding->translation[0U], 100.0F, 0.0001F,
                     "first same-name host translation");
        require_near(active[1U].host_binding->translation[0U], -400.0F, 0.0001F,
                     "second same-name host translation");

        service.unregister_keeper("Steam", &first_host);
        require(service.active_effect_instances().size() == 1U,
                "unregistering one host must preserve the other same-name actor's effect");
        require(service.active_effect_instances().front().host_identity == &second_host,
                "per-actor effect teardown must target identity rather than name");
    }

}  // namespace

int main() {
    try {
        const auto tests = std::array{
            std::pair{"default camera matches view XY billboard", &test_default_camera_matches_view_xy_billboard},
            std::pair{"translated rotated camera preserves view offsets", &test_translated_rotated_camera_preserves_view_offsets},
            std::pair{"rotated billboard matches oracle matrix", &test_rotated_billboard_matches_oracle_matrix},
            std::pair{"packet path only selects implemented billboards", &test_packet_path_only_selects_implemented_billboards},
            std::pair{"duplicate names keep distinct effect hosts", &test_duplicate_names_keep_distinct_effect_hosts},
        };
        for (const auto &[name, test] : tests) {
            test();
            std::cout << "[ok] " << name << '\n';
        }
        std::cout << tests.size() << " JPC billboard test(s) passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[failed] " << error.what() << '\n';
        return 1;
    }
}
