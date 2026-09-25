#include "JSystem/JKernel/JKRThread.hpp"
#include "NativeHeapFixture.hpp"

#include <aurora/guest_thread.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Result {
    OSMessageQueue completed{};
    std::array<OSMessage, 1> completedStorage{};
    std::array<OSMessage, 3> received{};
    bool allocationInHeap = false;
    bool unwound = false;

    Result() { OSInitMessageQueue(&completed, completedStorage.data(), completedStorage.size()); }
    void wait() {
        OSMessage message = nullptr;
        require(OSReceiveMessage(&completed, &message, OS_MESSAGE_BLOCK), "actual worker acknowledgement required");
        require(message == this, "native message must preserve its complete pointer value");
    }
};

class QueueWorker final : public JKRThread {
public:
    QueueWorker(JKRHeap* heap, Result& result) : JKRThread(heap, 0x8021, 3, 15), mResult(result) {}

    void* run() override {
        for (auto& message : mResult.received) message = waitMessageBlock();
        auto* allocation = new int(73);
        mResult.allocationInHeap = mHeap->find(allocation);
        delete allocation;
        OSSendMessage(&mResult.completed, &mResult, OS_MESSAGE_BLOCK);
        return &mResult;
    }

private:
    Result& mResult;
};

class CancelWorker final : public JKRThread {
public:
    // The original implicit-heap constructor resolves the actual allocation
    // containing this object, rather than borrowing an unrelated current heap.
    explicit CancelWorker(Result& result) : JKRThread(0x8000, 1, 15), mResult(result) {}

    void* run() override {
        struct Unwind {
            bool& observed;
            ~Unwind() { observed = true; }
        } unwind{mResult.unwound};
        OSSendMessage(&mResult.completed, &mResult, OS_MESSAGE_BLOCK);
        waitMessageBlock();
        std::abort();
    }

private:
    Result& mResult;
};

void exercise(const JKRHeap::Handle& heaps) {

    auto domain = smgpc::test::create_native_solid_heap(heaps, 256U * 1024U);
    const auto originalCount = JKRThread::sThreadList.getNumLinks();
    std::array<int, 4> values{3, 5, 7, 11};
    Result result;
    std::unique_ptr<QueueWorker> worker;
    {
        const JKRHeap::CurrentHeapScope allocation(*(domain));
        const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
        worker.reset(new QueueWorker(&(*domain), result));
    }
    require(worker->mHeap == &(*domain) && worker->mStackSize == 0x8020,
            "original thread must own an aligned stack in the requested heap");
    require(OSIsThreadSuspended(worker->getThreadRecord()), "original thread must start suspended");
    require(JKRThread::searchThread(worker->getThreadRecord()) == worker.get(),
            "original thread list must identify its actual SDK record");
    require(JKRThread::sThreadList.getNumLinks() == originalCount + 1, "actual thread must be registered once");
    require(worker->sendMessage(&values[0]) && worker->sendMessage(&values[1]), "queue must accept ordinary messages");
    worker->jamMessageBlock(&values[2]);
    require(!worker->sendMessage(&values[3]), "full original queue must reject a nonblocking send");
    worker->resume();
    result.wait();
    require(result.received == std::array<OSMessage, 3>{&values[2], &values[0], &values[1]},
            "jammed and queued host pointers must retain original delivery order and width");
    require(result.allocationInHeap, "SDK worker must inherit the original client heap routing");
    while (!OSIsThreadTerminated(worker->getThreadRecord())) OSYieldThread();
    auto* oldRecord = worker->getThreadRecord();
    worker.reset();
    require(JKRThread::searchThread(oldRecord) == nullptr && JKRThread::sThreadList.getNumLinks() == originalCount,
            "completed thread destruction must unregister the actual record");

    Result cancellation;
    std::unique_ptr<CancelWorker> blocked;
    {
        const JKRHeap::CurrentHeapScope allocation(*(domain));
        const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
        blocked.reset(new CancelWorker(cancellation));
    }
    require(blocked->mHeap == &(*domain), "implicit constructor must resolve the object's actual heap");
    blocked->resume();
    cancellation.wait();
    oldRecord = blocked->getThreadRecord();
    blocked.reset();
    require(cancellation.unwound, "destruction must finish cancellation before freeing worker storage");
    require(JKRThread::searchThread(oldRecord) == nullptr, "cancelled thread must leave no list entry");

    std::unique_ptr<JKRThread> suspended;
    {
        const JKRHeap::CurrentHeapScope allocation(*(domain));
        const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
        suspended.reset(new JKRThread(static_cast<JKRHeap*>(nullptr), 0x8000, 2, 15));
    }
    require(suspended->mHeap == &(*domain), "null explicit heap must select the actual current heap");
    suspended.reset();
    require(JKRThread::sThreadList.getNumLinks() == originalCount,
            "a never-resumed thread must also cancel and unregister during destruction");
}
}

int main() {
    try {
        const aurora::os::GuestThreadExecutionScope execution;
        OSInit();
        const auto heaps = smgpc::test::create_native_root_heap(2U * 1024U * 1024U);
        const auto originalFree = (*heaps).getTotalFreeSize();
        for (unsigned cycle = 0; cycle < 3; ++cycle) {
            exercise(heaps);
            require((*heaps).getTotalFreeSize() == originalFree,
                    "thread heap retirement must reclaim every original allocation");
        }
        std::cout << "Original JKRThread queues, pointer delivery, worker allocation, cancellation and three heap retirements passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
