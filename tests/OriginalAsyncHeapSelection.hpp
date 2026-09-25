#pragma once

#include "Game/Util/MemoryUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/allocation.hpp>
#include <memory>
#include <utility>

namespace smgpc::test {
// Original process callbacks already execute on the guest CPU. Select their
// test-owned heap using the same short setter as Game scene initialization;
// do not retain the global heap mutex across original asynchronous file waits.
class OriginalAsyncHeapSelection final {
public:
    explicit OriginalAsyncHeapSelection(std::shared_ptr<compat::JkrAllocationDomain> domain)
        : _domain(std::move(domain)), _previous(MR::getCurrentHeap()), _routing({true, true}) {
        MR::becomeCurrentHeap(&_domain->heap());
    }
    ~OriginalAsyncHeapSelection() { MR::becomeCurrentHeap(_previous); }
    OriginalAsyncHeapSelection(const OriginalAsyncHeapSelection&) = delete;
    OriginalAsyncHeapSelection& operator=(const OriginalAsyncHeapSelection&) = delete;
private:
    std::shared_ptr<compat::JkrAllocationDomain> _domain;
    JKRHeap* _previous;
    aurora::allocation::ClientAllocationScope _routing;
};
}
