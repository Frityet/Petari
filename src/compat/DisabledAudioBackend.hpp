#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>

class JKRHeap;
namespace aurora::audio { class DisabledObjectAudioService; }

namespace smgpc::compat {
// An explicitly disabled output backend. Completion means its requested work
// has completed with zero output voices/banks, not that a hardware system exists.
class DisabledAudioBackend final {
public:
    enum class BankGroup { Static, Stage, Scenario };
    enum class Phase { Created, Requested, Received, Initialized };

    explicit DisabledAudioBackend(JKRHeap& owner_heap);
    ~DisabledAudioBackend();
    DisabledAudioBackend(const DisabledAudioBackend&) = delete;
    DisabledAudioBackend& operator=(const DisabledAudioBackend&) = delete;

    void request_initialize();
    void receive_initialize();
    void initialize(std::span<const std::uint8_t> baa);
    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] Phase phase() const noexcept { return _phase; }
    void request_banks(BankGroup);
    [[nodiscard]] bool banks_complete(BankGroup) const noexcept;
    void prepare_reset() noexcept;
    void request_reset() noexcept;
    [[nodiscard]] bool reset_complete() const noexcept;
    void resume_reset() noexcept;
    void stop_all() noexcept;
    [[nodiscard]] bool has_output_device() const noexcept { return false; }

private:
    JKRHeap& _owner_heap; // Borrowed until the original object's heap finalizer.
    std::unique_ptr<aurora::audio::DisabledObjectAudioService> _objects;
    Phase _phase = Phase::Created;
    std::array<bool, 3> _banks{};
    bool _reset_requested = false;
};
}
