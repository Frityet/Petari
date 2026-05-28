#include "layout/BrlanAnimation.hpp"
#include "layout/BrlytLayout.hpp"
#include "resource/RarcArchive.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace {

    struct PaneRenderState {
        float translate_x = 0.0F;
        float translate_y = 0.0F;
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float alpha = 255.0F;
        bool visible = true;
    };

    struct Rect {
        float left = 0.0F;
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
    };

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    [[nodiscard]] std::filesystem::path pc_port_root() {
        const auto cwd = std::filesystem::current_path();
        std::error_code error{};
        if (std::filesystem::is_directory(cwd / "pc-port" / "src", error)) {
            return cwd / "pc-port";
        }
        if (std::filesystem::is_directory(cwd / "src", error) && std::filesystem::is_regular_file(cwd / "xmake.lua", error)) {
            return cwd;
        }

        return cwd / "pc-port";
    }

    [[nodiscard]] std::string lowercase(std::string_view text) {
        auto lowered = std::string(text);
        std::ranges::transform(lowered, lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return lowered;
    }

    [[nodiscard]] std::string sanitize_filename(std::string_view text) {
        auto sanitized = std::string{};
        sanitized.reserve(text.size());

        for (const auto c : text) {
            const auto value = static_cast<unsigned char>(c);
            if (std::isalnum(value) != 0 || c == '-' || c == '_') {
                sanitized.push_back(c);
            } else {
                sanitized.push_back('_');
            }
        }

        return sanitized.empty() ? "layout" : sanitized;
    }

    [[nodiscard]] std::string archive_name_for(std::string_view layout_name) {
        auto archive_name = std::string(layout_name);
        if (!archive_name.ends_with(".arc")) {
            archive_name.append(".arc");
        }
        return archive_name;
    }

    [[nodiscard]] std::optional<std::span<const std::uint8_t>> find_archive_file(const smgpc::compat::RarcArchive &archive,
                                                                                 std::string_view suffix) {
        const auto lowered_suffix = lowercase(suffix);
        const auto it = std::ranges::find_if(archive.entries(), [&lowered_suffix](const auto &entry) {
            const auto lowered = lowercase(entry.path);
            return lowered.size() >= lowered_suffix.size() && lowered.ends_with(lowered_suffix);
        });
        if (it == archive.entries().end()) {
            return std::nullopt;
        }

        return archive.file_data(*it);
    }

    [[nodiscard]] std::unordered_map<std::string, smgpc::compat::BrlanAnimation> load_animations(const smgpc::compat::RarcArchive &archive) {
        auto animations = std::unordered_map<std::string, smgpc::compat::BrlanAnimation>{};
        for (const auto &entry : archive.entries()) {
            const auto lowered = lowercase(entry.path);
            if (!lowered.starts_with("anim/") || !lowered.ends_with(".brlan")) {
                continue;
            }

            auto name = lowered.substr(std::string_view("anim/").size());
            name.resize(name.size() - std::string_view(".brlan").size());
            animations.emplace(std::move(name), smgpc::compat::parse_brlan_animation(archive.file_data(entry)));
        }

        return animations;
    }

    void apply_pane_frame(PaneRenderState &state, const smgpc::compat::BrlanPaneFrame &frame) {
        if (frame.translate_x.has_value()) {
            state.translate_x = *frame.translate_x;
        }
        if (frame.translate_y.has_value()) {
            state.translate_y = *frame.translate_y;
        }
        if (frame.scale_x.has_value()) {
            state.scale_x = *frame.scale_x;
        }
        if (frame.scale_y.has_value()) {
            state.scale_y = *frame.scale_y;
        }
        if (frame.alpha.has_value()) {
            state.alpha = *frame.alpha;
        }
        if (frame.visible.has_value()) {
            state.visible = *frame.visible;
        }
    }

    [[nodiscard]] PaneRenderState pane_state_for(const smgpc::compat::BrlytLayout &layout, std::size_t pane_index,
                                                 const std::unordered_map<std::string, smgpc::compat::BrlanPaneFrame> &committed,
                                                 const std::vector<const smgpc::compat::BrlanAnimation *> &active_animations,
                                                 std::span<const float> active_frames) {
        const auto &pane = layout.panes.at(pane_index);
        auto local = PaneRenderState{
            .translate_x = pane.translate_x,
            .translate_y = pane.translate_y,
            .scale_x = pane.scale_x,
            .scale_y = pane.scale_y,
            .alpha = static_cast<float>(pane.alpha),
            .visible = pane.visible,
        };

        if (const auto committed_it = committed.find(pane.name); committed_it != committed.end()) {
            apply_pane_frame(local, committed_it->second);
        }
        for (auto i = std::size_t{}; i < active_animations.size(); ++i) {
            if (active_animations[i] == nullptr) {
                continue;
            }
            apply_pane_frame(local, active_animations[i]->pane_frame(pane.name, active_frames[i]));
        }

        if (pane.parent_index < 0) {
            return local;
        }

        const auto parent = pane_state_for(layout, static_cast<std::size_t>(pane.parent_index), committed, active_animations, active_frames);
        return PaneRenderState{
            .translate_x = parent.translate_x + local.translate_x * parent.scale_x,
            .translate_y = parent.translate_y + local.translate_y * parent.scale_y,
            .scale_x = parent.scale_x * local.scale_x,
            .scale_y = parent.scale_y * local.scale_y,
            .alpha = parent.alpha * (local.alpha / 255.0F),
            .visible = parent.visible && local.visible,
        };
    }

    [[nodiscard]] float base_position_x(std::uint8_t base_position, float width) {
        switch (base_position % 3U) {
        case 1U:
            return -width * 0.5F;
        case 2U:
            return -width;
        default:
            return 0.0F;
        }
    }

    [[nodiscard]] float base_position_y(std::uint8_t base_position, float height) {
        switch (base_position / 3U) {
        case 1U:
            return -height * 0.5F;
        case 2U:
            return 0.0F;
        default:
            return -height;
        }
    }

    [[nodiscard]] Rect pane_rect(const smgpc::compat::BrlytPane &pane, const PaneRenderState &state) {
        const auto width = pane.width * state.scale_x;
        const auto height = pane.height * state.scale_y;
        const auto x = state.translate_x + base_position_x(pane.base_position, width);
        const auto y = state.translate_y + base_position_y(pane.base_position, height);
        return {
            .left = x,
            .top = y,
            .right = x + width,
            .bottom = y + height,
        };
    }

    [[nodiscard]] std::optional<Rect> merge_rect(std::optional<Rect> bounds, const Rect &rect) {
        if (!bounds.has_value()) {
            return rect;
        }

        bounds->left = std::min(bounds->left, rect.left);
        bounds->top = std::min(bounds->top, rect.top);
        bounds->right = std::max(bounds->right, rect.right);
        bounds->bottom = std::max(bounds->bottom, rect.bottom);
        return bounds;
    }

    void write_rect(std::ofstream &out, const Rect &rect) {
        out << '[' << rect.left << ", " << rect.top << "] -> [" << rect.right << ", " << rect.bottom << ']';
    }

    void write_layout_probe(const std::filesystem::path &output, std::string_view layout_name, const smgpc::compat::BrlytLayout &layout,
                            const std::unordered_map<std::string, smgpc::compat::BrlanAnimation> &animations) {
        std::filesystem::create_directories(output.parent_path());

        auto out = std::ofstream(output);
        if (!out) {
            throw std::runtime_error("cannot write layout probe " + output.string());
        }

        out << "# Layout Probe: " << layout_name << "\n\n";
        out << "- layout size: " << layout.width << " x " << layout.height << '\n';
        out << "- panes: " << layout.panes.size() << '\n';
        out << "- pictures: " << layout.pictures.size() << '\n';
        out << "- text boxes: " << layout.text_boxes.size() << '\n';
        out << "- materials: " << layout.materials.size() << '\n';
        out << "- animations: " << animations.size() << "\n\n";

        out << "## Animations\n\n";
        out << "| name | frames | loop | contents |\n";
        out << "| --- | ---: | --- | ---: |\n";
        for (const auto &[name, animation] : animations) {
            out << "| `" << name << "` | " << animation.frame_size << " | " << (animation.loop ? "yes" : "no") << " | " << animation.contents.size()
                << " |\n";
        }
        out << '\n';

        out << "## Panes\n\n";
        out << "| index | parent | name | base | translate | scale | size | alpha | visible |\n";
        out << "| ---: | ---: | --- | ---: | --- | --- | --- | ---: | --- |\n";
        for (auto i = std::size_t{}; i < layout.panes.size(); ++i) {
            const auto &pane = layout.panes[i];
            out << "| " << i << " | " << pane.parent_index << " | `" << pane.name << "` | " << static_cast<int>(pane.base_position) << " | "
                << pane.translate_x << ',' << pane.translate_y << " | " << pane.scale_x << ',' << pane.scale_y << " | " << pane.width << 'x'
                << pane.height << " | " << static_cast<int>(pane.alpha) << " | " << (pane.visible ? "yes" : "no") << " |\n";
        }
        out << '\n';

        out << "## Materials\n\n";
        out << "| index | name | textures | tex SRTs | tex coord gens | TEV stages | alpha compare | blend |\n";
        out << "| ---: | --- | --- | --- | --- | --- | --- | --- |\n";
        for (auto i = std::size_t{}; i < layout.materials.size(); ++i) {
            const auto &material = layout.materials[i];
            out << "| " << i << " | `" << material.name << "` | ";
            for (auto texture_index = std::size_t{}; texture_index < material.textures.size(); ++texture_index) {
                const auto &texture = material.textures[texture_index];
                if (texture_index != 0U) {
                    out << "<br>";
                }
                out << texture_index << ":`" << texture.texture_name << "` wrap=" << static_cast<int>(texture.wrap_s) << ','
                    << static_cast<int>(texture.wrap_t) << " filter=" << static_cast<int>(texture.min_filter) << ','
                    << static_cast<int>(texture.mag_filter);
            }
            out << " | ";
            for (auto srt_index = std::size_t{}; srt_index < material.tex_srts.size(); ++srt_index) {
                const auto &srt = material.tex_srts[srt_index];
                if (srt_index != 0U) {
                    out << "<br>";
                }
                out << srt_index << ":t=" << srt.translate_s << ',' << srt.translate_t << " r=" << srt.rotate << " s=" << srt.scale_s << ','
                    << srt.scale_t;
            }
            out << " | ";
            for (auto gen_index = std::size_t{}; gen_index < material.tex_coord_gens.size(); ++gen_index) {
                const auto &gen = material.tex_coord_gens[gen_index];
                if (gen_index != 0U) {
                    out << "<br>";
                }
                out << gen_index << ":type=" << static_cast<int>(gen.tex_gen_type) << " src=" << static_cast<int>(gen.tex_gen_src)
                    << " mtx=" << static_cast<int>(gen.tex_mtx);
            }
            out << " | ";
            for (auto stage_index = std::size_t{}; stage_index < material.tev_stages.size(); ++stage_index) {
                const auto &stage = material.tev_stages[stage_index];
                if (stage_index != 0U) {
                    out << "<br>";
                }
                out << stage_index << ":tc=" << static_cast<int>(stage.tex_coord_gen) << " tm=" << stage.tex_map
                    << " cc=" << static_cast<int>(stage.color_chan) << " c=[" << static_cast<int>(stage.color.a) << ','
                    << static_cast<int>(stage.color.b) << ',' << static_cast<int>(stage.color.c) << ','
                    << static_cast<int>(stage.color.d) << "]->" << static_cast<int>(stage.color.out_reg) << " a=["
                    << static_cast<int>(stage.alpha.a) << ',' << static_cast<int>(stage.alpha.b) << ','
                    << static_cast<int>(stage.alpha.c) << ',' << static_cast<int>(stage.alpha.d) << "]->"
                    << static_cast<int>(stage.alpha.out_reg);
            }
            out << " | " << (material.alpha_compare.enabled ? "yes" : "no") << " c=" << static_cast<int>(material.alpha_compare.comp0) << ','
                << static_cast<int>(material.alpha_compare.comp1) << " r=" << static_cast<int>(material.alpha_compare.ref0) << ','
                << static_cast<int>(material.alpha_compare.ref1) << " op=" << static_cast<int>(material.alpha_compare.op) << " | "
                << (material.blend_mode.enabled ? "yes" : "default") << " type=" << static_cast<int>(material.gx_state.blend.type)
                << " src=" << static_cast<int>(material.gx_state.blend.src_factor) << " dst="
                << static_cast<int>(material.gx_state.blend.dst_factor) << " op=" << static_cast<int>(material.gx_state.blend.op)
                << " color_update=" << (material.gx_state.blend.color_update ? "yes" : "no")
                << " alpha_update=" << (material.gx_state.blend.alpha_update ? "yes" : "no") << " |\n";
        }
        out << '\n';

        const auto appear = animations.find("appear");
        const auto wait = animations.find("wait");
        auto committed = std::unordered_map<std::string, smgpc::compat::BrlanPaneFrame>{};
        if (appear != animations.end()) {
            const auto frame = static_cast<float>(std::max<int>(0, appear->second.frame_size - 1));
            for (const auto &pane : layout.panes) {
                committed.emplace(pane.name, appear->second.pane_frame(pane.name, frame));
            }
        }

        const auto *wait_animation = wait == animations.end() ? nullptr : &wait->second;
        const std::vector<const smgpc::compat::BrlanAnimation *> active{wait_animation};
        const std::vector<float> active_frames{80.0F};
        auto picture_bounds = std::optional<Rect>{};
        auto text_bounds = std::optional<Rect>{};

        out << "## Runtime Bounds With Appear Committed And Wait Frame 80\n\n";
        out << "| kind | name | material | pane | rect | state scale | visible |\n";
        out << "| --- | --- | ---: | --- | --- | --- | --- |\n";
        for (const auto &picture : layout.pictures) {
            if (picture.pane_index >= layout.panes.size()) {
                continue;
            }
            const auto &pane = layout.panes[picture.pane_index];
            const auto state = pane_state_for(layout, picture.pane_index, committed, active, active_frames);
            const auto rect = pane_rect(pane, state);
            if (state.visible && picture.visible) {
                picture_bounds = merge_rect(picture_bounds, rect);
            }
            out << "| picture | `" << picture.name << "` | " << picture.material_index << " | `" << pane.name << "` | ";
            write_rect(out, rect);
            out << " | " << state.scale_x << ',' << state.scale_y << " | " << ((state.visible && picture.visible) ? "yes" : "no") << " |\n";
        }
        for (const auto &text_box : layout.text_boxes) {
            if (text_box.pane_index >= layout.panes.size()) {
                continue;
            }
            const auto &pane = layout.panes[text_box.pane_index];
            const auto state = pane_state_for(layout, text_box.pane_index, committed, active, active_frames);
            const auto rect = pane_rect(pane, state);
            if (state.visible && text_box.visible) {
                text_bounds = merge_rect(text_bounds, rect);
            }
            out << "| text | `" << text_box.name << "` | " << text_box.material_index << " | `" << pane.name << "` | ";
            write_rect(out, rect);
            out << " | " << state.scale_x << ',' << state.scale_y << " | " << ((state.visible && text_box.visible) ? "yes" : "no") << " |\n";
        }
        out << '\n';

        out << "## Bounds Summary\n\n";
        if (picture_bounds.has_value()) {
            out << "- visible pictures: ";
            write_rect(out, *picture_bounds);
            out << '\n';
        }
        if (text_bounds.has_value()) {
            out << "- visible text: ";
            write_rect(out, *text_bounds);
            out << '\n';
        }
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto layout_name = argc > 1 ? std::string_view(argv[1]) : std::string_view("TitleLogo");
    const auto archive_path = disc_files_root() / "KrKorean" / "LayoutData" / archive_name_for(layout_name);
    const auto archive = smgpc::compat::RarcArchive::from_file(archive_path);
    const auto brlyt = find_archive_file(archive, ".brlyt");
    if (!brlyt.has_value()) {
        throw std::runtime_error("layout archive has no BRLYT: " + archive_path.string());
    }

    const auto layout = smgpc::compat::parse_brlyt_layout(*brlyt);
    const auto animations = load_animations(archive);
    const auto output = pc_port_root() / ".cache" / "layout-probes" / (sanitize_filename(layout_name) + ".md");
    write_layout_probe(output, layout_name, layout, animations);
    std::cout << "wrote " << output << '\n';
    return 0;
} catch (const std::exception &e) {
    std::cerr << "layout probe failed: " << e.what() << '\n';
    return 1;
}
