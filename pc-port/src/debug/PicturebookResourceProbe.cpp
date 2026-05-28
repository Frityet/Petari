#include "DebugPaths.hpp"
#include "layout/BrlanAnimation.hpp"
#include "layout/BrlytLayout.hpp"
#include "render/J3dModel.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TplTexture.hpp"

#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    struct Options {
        std::optional<std::filesystem::path> output;
    };

    template <typename... Args>
    void add_line(std::vector<std::string> &report, Args &&...args) {
        auto out = std::ostringstream{};
        (out << ... << std::forward<Args>(args));
        report.push_back(out.str());
    }

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] bool has_prefix(std::span<const std::uint8_t> data, std::string_view prefix) {
        if (data.size() < prefix.size()) {
            return false;
        }

        for (auto i = std::size_t{}; i < prefix.size(); ++i) {
            if (data[i] != static_cast<std::uint8_t>(prefix[i])) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] std::filesystem::path disc_path(std::string_view relative_path) {
        return smgpc::debug::disc_files_root() / std::filesystem::path(std::string(relative_path));
    }

    [[nodiscard]] smgpc::compat::RarcArchive load_archive(std::string_view relative_path) {
        return smgpc::compat::RarcArchive::from_file(disc_path(relative_path));
    }

    [[nodiscard]] bool entry_path_ends_with(const smgpc::compat::RarcEntry &entry, std::string_view suffix) {
        return entry.path.size() >= suffix.size() && std::string_view(entry.path).ends_with(suffix);
    }

    [[nodiscard]] std::uint64_t visible_pixel_count(const smgpc::compat::DecodedTexture &texture) {
        auto count = std::uint64_t{};
        for (auto offset = std::size_t{}; offset < texture.rgba.size(); offset += 4U) {
            if (texture.rgba[offset] != 0U || texture.rgba[offset + 1U] != 0U || texture.rgba[offset + 2U] != 0U ||
                texture.rgba[offset + 3U] != 0U) {
                ++count;
            }
        }
        return count;
    }

    struct BtiAggregate {
        std::size_t count = 0U;
        std::uint16_t first_width = 0U;
        std::uint16_t first_height = 0U;
        smgpc::compat::TplTextureFormat first_format = smgpc::compat::TplTextureFormat::I4;
        std::uint64_t visible_pixels = 0U;
    };

    [[nodiscard]] BtiAggregate decode_bti_entries(const smgpc::compat::RarcArchive &archive) {
        auto aggregate = BtiAggregate{};
        for (const auto &entry : archive.entries()) {
            if (!entry_path_ends_with(entry, ".bti")) {
                continue;
            }

            const auto texture = smgpc::compat::decode_bti_texture(archive.file_data(entry));
            require(!texture.image.rgba.empty(), "BTI decode produced an empty image");
            if (aggregate.count == 0U) {
                aggregate.first_width = texture.width;
                aggregate.first_height = texture.height;
                aggregate.first_format = texture.format;
            }
            ++aggregate.count;
            aggregate.visible_pixels += visible_pixel_count(texture.image);
        }
        return aggregate;
    }

    [[nodiscard]] std::size_t brlan_target_count(const smgpc::compat::BrlanAnimation &animation) {
        auto count = std::size_t{};
        for (const auto &content : animation.contents) {
            for (const auto &info : content.infos) {
                count += info.targets.size();
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t brlan_key_count(const smgpc::compat::BrlanAnimation &animation) {
        auto count = std::size_t{};
        for (const auto &content : animation.contents) {
            for (const auto &info : content.infos) {
                for (const auto &target : info.targets) {
                    count += target.step_keys.size();
                    count += target.hermite_keys.size();
                }
            }
        }
        return count;
    }

    void probe_layout_archive(std::vector<std::string> &report, std::string_view relative_path, std::string_view brlyt_path,
                              std::span<const std::string_view> brlan_paths) {
        const auto archive = load_archive(relative_path);
        add_line(report, "archive\t", relative_path, "\tentries\t", archive.entries().size());

        require(archive.contains(brlyt_path), "layout archive is missing required BRLYT");
        const auto layout = smgpc::compat::parse_brlyt_layout(archive.file_data(brlyt_path));
        require(!layout.panes.empty(), "BRLYT did not expose any panes");
        require(!layout.materials.empty(), "BRLYT did not expose any materials");
        add_line(report, "layout\t", relative_path, "\t", brlyt_path, "\tpanes\t", layout.panes.size(), "\tpictures\t", layout.pictures.size(),
                 "\ttext_boxes\t", layout.text_boxes.size(), "\tmaterials\t", layout.materials.size(), "\ttextures\t",
                 layout.texture_names.size(), "\tfonts\t", layout.font_names.size());

        for (const auto brlan_path : brlan_paths) {
            require(archive.contains(brlan_path), "layout archive is missing required BRLAN");
            const auto animation = smgpc::compat::parse_brlan_animation(archive.file_data(brlan_path));
            require(!animation.contents.empty(), "BRLAN did not expose animation contents");
            add_line(report, "animation\t", relative_path, "\t", brlan_path, "\tframes\t", animation.frame_size, "\tloop\t",
                     animation.loop ? 1 : 0, "\tcontents\t", animation.contents.size(), "\ttargets\t", brlan_target_count(animation),
                     "\tkeys\t", brlan_key_count(animation));
        }
    }

    void probe_picturebook_texture_archives(std::vector<std::string> &report) {
        {
            const auto archive = load_archive("ObjectData/PictureBookTexture.arc");
            const auto bti = decode_bti_entries(archive);
            require(bti.count == 3U, "PictureBookTexture.arc should contain three BTI textures");
            require(bti.visible_pixels > 0U, "PictureBookTexture.arc BTI textures should decode visible pixels");
            add_line(report, "texture_archive\tObjectData/PictureBookTexture.arc\tbti_files\t", bti.count, "\tentries\t", archive.entries().size(),
                     "\tdecoded_bti_files\t", bti.count, "\tfirst_size\t", bti.first_width, "x", bti.first_height, "\tfirst_format\t",
                     static_cast<std::uint32_t>(bti.first_format), "\tvisible_pixels\t", bti.visible_pixels);
        }

        constexpr auto expected_page_counts = std::array<std::size_t, 9U>{5U, 5U, 3U, 5U, 3U, 3U, 8U, 7U, 5U};
        for (auto chapter = 1U; chapter <= expected_page_counts.size(); ++chapter) {
            const auto relative_path = "ObjectData/PictureBookChapter" + std::to_string(chapter) + ".arc";
            const auto archive = load_archive(relative_path);
            const auto bti = decode_bti_entries(archive);
            require(bti.count == expected_page_counts[chapter - 1U], "PictureBookChapter archive has an unexpected BTI page count");
            require(bti.visible_pixels > 0U, "PictureBookChapter archive BTI textures should decode visible pixels");
            add_line(report, "chapter_archive\t", relative_path, "\tbti_files\t", bti.count, "\tentries\t", archive.entries().size(),
                     "\tdecoded_bti_files\t", bti.count, "\tfirst_size\t", bti.first_width, "x", bti.first_height, "\tfirst_format\t",
                     static_cast<std::uint32_t>(bti.first_format), "\tvisible_pixels\t", bti.visible_pixels);
        }
    }

    void probe_rosetta_picturebook_model(std::vector<std::string> &report) {
        const auto archive = load_archive("ObjectData/RosettaPictureBook.arc");
        require(archive.contains("rosettapicturebook.bdl"), "RosettaPictureBook.arc is missing rosettapicturebook.bdl");
        const auto model = smgpc::compat::inspect_j3d_model(archive.file_data("rosettapicturebook.bdl"));
        require(model.section_count > 0U, "RosettaPictureBook model did not expose any J3D sections");
        add_line(report, "j3d_model\tObjectData/RosettaPictureBook.arc\trosettapicturebook.bdl\tsections\t", model.section_count, "\tjoints\t",
                 model.joints.has_value() ? model.joints->joint_count : 0U, "\tshapes\t",
                 model.shapes.has_value() ? model.shapes->shape_count : 0U, "\tmaterials\t",
                 model.materials.has_value() ? model.materials->material_count : 0U, "\ttextures\t", model.textures.size());
    }

    [[nodiscard]] std::vector<std::uint8_t> read_file_prefix(const std::filesystem::path &path, std::size_t size) {
        auto input = std::ifstream(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("could not open " + path.string());
        }

        auto bytes = std::vector<std::uint8_t>(size);
        input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        bytes.resize(static_cast<std::size_t>(input.gcount()));
        return bytes;
    }

    void probe_movie_file(std::vector<std::string> &report, std::string_view relative_path) {
        const auto path = disc_path(relative_path);
        require(std::filesystem::is_regular_file(path), "required THP movie file is missing");
        const auto prefix = read_file_prefix(path, 4U);
        require(has_prefix(prefix, "THP"), "movie file does not start with THP magic");
        add_line(report, "movie\t", relative_path, "\tbytes\t", std::filesystem::file_size(path), "\tmagic\tTHP");
    }

    void probe_message_archive(std::vector<std::string> &report) {
        const auto archive = load_archive("KrKorean/MessageData/Message.arc");
        require(archive.contains("message.bmg"), "Message.arc is missing message.bmg");
        require(archive.contains("messageid.tbl"), "Message.arc is missing messageid.tbl");
        require(archive.contains("struct.tbl"), "Message.arc is missing struct.tbl");
        require(has_prefix(archive.file_data("message.bmg"), "MESG"), "message.bmg does not start with MESG magic");
        add_line(report, "message_archive\tKrKorean/MessageData/Message.arc\tentries\t", archive.entries().size(), "\tbmg_bytes\t",
                 archive.find("message.bmg")->data_size, "\tid_table_bytes\t", archive.find("messageid.tbl")->data_size,
                 "\tstruct_table_bytes\t", archive.find("struct.tbl")->data_size);
    }

    [[nodiscard]] Options parse_options(int argc, char **argv) {
        auto options = Options{};
        for (auto i = 1; i < argc; ++i) {
            const auto arg = std::string_view(argv[i]);
            if (arg == "--output") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--output requires a path");
                }
                options.output = std::filesystem::path(argv[++i]);
                continue;
            }

            throw std::runtime_error("unknown argument: " + std::string(arg));
        }
        return options;
    }

    void write_report(const Options &options, const std::vector<std::string> &report) {
        for (const auto &line : report) {
            std::cout << line << '\n';
        }

        if (!options.output.has_value()) {
            return;
        }

        if (!options.output->parent_path().empty()) {
            std::filesystem::create_directories(options.output->parent_path());
        }

        auto output = std::ofstream(*options.output, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("could not write " + options.output->string());
        }
        for (const auto &line : report) {
            output << line << '\n';
        }
    }

    [[nodiscard]] int run_probe(int argc, char **argv) {
        const auto options = parse_options(argc, argv);
        auto report = std::vector<std::string>{};
        add_line(report, "probe\tpicturebook_prologue_resources\troot\t", smgpc::debug::disc_files_root().generic_string());

        constexpr std::string_view picturebook_anims[] = {
            "anim/appear.brlan",
            "anim/buttonappear.brlan",
            "anim/buttondecide.brlan",
            "anim/buttonend.brlan",
            "anim/buttonselectin.brlan",
            "anim/buttonselectout.brlan",
            "anim/buttonwait.brlan",
            "anim/end.brlan",
            "anim/pagenext.brlan",
            "anim/textcolor.brlan",
        };
        probe_layout_archive(report, "LayoutData/PictureBook.arc", "blyt/picturebook.brlyt", picturebook_anims);

        constexpr std::string_view prologue_demo_anims[] = {"anim/prologue.brlan"};
        probe_layout_archive(report, "LayoutData/PrologueDemo.arc", "blyt/prologuedemo.brlyt", prologue_demo_anims);

        constexpr std::string_view prologue_steward_anims[] = {
            "anim/crossfade1.brlan",
            "anim/crossfade2.brlan",
            "anim/crossfade3.brlan",
            "anim/fadein.brlan",
            "anim/fadeout.brlan",
            "anim/winfadein.brlan",
            "anim/winfadeout.brlan",
        };
        probe_layout_archive(report, "LayoutData/PrologueStarSteward.arc", "blyt/prologuestarsteward.brlyt", prologue_steward_anims);

        probe_picturebook_texture_archives(report);
        probe_rosetta_picturebook_model(report);
        probe_movie_file(report, "MovieData/PrologueA.thp");
        probe_movie_file(report, "MovieData/PrologueB.thp");
        probe_message_archive(report);

        write_report(options, report);
        return 0;
    }

}  // namespace

int main(int argc, char **argv) try {
    return run_probe(argc, argv);
} catch (const std::exception &e) {
    std::cerr << "Picturebook resource probe failed: " << e.what() << '\n';
    return 1;
}
