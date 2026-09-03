#include <dolphin/base/PPCArch.h>

#include <atomic>

// Retail PPCSync executes sc; the OSSync handler runs isync/sync with HID0
// temporarily adjusted. Retain its memory-ordering boundary on a native CPU;
// cache-range operations have their own providers.
extern "C" void PPCSync() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}
