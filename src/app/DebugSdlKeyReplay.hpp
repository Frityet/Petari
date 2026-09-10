#pragma once

#ifndef NDEBUG

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <SDL3/SDL.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::app {

    // Synthetic integration-test events; the normal window event handler and
    // runtime input translation still consume every key transition.
    class DebugSdlKeyReplay final {
    public:
        explicit DebugSdlKeyReplay(SDL_Window* window) {
            const aurora::allocation::HostAllocationScope host;
            const auto* environment = std::getenv("SMGPC_DEBUG_SDL_KEY_SCRIPT");
            if (environment == nullptr || environment[0] == '\0') return;
            auto script = std::string_view(environment);
            while (true) {
                const auto separator = script.find(';');
                const auto entry = trim(script.substr(0, separator));
                const auto colon = entry.find(':');
                if (colon == std::string_view::npos) fail("Expected first-last:key in SDL key script");
                const auto range = trim(entry.substr(0, colon));
                const auto dash = range.find('-');
                if (dash == std::string_view::npos) fail("Expected an inclusive tick range in SDL key script");
                const auto first = parse_tick(range.substr(0, dash));
                const auto last = parse_tick(range.substr(dash + 1));
                if (first > last) fail("SDL key script range ends before it starts");
                const auto name = std::string(trim(entry.substr(colon + 1)));
                const auto key = SDL_GetKeyFromName(name.c_str());
                if (key == SDLK_UNKNOWN) fail("Unknown SDL key name in SDL key script");
                auto found = std::find_if(_keys.begin(), _keys.end(), [key](const auto& state) { return state.key == key; });
                if (found == _keys.end()) {
                    auto state = KeyState{};
                    state.key = key;
                    state.scancode = SDL_GetScancodeFromKey(key, &state.modifiers);
                    if (state.scancode == SDL_SCANCODE_UNKNOWN) fail("SDL key script key has no scancode in the current layout");
                    _keys.push_back(state);
                    found = _keys.end() - 1;
                }
                found->ranges.push_back({first, last});
                if (separator == std::string_view::npos) break;
                script.remove_prefix(separator + 1);
            }
            _window = SDL_GetWindowID(window);
            if (_window == 0) fail("SDL key replay requires a live SDL window");
        }

        // Call at each reached simulation tick, then poll the ordinary window
        // event loop if this returns true, before starting that Game tick.
        [[nodiscard]] bool advance(std::uint64_t tick) {
            if (_keys.empty()) return false;
            const aurora::allocation::HostAllocationScope host;
            if (tick < _last_tick) fail("SDL key replay ticks must not move backwards");
            _last_tick = tick;
            auto emitted = false;
            for (auto& state : _keys) {
                const auto down = std::any_of(state.ranges.begin(), state.ranges.end(), [tick](const auto& range) {
                    return tick >= range.first && tick <= range.last;
                });
                if (down == state.down) continue;
                auto event = SDL_Event{};
                event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
                event.key.timestamp = SDL_GetTicksNS();
                event.key.windowID = _window;
                event.key.scancode = state.scancode;
                event.key.key = state.key;
                event.key.mod = state.modifiers;
                event.key.down = down;
                event.key.repeat = false;
                if (!SDL_PushEvent(&event)) fail("SDL rejected a scripted key transition");
                state.down = down;
                emitted = true;
                std::fprintf(stderr, "[smgpc:sdl-replay] tick=%llu key=%s down=%d synthetic=1\n",
                             static_cast<unsigned long long>(tick), SDL_GetKeyName(state.key), down);
            }
            return emitted;
        }

    private:
        struct Range { std::uint64_t first, last; };
        struct KeyState {
            SDL_Keycode key = SDLK_UNKNOWN;
            SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
            SDL_Keymod modifiers = SDL_KMOD_NONE;
            bool down = false;
            std::vector<Range> ranges;
        };
        [[noreturn]] static void fail(const char* message) {
            aurora::throw_host_exception<std::invalid_argument>(message);
        }
        [[nodiscard]] static std::string_view trim(std::string_view value) {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos) return {};
            return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
        }
        [[nodiscard]] static std::uint64_t parse_tick(std::string_view text) {
            text = trim(text);
            if (text.empty()) fail("Missing tick in SDL key script");
            auto tick = std::uint64_t{};
            const auto result = std::from_chars(text.data(), text.data() + text.size(), tick);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) fail("Invalid tick in SDL key script");
            return tick;
        }

        std::vector<KeyState> _keys;
        SDL_WindowID _window = 0;
        std::uint64_t _last_tick = 0;
    };

} // namespace smgpc::app

#endif
