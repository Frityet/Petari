#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>

namespace smgpc::render::core {

    // Host events can contain an entire tap between two controller samples.
    // Keep each physical source independent and expose a press for one poll,
    // even if its matching release is already in the same event batch.
    template <typename Button>
    class FrameButtonState final {
    public:
        using Source = std::uint64_t;

        void begin_poll() {
            _pressed.fill(false);
        }

        void press(Source source, Button button, bool repeat = false) {
            const auto index = static_cast<std::size_t>(button);
            if (repeat || index >= _held.size()) return;
            const auto [entry, inserted] = _sources.emplace(source, button);
            if (!inserted) return;
            ++_held[index];
            _pressed[index] = true;
        }

        void release(Source source) {
            const auto entry = _sources.find(source);
            if (entry == _sources.end()) return;
            --_held[static_cast<std::size_t>(entry->second)];
            _sources.erase(entry);
        }

        void clear() {
            _sources.clear();
            _held.fill(0);
            _pressed.fill(false);
        }

        [[nodiscard]] bool is_pressed(Button button) const {
            const auto index = static_cast<std::size_t>(button);
            return index < _held.size() && (_held[index] != 0 || _pressed[index]);
        }

    private:
        std::map<Source, Button> _sources;
        std::array<std::size_t, static_cast<std::size_t>(Button::COUNT)> _held{};
        std::array<bool, static_cast<std::size_t>(Button::COUNT)> _pressed{};
    };

}  // namespace smgpc::render::core
