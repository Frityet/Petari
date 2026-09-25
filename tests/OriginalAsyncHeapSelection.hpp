#pragma once

#include "Game/Util/MemoryUtil.hpp"
#include "NativeHeapFixture.hpp"
#include <aurora/allocation.hpp>
#include <memory>
#include <utility>

namespace smgpc::test {
// Original process callbacks already execute on the guest CPU. Select their
// test-owned heap using the same short setter as Game scene initialization;
// do not retain the global heap mutex across original asynchronous file waits.
class OriginalAsyncHeapSelection final {
public:
    explicit OriginalAsyncHeapSelection(JKRHeap::Handle domain)
        : _domain(std::move(domain)), _previous(MR::getCurrentHeap()->retainNativeLifetime()), _routing({true, true}) {
        MR::becomeCurrentHeap(&(*_domain));
    }
    ~OriginalAsyncHeapSelection() { MR::becomeCurrentHeap(_previous.get()); }
    OriginalAsyncHeapSelection(const OriginalAsyncHeapSelection&) = delete;
    OriginalAsyncHeapSelection& operator=(const OriginalAsyncHeapSelection&) = delete;
private:
    JKRHeap::Handle _domain;
    JKRHeap::Handle _previous;
    aurora::allocation::ClientAllocationScope _routing;
};
}
