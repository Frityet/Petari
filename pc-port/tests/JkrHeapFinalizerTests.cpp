#include "compat/JkrHeapFinalizer.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRDisposer.hpp"
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>

namespace {
    using namespace smgpc::compat;
    void require(bool value) { if (!value) throw std::runtime_error("JKR heap finalizer invariant failed"); }
    struct Resource {
        int& retired;
        Resource* sibling = nullptr;
        explicit Resource(int& counter) : retired(counter) {
            register_jkr_heap_finalizer(this, [](void* pointer) noexcept { static_cast<Resource*>(pointer)->~Resource(); });
        }
        ~Resource() { unregister_jkr_heap_finalizer(this); ++retired; delete sibling; }
    };
    struct OriginalOwner final : JKRDisposer {
        Resource* resource;
        explicit OriginalOwner(Resource* value) : resource(value) {}
        ~OriginalOwner() override { delete resource; }
    };
}
int main() {
    auto runtime = JkrHeapRuntime::create(1024 * 1024);
    auto domain = JkrAllocationDomain::create(runtime, 256 * 1024);
    auto& heap = domain->heap();
    int host_count=0, deleted=0, bulk=0;
    {
        Resource host(host_count);
        {
            JkrAllocationScope scope(domain);
            auto* object=new Resource(deleted); delete object;
            new Resource(bulk);
        }
        heap.freeAll();
        require(deleted==1 && bulk==1 && host_count==0);
    }
    require(host_count==1);
    int head=0, tail=0;
    {
        JkrAllocationScope scope(domain);
        new Resource(head);
        new (heap.alloc(sizeof(Resource), -int(alignof(Resource)))) Resource(tail);
    }
    heap.freeTail(); require(tail==1 && head==0);
    heap.freeAll(); require(tail==1 && head==1);
    int range=0, survivor=0;
    Resource* ranged;
    {
        JkrAllocationScope scope(domain);
        ranged=new Resource(range); new Resource(survivor);
    }
    heap.dispose(ranged, u32(sizeof(Resource))); require(range==1 && survivor==0);
    heap.freeAll(); require(range==1 && survivor==1);
    int first=0, second=0, owned=0;
    {
        JkrAllocationScope scope(domain);
        auto* sibling=new Resource(second);
        auto* owner=new Resource(first); owner->sibling=sibling;
        new OriginalOwner(new Resource(owned));
    }
    heap.freeAll(); require(first==1 && second==1 && owned==1);
    int final=0;
    { JkrAllocationScope scope(domain); new Resource(final); }
    domain.reset(); require(final==1);
    std::cout << "JKR native finalizers: host/explicit/bulk, tail, range, callback mutation/original disposer, domain destruction pass\n";
}
