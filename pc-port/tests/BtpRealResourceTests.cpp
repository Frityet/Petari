#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "render/J3dAnimation.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dModel.hpp"

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
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

    [[nodiscard]] std::optional<std::filesystem::path> find_real_file_select_archive() {
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::filesystem::path candidates[]{
                root / "orig/RMGK02/files/ObjectData/FileSelectDataMario.arc",
                root / "orig/RMGK01/files/ObjectData/FileSelectDataMario.arc",
                root / "container/orig/RMGK01/files/ObjectData/FileSelectDataMario.arc",
                root / "pc-port/container/orig/RMGK01/files/ObjectData/FileSelectDataMario.arc",
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

    void test_retail_tpt1_tracks_match_retail_model(const smgpc::resource::RarcArchive &archive) {
        const auto *model_entry = archive.find_by_basename("fileselectdatamario.bdl");
        require(model_entry != nullptr, "the retail archive must contain FileSelectDataMario.bdl");
        const auto geometry = smgpc::render::extract_j3d_model_geometry(archive.file_data(*model_entry));
        require(geometry.materials.has_value() && !geometry.textures.empty(),
                "the retail model must expose its real material and texture tables");

        constexpr std::string_view animation_names[]{"blink.btp", "close.btp", "open.btp", "wait.btp"};
        for (const auto animation_name : animation_names) {
            const auto *entry = archive.find_by_basename(animation_name);
            require(entry != nullptr, "the retail file-select archive is missing a required BTP");
            const auto summary = smgpc::render::inspect_j3d_animation(archive.file_data(*entry));
            require(summary.type == "btp1" && summary.btp.has_value() && !summary.btp->materials.empty(),
                    "a retail .btp must parse as a non-empty TPT1 animation");

            for (const auto &track : summary.btp->materials) {
                const auto material = std::ranges::find_if(geometry.materials->materials, [&track](const auto &candidate) {
                    return candidate.name == track.material_name;
                });
                require(material != geometry.materials->materials.end(),
                        "a retail BTP track must name a real model material");
                const auto passes = smgpc::render::j3d_material_texture_passes(*material);
                require(std::ranges::any_of(passes, [&track](const auto &pass) {
                            return pass.tex_map_slot == track.texture_slot;
                        }),
                        "a retail BTP track must target a real texture-map slot");

                for (auto frame = std::uint16_t{}; frame < track.max_frame; ++frame) {
                    const auto texture_index = summary.btp->texture_indices[track.texture_index_offset + frame];
                    require(texture_index < geometry.textures.size(),
                            "every retail BTP value must address a real TEX1 texture");
                    const auto evaluated = smgpc::render::j3d_evaluate_btp_texture_index(
                        *summary.btp, track.material_name, track.texture_slot, static_cast<float>(frame));
                    require(evaluated.has_value() && *evaluated == texture_index,
                            "TPT1 evaluation must select the retail texture index for the current frame");
                }
                const auto final_value = summary.btp->texture_indices[track.texture_index_offset + track.max_frame - 1U];
                const auto clamped = smgpc::render::j3d_evaluate_btp_texture_index(
                    *summary.btp, track.material_name, track.texture_slot, static_cast<float>(track.max_frame + 100U));
                require(clamped.has_value() && *clamped == final_value,
                        "a one-shot retail TPT1 track must hold its final texture");
            }
        }
    }

    void test_retail_actor_action_mapping(const smgpc::resource::RarcArchive &archive) {
        const auto *entry = archive.find_by_basename("actoranimctrl.bcsv");
        require(entry != nullptr, "the retail archive must contain ActorAnimCtrl.bcsv");
        const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry));
        auto found_normal = false;
        for (auto row = 0U; row < table.entry_count(); ++row) {
            if (table.get_string(row, "ActorAnimName") != std::optional<std::string>{"normal"}) {
                continue;
            }
            found_normal = true;
            require(table.get_string(row, "BtpName") == std::optional<std::string>{"wait"},
                    "the retail normal action must resolve its BTP through ActorAnimCtrl");
        }
        require(found_normal, "the retail animation table must contain its normal action");
    }

    void test_malformed_tpt1_is_rejected(const smgpc::resource::RarcArchive &archive) {
        const auto *entry = archive.find_by_basename("blink.btp");
        require(entry != nullptr, "the retail archive must contain blink.btp");
        const auto source = archive.file_data(*entry);
        auto malformed = std::vector<std::uint8_t>(source.begin(), source.end());
        malformed[0x2cU] = 0U;
        malformed[0x2dU] = 0U;

        auto rejected = false;
        try {
            static_cast<void>(smgpc::render::inspect_j3d_animation(malformed));
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        require(rejected, "a trackless TPT1 must be rejected instead of reporting playback success");
    }

    void test_real_frame_controller_crossings() {
        constexpr auto frame_max = std::int16_t{8};
        constexpr std::array<std::uint8_t, 5U> attributes{0U, 1U, 2U, 3U, 4U};
        constexpr std::array<float, 11U> pass_frames{-1.0F, 0.0F, 0.5F, 1.0F, 2.5F, 4.0F,
                                                     6.5F, 7.0F, 7.5F, 8.0F, 9.0F};

        for (const auto attribute : attributes) {
            auto controller = J3DFrameCtrl{frame_max};
            controller.setAttribute(attribute);
            for (auto elapsed = 0U; elapsed < 40U; ++elapsed) {
                const auto derived_frame = smgpc::render::j3d_animation_frame(
                    attribute, frame_max, static_cast<float>(elapsed));
                require(std::abs(controller.getFrame() - derived_frame) < 0.0001F,
                        "host animation frame derivation must match the retail J3DFrameCtrl update state");
                for (const auto pass_frame : pass_frames) {
                    require(smgpc::render::j3d_animation_check_pass(
                                attribute, frame_max, static_cast<float>(elapsed), pass_frame) ==
                                (controller.checkPass(pass_frame) != FALSE),
                            "host BCK passage must match the retail J3DFrameCtrl interval exactly");
                }
                controller.update();
            }
        }
    }
}  // namespace

int main() {
    auto passed = 0;
    const auto archive_path = find_real_file_select_archive();
    if (!archive_path.has_value()) {
        std::cout << "[skip] retail FileSelectDataMario.arc checks\n";
    } else {
        const auto archive = smgpc::resource::RarcArchive::from_file(*archive_path);
        test_retail_tpt1_tracks_match_retail_model(archive);
        ++passed;
        test_retail_actor_action_mapping(archive);
        ++passed;
        test_malformed_tpt1_is_rejected(archive);
        ++passed;
    }
    test_real_frame_controller_crossings();
    ++passed;

    std::cout << "BTP real-resource tests passed: " << passed << "/4\n";
    return 0;
}
