#include "Game/Player/PlayerEvent.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Call {
    unsigned callback;
    u16 event_frame;
    u16 sequence_frame;
    bool operator==(const Call&) const = default;
};

class Sequence final : public EventSequence {
public:
    Sequence() : EventSequence(6) {
        addEventOnTime("one frame", static_cast<EventFunc1>(&Sequence::once), 0);
        addEventInTime("inclusive interval", static_cast<EventFunc1>(&Sequence::interval), 1, 3);
        addEventInStatus("conditional", static_cast<EventFunc1>(&Sequence::status), static_cast<EventFunc2>(&Sequence::even));
        addEventInPhase("first phase", static_cast<EventFunc1>(&Sequence::phase_zero), 0);
        addEventInPhase("second phase", static_cast<EventFunc1>(&Sequence::phase_one), 1);
        addEventOnTime("after stop", static_cast<EventFunc1>(&Sequence::after_stop), 3);
    }
    void updateBefore() override { ++before; }
    void updateAfter() override { ++after; }
    void once(u16 event, u16 frame) { calls.push_back({0, event, frame}); }
    void interval(u16 event, u16 frame) { calls.push_back({1, event, frame}); }
    void status(u16 event, u16 frame) { calls.push_back({2, event, frame}); }
    bool even(u16 frame) { checks.push_back(frame); return (frame & 1) == 0; }
    void phase_zero(u16 event, u16 frame) {
        calls.push_back({3, event, frame});
        if (event == 1) nextPhase();
    }
    void phase_one(u16 event, u16 frame) {
        calls.push_back({4, event, frame});
        if (event == 1) stopSequence();
    }
    void after_stop(u16 event, u16 frame) { calls.push_back({5, event, frame}); }
    std::vector<Call> calls;
    std::vector<u16> checks;
    unsigned before = 0;
    unsigned after = 0;
};

class WrappingSequence final : public EventSequence {
public:
    WrappingSequence() : EventSequence(1) {
        addEventInStatus("always", static_cast<EventFunc1>(&WrappingSequence::callback), nullptr);
    }
    void callback(u16 event, u16 frame) { last_event = event; last_frame = frame; ++calls; }
    unsigned calls = 0;
    u16 last_event = 0;
    u16 last_frame = 0;
};
} // namespace

int main() {
    Sequence sequence;
    require(!sequence.checkAndRun(0), "frame0 must continue");
    require(!sequence.checkAndRun(1) && sequence.getPhase() == 1, "nextPhase must apply after callbacks");
    require(!sequence.checkAndRun(2), "first frame of phase1 must continue");
    require(sequence.checkAndRun(3), "second frame of phase1 must stop");
    require(sequence.before == 4 && sequence.after == 4, "before/after hooks must run even when a callback stops the sequence");
    const std::vector<Call> expected = {
        {0, 0, 0}, {2, 0, 0}, {3, 0, 0},
        {1, 0, 1}, {3, 1, 1},
        {1, 1, 2}, {2, 1, 2}, {4, 0, 2},
        {1, 2, 3}, {4, 1, 3}
    };
    require(sequence.calls == expected, "original time/status/phase callback order or local frame counters changed");
    require(sequence.checks == std::vector<u16>({0, 1, 2, 3}), "status checks must use the sequence frame on every visit");
    sequence.clearFlag();
    sequence.calls.clear();
    require(sequence.getPhase() == 0 && !sequence.checkAndRun(0), "clearFlag must restart phase and stop state");
    require(sequence.calls == std::vector<Call>({{0, 0, 0}, {2, 0, 0}, {3, 0, 0}}), "clearFlag must restart each event-local counter");

    HashSortTable table(2);
    const auto address = reinterpret_cast<HashSortTable::Value>(&sequence);
    require(table.add(0xFE010203U, address), "real event pointer must enter the hash table");
    require(table.add(0x12010203U, 0), "lower hash fixture entry must be added");
    table.sort();
    HashSortTable::Value found = 0;
    require(table.search(0xFE010203U, &found) && found == address,
            "sorting and lookup must preserve every bit of the event owner pointer");
    require(reinterpret_cast<EventSequence*>(found) == &sequence, "typed event lookup must retain identity");
    require(!table.search(0xFE010204U, &found) && found == 0, "absent event lookup must clear its output");
    delete[] table.mHashCodes;
    delete[] table._8;
    delete[] table._C;
    delete[] table._10;

    WrappingSequence wrapping;
    for (u32 i = 0; i <= 65536; ++i) require(!wrapping.checkAndRun(i), "unconditional status event must remain active");
    require(wrapping.calls == 65537 && wrapping.last_event == 0 && wrapping.last_frame == 0,
            "original event counters and callback sequence arguments must retain u16 wrap semantics");
    std::cout << "PASS original event time/status/phase ordering, reset, stop, pointer-width hash lookup and u16 counters\n";
}
