#include "render/J3dAnimation.hpp"
#include "resource/RarcArchive.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Operation>
    void require_runtime_error(Operation operation, std::string_view message) {
        auto rejected = false;
        try {
            operation();
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        require(rejected, message);
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_object_archive(std::string_view object_name) {
        const auto archive_name = std::string(object_name) + ".arc";
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::filesystem::path candidates[]{
                root / "orig/RMGK02/files/ObjectData" / archive_name,
                root / "orig/RMGK01/files/ObjectData" / archive_name,
                root / "container/orig/RMGK02/files/ObjectData" / archive_name,
                root / "container/orig/RMGK01/files/ObjectData" / archive_name,
                root / "pc-port/container/orig/RMGK02/files/ObjectData" / archive_name,
                root / "pc-port/container/orig/RMGK01/files/ObjectData" / archive_name,
            };
            for (const auto &candidate : candidates) {
                auto error = std::error_code{};
                if (std::filesystem::is_regular_file(candidate, error)) {
                    return candidate;
                }
            }
            if (root == root.root_path()) {
                break;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> bytes, std::size_t offset) {
        require(offset + 2U <= bytes.size(), "test mutation read is outside the BRK");
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) |
                                          static_cast<std::uint16_t>(bytes[offset + 1U]));
    }

    [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> bytes, std::size_t offset) {
        require(offset + 4U <= bytes.size(), "test mutation read is outside the BRK");
        return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
               static_cast<std::uint32_t>(bytes[offset + 3U]);
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        require(offset + 2U <= bytes.size(), "test mutation write is outside the BRK");
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] smgpc::render::J3dBrkAnimationSummary
    parse_brk(const smgpc::resource::RarcArchive &archive, std::string_view animation_name) {
        const auto *entry = archive.find_by_basename(animation_name);
        require(entry != nullptr, "retail object archive is missing its expected BRK");
        const auto animation = smgpc::render::inspect_j3d_animation(archive.file_data(*entry));
        require(animation.type == "brk1" && animation.brk.has_value(),
                "retail .brk must parse as a TRK1 animation");
        return *animation.brk;
    }

    void require_component_sizes(const std::array<std::vector<std::int16_t>, 4U> &values,
                                 const std::array<std::size_t, 4U> &expected,
                                 std::string_view message) {
        for (auto channel = std::size_t{}; channel < values.size(); ++channel) {
            require(values[channel].size() == expected[channel], message);
        }
    }

    void require_register_track(const smgpc::render::J3dBrkAnimationSummary::RegisterTrack &track,
                                std::string_view material_name, std::uint16_t material_id,
                                std::uint8_t register_id, std::uint16_t alpha_key_count,
                                std::uint16_t alpha_value_offset) {
        require(track.material_name == material_name && track.stored_material_id == material_id &&
                    track.register_id == register_id,
                "retail TRK1 material/register binding does not match the authored track");
        for (auto channel = std::size_t{}; channel < 3U; ++channel) {
            require(track.channels[channel].max_frame == 1U &&
                        track.channels[channel].offset == 0U &&
                        track.channels[channel].type == 1U,
                    "retail TRK1 static RGB channel metadata changed");
        }
        require(track.channels[3U].max_frame == alpha_key_count &&
                    track.channels[3U].offset == alpha_value_offset &&
                    track.channels[3U].type == 1U,
                "retail TRK1 alpha channel metadata changed");
    }

    void test_sphere_air_appear_disappear(const smgpc::resource::RarcArchive &archive) {
        const auto appear = parse_brk(archive, "appear.brk");
        const auto disappear = parse_brk(archive, "disappear.brk");
        for (const auto *brk : {&appear, &disappear}) {
            require(brk->attribute == 0U && brk->frame_max == 59,
                    "SphereAir visibility BRK must retain its one-shot 59-frame header");
            require(brk->color_tracks.size() == 1U && brk->konst_tracks.empty(),
                    "SphereAir visibility BRK must animate one TEV color register only");
            require_component_sizes(brk->color_values, {1U, 1U, 1U, 8U},
                                    "SphereAir color-component table sizes changed");
            require_register_track(brk->color_tracks.front(), "AirMat", 0U, 0U, 2U, 0U);
        }

        require(smgpc::render::j3d_evaluate_brk_color_track(appear, 0U, 0.0F) ==
                        std::array<std::int16_t, 4U>{255, 255, 255, -100} &&
                    smgpc::render::j3d_evaluate_brk_color_track(appear, 0U, 30.0F) ==
                        std::array<std::int16_t, 4U>{255, 255, 255, -50} &&
                    smgpc::render::j3d_evaluate_brk_color_track(appear, 0U, 60.0F) ==
                        std::array<std::int16_t, 4U>{255, 255, 255, 0},
                "SphereAir Appear must evaluate the authored raw TEV-color curve");
        require(smgpc::render::j3d_evaluate_brk_color_track(disappear, 0U, 0.0F) ==
                        std::array<std::int16_t, 4U>{255, 255, 255, 0} &&
                    smgpc::render::j3d_evaluate_brk_color_track(disappear, 0U, 30.0F) ==
                        std::array<std::int16_t, 4U>{255, 255, 255, -50} &&
                    smgpc::render::j3d_evaluate_brk_color_track(disappear, 0U, 60.0F) ==
                        std::array<std::int16_t, 4U>{255, 255, 255, -100},
                "SphereAir Disappear must evaluate the authored raw TEV-color curve");
    }

    void test_lens_flare(const smgpc::resource::RarcArchive &archive) {
        const auto brk = parse_brk(archive, "lensflare.brk");
        require(brk.attribute == 2U && brk.frame_max == 100,
                "LensFlare BRK must retain its authored repeat attribute and 100-frame header");
        require(brk.color_tracks.empty() && brk.konst_tracks.size() == 1U,
                "LensFlare BRK must animate exactly one konst register");
        require_component_sizes(brk.konst_values, {1U, 1U, 1U, 8U},
                                "LensFlare konst-component table sizes changed");
        require_register_track(brk.konst_tracks.front(), "LensFleareMat", 0U, 0U, 2U, 0U);

        require(smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 0.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 40U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 50.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 20U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 100.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 0U},
                "LensFlare must evaluate its authored raw konst-alpha curve");

        const auto wrapped_frame = smgpc::render::j3d_animation_frame(
            brk.attribute, brk.frame_max, 100.0F);
        require(wrapped_frame == 0.0F &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, wrapped_frame)[3U] == 40U &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 100.0F)[3U] == 0U,
                "LensFlare raw frame 100 must hold the end key instead of wrapping attribute 2 to frame zero");
    }

    void test_glare_glow(const smgpc::resource::RarcArchive &archive) {
        const auto brk = parse_brk(archive, "glareglow.brk");
        require(brk.attribute == 0U && brk.frame_max == 119,
                "GlareGlow BRK must retain its one-shot 119-frame header");
        require(brk.color_tracks.empty() && brk.konst_tracks.size() == 2U,
                "GlareGlow BRK must animate exactly two konst registers");
        require_component_sizes(brk.konst_values, {1U, 1U, 1U, 24U},
                                "GlareGlow konst-component table sizes changed");
        require_register_track(brk.konst_tracks[0U], "pasted__heatwave", 0U, 0U, 3U, 0U);
        require_register_track(brk.konst_tracks[1U], "pasted__heatwave(2)", 1U, 0U, 3U, 12U);

        require(smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 0.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 125U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 59.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 125U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 0U, 120.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 0U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 1U, 0.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 50U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 1U, 59.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 50U} &&
                    smgpc::render::j3d_evaluate_brk_konst_track(brk, 1U, 120.0F) ==
                        std::array<std::uint8_t, 4U>{255U, 255U, 255U, 0U},
                "GlareGlow must evaluate both authored raw konst-alpha curves");
    }

    void test_glare_line(const smgpc::resource::RarcArchive &archive) {
        const auto brk = parse_brk(archive, "glareline.brk");
        require(brk.attribute == 2U && brk.frame_max == 120,
                "GlareLine BRK must retain its repeat attribute and 120-frame header");
        require(brk.color_tracks.empty() && brk.konst_tracks.size() == 3U,
                "GlareLine BRK must animate exactly three konst registers");
        require_component_sizes(brk.konst_values, {1U, 1U, 1U, 12U},
                                "GlareLine konst-component table sizes changed");
        constexpr std::array<std::string_view, 3U> material_names{
            "glare_b_03", "glare_b_03(2)", "glare_b_03(3)"};
        for (auto track_index = std::size_t{}; track_index < material_names.size(); ++track_index) {
            require_register_track(brk.konst_tracks[track_index], material_names[track_index],
                                   static_cast<std::uint16_t>(track_index), 0U, 3U, 0U);
            require(smgpc::render::j3d_evaluate_brk_konst_track(brk, track_index, 0.0F) ==
                            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 145U} &&
                        smgpc::render::j3d_evaluate_brk_konst_track(brk, track_index, 59.0F) ==
                            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 145U} &&
                        smgpc::render::j3d_evaluate_brk_konst_track(brk, track_index, 120.0F) ==
                            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 0U},
                    "GlareLine must evaluate every authored raw konst-alpha curve");
        }
    }

    void test_malformed_trk1_is_rejected(const smgpc::resource::RarcArchive &archive) {
        const auto *entry = archive.find_by_basename("lensflare.brk");
        require(entry != nullptr, "retail LensFlare archive is missing lensflare.brk");
        const auto source = archive.file_data(*entry);
        constexpr auto section_offset = std::size_t{0x20U};
        const auto konst_table_offset = section_offset +
                                        read_be32(source, section_offset + 0x24U);
        const auto konst_names_offset = section_offset +
                                        read_be32(source, section_offset + 0x34U);

        auto bad_bounds = std::vector<std::uint8_t>(source.begin(), source.end());
        write_be16(bad_bounds, konst_table_offset + 3U * 6U + 2U, 0xffffU);
        require_runtime_error(
            [&] { static_cast<void>(smgpc::render::inspect_j3d_animation(bad_bounds)); },
            "TRK1 must reject a keyframe channel outside its component table");

        auto bad_name = std::vector<std::uint8_t>(source.begin(), source.end());
        const auto first_name_relative = read_be16(source, konst_names_offset + 6U);
        bad_name[konst_names_offset + first_name_relative] = 0U;
        require_runtime_error(
            [&] { static_cast<void>(smgpc::render::inspect_j3d_animation(bad_name)); },
            "TRK1 must reject an empty material name");

        auto bad_register = std::vector<std::uint8_t>(source.begin(), source.end());
        bad_register[konst_table_offset + 0x18U] = 4U;
        require_runtime_error(
            [&] { static_cast<void>(smgpc::render::inspect_j3d_animation(bad_register)); },
            "TRK1 must reject an out-of-range konst register index");
    }
}  // namespace

int main() {
    auto passed = 0U;

    const auto sphere_air_path = find_object_archive("SphereAir");
    if (!sphere_air_path.has_value()) {
        std::cout << "[skip] retail SphereAir BRK checks\n";
    } else {
        test_sphere_air_appear_disappear(
            smgpc::resource::RarcArchive::from_file(*sphere_air_path));
        ++passed;
    }

    const auto lens_flare_path = find_object_archive("LensFlare");
    if (!lens_flare_path.has_value()) {
        std::cout << "[skip] retail LensFlare BRK checks\n";
    } else {
        const auto archive = smgpc::resource::RarcArchive::from_file(*lens_flare_path);
        test_lens_flare(archive);
        ++passed;
        test_malformed_trk1_is_rejected(archive);
        ++passed;
    }

    const auto glare_glow_path = find_object_archive("GlareGlow");
    if (!glare_glow_path.has_value()) {
        std::cout << "[skip] retail GlareGlow BRK checks\n";
    } else {
        test_glare_glow(smgpc::resource::RarcArchive::from_file(*glare_glow_path));
        ++passed;
    }

    const auto glare_line_path = find_object_archive("GlareLine");
    if (!glare_line_path.has_value()) {
        std::cout << "[skip] retail GlareLine BRK checks\n";
    } else {
        test_glare_line(smgpc::resource::RarcArchive::from_file(*glare_line_path));
        ++passed;
    }

    std::cout << "BRK real-resource tests passed: " << passed << "/5 available checks\n";
    return 0;
}
