#include "layout/LayoutResourceResolver.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace smgpc::layout {
    namespace {

        [[nodiscard]] std::string lower_copy(std::string_view value) {
            auto lower = std::string(value);
            std::ranges::transform(lower, lower.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            return lower;
        }

        [[nodiscard]] const smgpc::resource::RarcEntry *find_unique_exact_path(
            std::span<const smgpc::resource::RarcEntry> entries, std::string_view requested_path) {
            const auto expected = lower_copy(requested_path);
            const smgpc::resource::RarcEntry *match = nullptr;
            for (const auto &entry : entries) {
                if (lower_copy(entry.path) != expected) {
                    continue;
                }
                if (match != nullptr) {
                    throw std::runtime_error("Layout archive contains duplicate exact resource " + expected);
                }
                match = &entry;
            }
            return match;
        }

        [[nodiscard]] bool valid_resource_stem(std::string_view name) {
            return !name.empty() && name.find('/') == std::string_view::npos &&
                   name.find('\\') == std::string_view::npos;
        }

    }  // namespace

    const smgpc::resource::RarcEntry *find_layout_brlyt(
        std::span<const smgpc::resource::RarcEntry> entries, std::string_view layout_name) {
        if (!valid_resource_stem(layout_name)) {
            return nullptr;
        }
        return find_unique_exact_path(entries, "blyt/" + lower_copy(layout_name) + ".brlyt");
    }

    const smgpc::resource::RarcEntry *find_layout_brlan(
        std::span<const smgpc::resource::RarcEntry> entries, std::string_view animation_name) {
        if (!valid_resource_stem(animation_name)) {
            return nullptr;
        }
        return find_unique_exact_path(entries, "anim/" + lower_copy(animation_name) + ".brlan");
    }

}  // namespace smgpc::layout
