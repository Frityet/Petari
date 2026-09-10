#pragma once

#include <memory>
#include <array>

class WPad;
class WPadHolder;
class WPadRumble;
class JKRHeap;

namespace smgpc::compat {
class JkrAllocationDomain;

// Retains the original WPadHolder without constructing GameSystem.
// The native platform produces one new KPAD sample per simulation frame.
class WPadOwnership final {
public:
    explicit WPadOwnership(std::shared_ptr<JkrAllocationDomain> domain);
    ~WPadOwnership();
    WPadOwnership(const WPadOwnership&) = delete;
    WPadOwnership& operator=(const WPadOwnership&) = delete;
    void update_samples();
    [[nodiscard]] WPad& pad(int channel);
    [[nodiscard]] WPadHolder& holder();
    [[nodiscard]] JKRHeap& heap();
private:
    struct State;
    std::unique_ptr<State> _state;
    WPadOwnership* _previous = nullptr;
    std::array<WPadRumble*, 2> _previous_rumble{};
};
[[nodiscard]] WPadHolder& require_wpad_holder();
[[nodiscard]] JKRHeap& require_wpad_heap();
} // namespace smgpc::compat
