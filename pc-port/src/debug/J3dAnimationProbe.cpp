#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/RarcArchive.hpp"

#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto& candidate : candidates) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    [[nodiscard]] std::filesystem::path output_directory(std::string_view object_name) {
        auto out = std::filesystem::current_path() / ".cache" / "j3d-animation-probes" / std::string(object_name);
        std::filesystem::create_directories(out);
        return out;
    }

    void write_btk_sample(std::ofstream& out, const smgpc::game::J3dBtkAnimationSummary& btk,
                          const smgpc::game::J3dBtkMaterialAnimationSummary& material, float frame) {
        const auto sample = smgpc::game::j3d_evaluate_btk_texture_srt(btk, material.material_name, material.tex_matrix_id, frame);
        if (!sample.has_value()) {
            out << "-";
            return;
        }

        out << "scale=(" << sample->scale_s << "," << sample->scale_t << ") rot=" << sample->rotation << " trans=(" << sample->translate_s << ","
            << sample->translate_t << ")";
    }

    void write_bck_sample(std::ofstream& out, const smgpc::game::J3dBckAnimationSummary& bck, std::uint16_t joint, float frame) {
        const auto sample = smgpc::game::j3d_evaluate_bck_joint_transform(bck, joint, frame);
        if (!sample.has_value()) {
            out << "-";
            return;
        }

        out << "scale=(" << sample->scale[0U] << "," << sample->scale[1U] << "," << sample->scale[2U] << ") rot=(" << sample->rotation[0U] << ","
            << sample->rotation[1U] << "," << sample->rotation[2U] << ") trans=(" << sample->translation[0U] << "," << sample->translation[1U] << ","
            << sample->translation[2U] << ")";
    }

    void write_summary(std::ofstream& out, std::string_view file_name, const smgpc::game::J3dAnimationSummary& animation) {
        out << "# J3D Animation Probe: " << file_name << "\n\n";
        out << "- type: `" << animation.type << "`\n";
        out << "- file size: " << animation.file_size << "\n";
        out << "- block count: " << animation.block_count << "\n\n";

        out << "## Sections\n\n";
        out << "| tag | offset | size |\n";
        out << "| --- | ---: | ---: |\n";
        for (const auto& section : animation.sections) {
            out << "| `" << section.tag << "` | 0x" << std::hex << section.offset << std::dec << " | " << section.size << " |\n";
        }
        out << '\n';

        if (animation.bck.has_value()) {
            const auto& bck = *animation.bck;
            out << "## ANK1\n\n";
            out << "- frame max: " << bck.frame_max << "\n";
            out << "- joint count: " << bck.joint_count << "\n";
            out << "- rotation fraction: " << static_cast< int >(bck.rotation_fraction) << "\n";
            out << "- scale values: " << bck.scale_count << "\n";
            out << "- rotation values: " << bck.rotation_count << "\n";
            out << "- translation values: " << bck.translation_count << "\n\n";

            const std::array< float, 2U > sample_frames{0.0F, static_cast< float >(bck.frame_max) * 0.5F};
            out << "| joint | frame 0 transform | half-frame transform |\n";
            out << "| ---: | --- | --- |\n";
            for (auto i = std::uint16_t{}; i < bck.joints.size(); ++i) {
                out << "| " << i << " | ";
                write_bck_sample(out, bck, i, sample_frames[0U]);
                out << " | ";
                write_bck_sample(out, bck, i, sample_frames[1U]);
                out << " |\n";
            }
            out << '\n';
        }

        if (animation.btk.has_value()) {
            const auto& btk = *animation.btk;
            out << "## TTK1\n\n";
            out << "- frame max: " << btk.frame_max << "\n";
            out << "- track count: " << btk.track_count << "\n";
            out << "- scale values: " << btk.scale_count << "\n";
            out << "- rotation values: " << btk.rotation_count << "\n";
            out << "- translation values: " << btk.translation_count << "\n";
            out << "- tex matrix calc type: " << btk.tex_matrix_calc_type << "\n\n";

            const std::array< float, 2U > sample_frames{0.0F, static_cast< float >(btk.frame_max) * 0.5F};
            out << "| index | material | material id | tex matrix id | center | frame 0 SRT | half-frame SRT |\n";
            out << "| ---: | --- | ---: | ---: | --- | --- | --- |\n";
            for (auto i = std::size_t{}; i < btk.materials.size(); ++i) {
                const auto& material = btk.materials[i];
                out << "| " << i << " | `" << material.material_name << "` | " << material.material_id << " | "
                    << static_cast< int >(material.tex_matrix_id) << " | " << material.center[0U] << "," << material.center[1U] << ","
                    << material.center[2U] << " | ";
                write_btk_sample(out, btk, material, sample_frames[0U]);
                out << " | ";
                write_btk_sample(out, btk, material, sample_frames[1U]);
                out << " |\n";
            }
            out << '\n';
        }
    }

}  // namespace

int main(int argc, char** argv) try {
    const auto object_name = argc > 1 ? std::string_view(argv[1]) : std::string_view("CometNearOrbitSky");
    const auto archive = smgpc::game::RarcArchive::from_file(disc_files_root() / "ObjectData" / (std::string(object_name) + ".arc"));
    const auto out_dir = output_directory(object_name);

    for (const auto& entry : archive.entries()) {
        if (!entry.path.ends_with(".bck") && !entry.path.ends_with(".btk")) {
            continue;
        }

        const auto animation = smgpc::game::inspect_j3d_animation(archive.file_data(entry));
        const auto output = out_dir / (entry.path + ".md");
        auto file = std::ofstream(output);
        if (!file) {
            throw std::runtime_error("cannot write " + output.string());
        }

        write_summary(file, entry.path, animation);
        std::cout << output.string() << '\n';
    }

    return 0;
} catch (const std::exception& e) {
    std::cerr << "J3D animation probe failed: " << e.what() << '\n';
    return 1;
}
