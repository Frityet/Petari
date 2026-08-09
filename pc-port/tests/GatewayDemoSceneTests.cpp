#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PointGravity.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/StageHostScene.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
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

    void require_near(float actual, float expected, float tolerance,
                      std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ";actual=" +
                                     std::to_string(actual) + ";expected=" +
                                     std::to_string(expected));
        }
    }

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error, "the Gateway demo proof requires a readable working directory");
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
            "the Gateway demo proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] float dot(const TVec3f &left, const TVec3f &right) {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    [[nodiscard]] std::size_t model_packet_count(
        const smgpc::render::J3dModelGeometry &geometry) {
        auto count = std::size_t{};
        for (const auto &shape : geometry.shapes) {
            count += shape.draw_packets.size();
        }
        return count;
    }

    [[nodiscard]] std::size_t model_triangle_count(
        const smgpc::render::J3dModelGeometry &geometry) {
        auto count = std::size_t{};
        for (const auto &shape : geometry.shapes) {
            for (const auto &packet : shape.draw_packets) {
                count += packet.indices.size() / 3U;
            }
        }
        return count;
    }

    void test_real_gateway_spawn_planet_collision_and_gravity() {
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

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        auto scene = smgpc::scene::GatewayDemoScene{dvd};

        const auto &start = scene.start_info();
        require(start.object_name == "Mario" && start.start_id == 0 && start.zone_id == 0 &&
                    start.camera_id == 78 && start.layer_name == "layera" &&
                    start.table_path == "jmp/start/layera/startinfo",
                "the development scene must retain the exact scenario-1 StartInfo identity");
        require_near(start.world_position[0], 14459.978515625F, 0.0005F,
                     "scenario-1 StartInfo X");
        require_near(start.world_position[1], -12791.11328125F, 0.0005F,
                     "scenario-1 StartInfo Y");
        require_near(start.world_position[2], 6059.91162109375F, 0.0005F,
                     "scenario-1 StartInfo Z");
        require_near(start.local_rotation[0], 124.76948547363281F, 0.0005F,
                     "scenario-1 StartInfo rotation X");
        require_near(start.local_rotation[1], -114.13859558105469F, 0.0005F,
                     "scenario-1 StartInfo rotation Y");
        require_near(start.local_rotation[2], 47.654739379882812F, 0.0005F,
                     "scenario-1 StartInfo rotation Z");

        const auto start_iter = scene.player_start_iter();
        const char *start_name = nullptr;
        auto start_position = TVec3f{};
        require(start_iter.isValid() && start_iter.getValue("name", &start_name) &&
                    start_name != nullptr && std::string_view(start_name) == "Mario" &&
                    start_iter.getValue("pos_x", &start_position.x) &&
                    start_iter.getValue("pos_y", &start_position.y) &&
                    start_iter.getValue("pos_z", &start_position.z) &&
                    start_position.epsilonEquals(
                        TVec3f{14459.978515625F, -12791.11328125F, 6059.91162109375F},
                        0.0005F),
                "the forthcoming real MarioActor must receive the retained exact JMap row");

        const auto packets = model_packet_count(scene.planet_geometry());
        const auto triangles = model_triangle_count(scene.planet_geometry());
        require(!scene.planet_bdl().empty() && packets != 0U && triangles != 0U,
                "HeavensDoorMysteriousPlanet.bdl must parse into real draw packets");
        const auto attributes = smgpc::resource::BcsvTable::from_bytes(scene.planet_pa());
        require(!scene.planet_kcl().empty() && attributes.entry_count() != 0U &&
                    scene.collision().stats().mesh_count == 1U &&
                    scene.collision().stats().triangle_count != 0U,
                "the exact planet KCL and PA must form the development collision surface");

        const auto *point_gravity = dynamic_cast<const PointGravity *>(&scene.gravity());
        require(point_gravity != nullptr,
                "the child-zone gravity must be the exact Game PointGravity implementation");
        require_near(point_gravity->mTranslation.x, 14760.0F, 0.001F,
                     "child-zone point-gravity center X");
        require_near(point_gravity->mTranslation.y, -10676.2255859375F, 0.001F,
                     "child-zone point-gravity center Y");
        require_near(point_gravity->mTranslation.z, 6770.0F, 0.001F,
                     "child-zone point-gravity center Z");
        require_near(point_gravity->mRange, 8200.0F, 0.001F,
                     "child-zone point-gravity range");

        auto requester = NameObj{"Gateway Mario acceptance requester"};
        auto info = GravityInfo{};
        auto resolved_gravity = TVec3f{};
        require(scene.resolve_gravity(requester, start_position, &resolved_gravity, &info) &&
                    info.mGravityInstance == &scene.gravity(),
                "the exact child-zone point gravity must win at Mario's start position");
        auto expected_gravity =
            point_gravity->mTranslation - start_position;
        expected_gravity.scale(1.0F / expected_gravity.length());
        require(resolved_gravity.epsilonEquals(expected_gravity, 0.0001F),
                "resolved Gateway gravity must point toward the exact child-zone center");

        const auto contact = scene.prove_start_contact(requester);
        require(contact.surface.source_name ==
                        "HeavensDoorMysteriousPlanet.arc/heavensdoormysteriousplanet.kcl" &&
                    contact.surface.attributes.size() == scene.planet_pa().size() &&
                    contact.collision.attribute < attributes.entry_count(),
                "the start contact must retain exact KCL and PA provenance");
        require(contact.separation < 0.1F,
                "scenario-1 StartInfo must rest on the real planet KCL face");
        require(dot(contact.collision.normal, contact.gravity) < -0.95F,
                "the contacted planet face must oppose the resolved inward gravity");

        // This focused development surface must not silently turn an archive
        // model into a production NameObj creator. StageHost preflight remains
        // strict until the original planet actor closure exists.
        require(!scene.planet_placement().factory_supported,
                "the development scene must not mark the planet as production-factory supported");
        auto strict_preflight_rejected = false;
        try {
            smgpc::scene::preflight_stage_placements_or_throw(
                "HeavensDoorGalaxy", 1,
                std::span<const smgpc::scene::StagePlacementObject>{
                    &scene.planet_placement(), 1U});
        } catch (const std::runtime_error &) {
            strict_preflight_rejected = true;
        }
        require(strict_preflight_rejected,
                "strict StageHost preflight must remain unchanged by the development scene");

        std::cout << "[proof] disc=" << disc_path.string()
                  << "; start=(" << start_position.x << ',' << start_position.y << ','
                  << start_position.z << ")"
                  << "; planet_bdl_bytes=" << scene.planet_bdl().size()
                  << "; model_packets=" << packets << "; model_triangles=" << triangles
                  << "; kcl_bytes=" << scene.planet_kcl().size()
                  << "; kcl_triangles=" << scene.collision().stats().triangle_count
                  << "; pa_bytes=" << scene.planet_pa().size()
                  << "; gravity_center=(" << point_gravity->mTranslation.x << ','
                  << point_gravity->mTranslation.y << ','
                  << point_gravity->mTranslation.z << ')'
                  << "; start_surface_separation=" << contact.separation << '\n';
    }

}  // namespace

int main() {
    try {
        test_real_gateway_spawn_planet_collision_and_gravity();
        std::cout << "[ok] exact Gateway development scene is ready for real MarioActor\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] Gateway development scene: " << error.what() << '\n';
        return 1;
    }
}
