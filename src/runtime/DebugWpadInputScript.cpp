#include "runtime/DebugWpadInputScript.hpp"

#ifndef NDEBUG
#include <revolution.h>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <stdexcept>
#include <system_error>

namespace smgpc::runtime {
    namespace {
        [[nodiscard]] std::string_view trim(std::string_view text) {
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
                text.remove_prefix(1U);
            }
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
                text.remove_suffix(1U);
            }
            return text;
        }

        [[nodiscard]] std::optional<std::uint64_t> parse_frame_index(std::string_view text) {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }

            auto frame = std::uint64_t{};
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, frame);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }
            return frame;
        }

        [[nodiscard]] std::optional<float> parse_float(std::string_view text) {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }

            auto value = 0.0F;
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }
            return value;
        }

        [[nodiscard]] std::optional<bool> parse_bool(std::string_view text) {
            text = trim(text);
            if (text == "1" || text == "true" || text == "TRUE" || text == "True" || text == "on" || text == "ON") {
                return true;
            }
            if (text == "0" || text == "false" || text == "FALSE" || text == "False" || text == "off" || text == "OFF") {
                return false;
            }
            return std::nullopt;
        }

        struct DebugFrameRange {
            std::uint64_t first_frame = 0U;
            std::uint64_t last_frame = std::numeric_limits<std::uint64_t>::max();
        };

        [[nodiscard]] std::optional<DebugFrameRange> parse_debug_frame_range(std::string_view text) {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }

            const auto dash = text.find('-');
            if (dash == std::string_view::npos) {
                const auto frame = parse_frame_index(text);
                if (!frame.has_value()) {
                    return std::nullopt;
                }
                return DebugFrameRange{.first_frame = *frame, .last_frame = *frame};
            }

            const auto first = parse_frame_index(text.substr(0U, dash));
            if (!first.has_value()) {
                return std::nullopt;
            }

            auto last = std::numeric_limits<std::uint64_t>::max();
            const auto last_text = trim(text.substr(dash + 1U));
            if (!last_text.empty()) {
                const auto parsed_last = parse_frame_index(last_text);
                if (!parsed_last.has_value() || *parsed_last < *first) {
                    return std::nullopt;
                }
                last = *parsed_last;
            }

            return DebugFrameRange{.first_frame = *first, .last_frame = last};
        }

        [[nodiscard]] std::optional<std::uint32_t> debug_wpad_button_mask(std::string_view text) {
            text = trim(text);
            if (text == "A") {
                return WPAD_BUTTON_A;
            }
            if (text == "B") {
                return WPAD_BUTTON_B;
            }
            if (text == "UP") {
                return WPAD_BUTTON_UP;
            }
            if (text == "DOWN") {
                return WPAD_BUTTON_DOWN;
            }
            if (text == "LEFT") {
                return WPAD_BUTTON_LEFT;
            }
            if (text == "RIGHT") {
                return WPAD_BUTTON_RIGHT;
            }
            if (text == "PLUS" || text == "+") {
                return WPAD_BUTTON_PLUS;
            }
            if (text == "MINUS") {
                return WPAD_BUTTON_MINUS;
            }
            if (text == "HOME") {
                return WPAD_BUTTON_HOME;
            }
            if (text == "C") {
                return WPAD_BUTTON_C;
            }
            if (text == "Z") {
                return WPAD_BUTTON_Z;
            }
            if (text == "ONE" || text == "1") {
                return WPAD_BUTTON_1;
            }
            if (text == "TWO" || text == "2") {
                return WPAD_BUTTON_2;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::uint32_t parse_debug_wpad_button_mask(std::string_view text) {
            auto mask = std::uint32_t{};
            while (true) {
                const auto plus = text.find('+');
                const auto token = trim(text.substr(0U, plus));
                if (const auto button = debug_wpad_button_mask(token)) {
                    mask |= *button;
                }
                if (plus == std::string_view::npos) {
                    break;
                }
                text.remove_prefix(plus + 1U);
            }
            return mask;
        }

        [[nodiscard]] std::vector<DebugWpadButtonScriptSpan> parse_debug_wpad_button_script(std::string_view text) {
            auto spans = std::vector<DebugWpadButtonScriptSpan>{};
            while (!text.empty()) {
                const auto separator = text.find(';');
                const auto entry = trim(text.substr(0U, separator));
                if (!entry.empty()) {
                    const auto colon = entry.find(':');
                    if (colon != std::string_view::npos) {
                        const auto range = parse_debug_frame_range(entry.substr(0U, colon));
                        const auto mask = parse_debug_wpad_button_mask(entry.substr(colon + 1U));
                        if (range.has_value() && mask != 0U) {
                            spans.push_back(DebugWpadButtonScriptSpan{
                                .first_frame = range->first_frame,
                                .last_frame = range->last_frame,
                                .button_mask = mask,
                            });
                        }
                    }
                }
                if (separator == std::string_view::npos) {
                    break;
                }
                text.remove_prefix(separator + 1U);
            }
            return spans;
        }

        [[nodiscard]] std::vector<DebugWpadPointerScriptSpan> parse_debug_wpad_pointer_script(std::string_view text) {
            auto spans = std::vector<DebugWpadPointerScriptSpan>{};
            while (!text.empty()) {
                const auto separator = text.find(';');
                const auto entry = trim(text.substr(0U, separator));
                if (!entry.empty()) {
                    const auto colon = entry.find(':');
                    if (colon != std::string_view::npos) {
                        const auto range = parse_debug_frame_range(entry.substr(0U, colon));
                        auto values = entry.substr(colon + 1U);
                        const auto first_comma = values.find(',');
                        if (range.has_value() && first_comma != std::string_view::npos) {
                            const auto x = parse_float(values.substr(0U, first_comma));
                            values.remove_prefix(first_comma + 1U);
                            const auto second_comma = values.find(',');
                            const auto y = parse_float(values.substr(0U, second_comma));
                            auto valid = true;
                            if (second_comma != std::string_view::npos) {
                                if (const auto parsed_valid = parse_bool(values.substr(second_comma + 1U))) {
                                    valid = *parsed_valid;
                                }
                            }
                            if (x.has_value() && y.has_value()) {
                                spans.push_back(DebugWpadPointerScriptSpan{
                                    .first_frame = range->first_frame,
                                    .last_frame = range->last_frame,
                                    .x = *x,
                                    .y = *y,
                                    .valid = valid,
                                });
                            }
                        }
                    }
                }
                if (separator == std::string_view::npos) {
                    break;
                }
                text.remove_prefix(separator + 1U);
            }
            return spans;
        }

        [[nodiscard]] std::vector<DebugWpadStickScriptSpan> parse_debug_wpad_stick_script(std::string_view text) {
            std::vector<DebugWpadStickScriptSpan> spans;
            while (!text.empty()) {
                const auto separator = text.find(';');
                const auto entry = trim(text.substr(0, separator));
                if (!entry.empty()) {
                    const auto first_colon = entry.find(':');
                    const auto second_colon = first_colon == std::string_view::npos ? first_colon : entry.find(':', first_colon + 1);
                    if (second_colon == std::string_view::npos)
                        throw std::invalid_argument("SMGPC_DEBUG_WPAD_STICK_SCRIPT requires frame-range:x:y entries");
                    const auto range = parse_debug_frame_range(entry.substr(0, first_colon));
                    const auto x = parse_float(entry.substr(first_colon + 1, second_colon - first_colon - 1));
                    const auto y = parse_float(entry.substr(second_colon + 1));
                    if (!range || !x || !y || !std::isfinite(*x) || !std::isfinite(*y) ||
                        *x < -1.0F || *x > 1.0F || *y < -1.0F || *y > 1.0F)
                        throw std::invalid_argument("SMGPC_DEBUG_WPAD_STICK_SCRIPT requires valid frame ranges and finite axes in [-1,1]");
                    spans.push_back({range->first_frame, range->last_frame, *x, *y});
                }
                if (separator == std::string_view::npos) break;
                text.remove_prefix(separator + 1);
            }
            return spans;
        }

        [[nodiscard]] bool debug_span_active(std::uint64_t frame_index, std::uint64_t first_frame, std::uint64_t last_frame) {
            return frame_index >= first_frame && frame_index <= last_frame;
        }

    }

    DebugWpadInputScript::DebugWpadInputScript(std::string_view buttons, std::string_view pointer, std::string_view stick)
        : _buttons(parse_debug_wpad_button_script(buttons)), _pointer(parse_debug_wpad_pointer_script(pointer)),
          _stick(parse_debug_wpad_stick_script(stick)) {
    }

    DebugWpadInputScript DebugWpadInputScript::from_environment() {
        const auto* buttons = std::getenv("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT");
        const auto* pointer = std::getenv("SMGPC_DEBUG_WPAD_POINTER_SCRIPT");
        const auto* stick = std::getenv("SMGPC_DEBUG_WPAD_STICK_SCRIPT");
        return DebugWpadInputScript{buttons ? buttons : "", pointer ? pointer : "", stick ? stick : ""};
    }

    DebugWpadInputScript::Applied DebugWpadInputScript::apply(
        std::uint64_t frame, std::uint32_t& hold, render::core::InputPointerState& pointer, float& stick_x, float& stick_y) const {
        Applied result;
        for (const auto& span : _buttons) {
            if (debug_span_active(frame, span.first_frame, span.last_frame)) {
                hold |= span.button_mask;
                result.buttons = true;
            }
        }
        for (const auto& span : _pointer) {
            if (debug_span_active(frame, span.first_frame, span.last_frame)) {
                pointer = {.x = span.x, .y = span.y, .valid = span.valid};
                result.pointer = true;
            }
        }
        for (const auto& span : _stick) {
            if (debug_span_active(frame, span.first_frame, span.last_frame)) {
                stick_x = span.x;
                stick_y = span.y;
                result.stick = true;
            }
        }
        return result;
    }
}
#endif
