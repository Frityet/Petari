#pragma once

#include <memory>

class WPad;

namespace smgpc::compat {
class JkrAllocationDomain;

// Retains the original typed input records without constructing GameSystem.
// The native platform produces one new KPAD sample per simulation frame.
class WPadOwnership final {
public:
    explicit WPadOwnership(std::shared_ptr<JkrAllocationDomain> domain);
    ~WPadOwnership();
    WPadOwnership(const WPadOwnership&) = delete;
    WPadOwnership& operator=(const WPadOwnership&) = delete;
    void update_pointer_samples();
    [[nodiscard]] WPad& pad(int channel);
private:
    struct State;
    std::unique_ptr<State> _state;
    WPadOwnership* _previous = nullptr;
};
} // namespace smgpc::compat
