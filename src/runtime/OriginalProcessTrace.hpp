#pragma once

#ifndef NDEBUG
#include <cstdint>
#include <memory>

class GameSystem;

namespace smgpc::runtime {
    // Optional observations at a completed original frame, under its guest
    // execution owner. Diagnostics never change actors, input or story state.
    class OriginalProcessTrace final {
    public:
        OriginalProcessTrace();
        ~OriginalProcessTrace();
        void capture(const GameSystem&, std::uint64_t frame_index);
        OriginalProcessTrace(const OriginalProcessTrace&) = delete;
        OriginalProcessTrace& operator=(const OriginalProcessTrace&) = delete;
    private:
        struct State;
        std::unique_ptr<State> _state;
    };
}
#endif
