#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "NativeHeapFixture.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/guest_thread.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
    using namespace std::chrono_literals;

    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    template<class Predicate> bool wait_until(Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (!predicate()) {
            if (std::chrono::steady_clock::now() >= deadline) return false;
            std::this_thread::yield();
        }
        return true;
    }

    void require_scheduler_enabled() {
        const auto depth = OSDisableScheduler();
        OSEnableScheduler();
        require(depth == 0, "J3D command ownership must not disable the scheduler");
    }

    void original_interrupt_snapshot() {
        J3DModelData model;
        GDLObj caller{};
        for (const auto interrupts : {TRUE, FALSE, TRUE}) {
            OSRestoreInterrupts(interrupts);
            GDSetCurrent(&caller);
            {
                J3DSys::CommandScope commands;
                require_scheduler_enabled();
                // This actual original routine captures its interrupt state
                // once in a function-static, even on later calls by a caller
                // with a different interrupt bit. No material fixture is
                // needed to exercise its GD and scheduler critical section.
                model.indexToPtr();
            }
            require(__GDCurrentDL == &caller, "Original command generation must restore the caller's GD object");
            require(OSDisableInterrupts() == interrupts, "Scope must restore each caller's actual interrupt bit");
            OSRestoreInterrupts(TRUE);
        }
        GDSetCurrent(nullptr);
    }

    void nested_original_mutex_and_exception() {
        const aurora::os::GuestThreadExecutionScope guest;
        auto& heap = MR::MutexHolder<1>::sMutex;
        auto& model = MR::MutexHolder<0>::sMutex;
        OSLockMutex(&heap);
        OSLockMutex(&model);
        GDLObj caller{}, outer_dl{}, nested_dl{};
        GDSetCurrent(&caller);
        j3dSys.mFlags = 0x1234;
        J3DSys::mCurrentMtx[0][3] = 4;
        J3DSys::sTexCoordScaleTable[2].field_0x00 = 11;
        {
            J3DSys::ContextScope outer;
            require(heap.count == 2 && model.count == 2, "Scope must recurse through the original mutexes");
            GDSetCurrent(&outer_dl);
            j3dSys.mFlags = 0x5678;
            J3DSys::mCurrentMtx[0][3] = 8;
            J3DSys::sTexCoordScaleTable[2].field_0x00 = 22;
            try {
                J3DSys::ContextScope nested;
                OSLockMutex(&model); // Original manually paired acquisition.
                GDSetCurrent(&nested_dl);
                j3dSys.mFlags = 0x9999;
                J3DSys::mCurrentMtx[0][3] = 16;
                J3DSys::sTexCoordScaleTable[2].field_0x00 = 33;
                throw std::runtime_error("original body failed before its unlock");
            } catch (const std::runtime_error&) {
            }
            require(heap.count == 2 && model.count == 2, "Unwind must release only newly acquired recursive locks");
            require(__GDCurrentDL == &outer_dl && j3dSys.mFlags == 0x5678 &&
                        J3DSys::mCurrentMtx[0][3] == 8 && J3DSys::sTexCoordScaleTable[2].field_0x00 == 22,
                    "Nested scene unwind must restore the enclosing GD and J3D context");
        }
        require(heap.count == 1 && model.count == 1, "Pre-existing original mutex ownership must survive the scope");
        require(__GDCurrentDL == &caller && j3dSys.mFlags == 0x1234 &&
                    J3DSys::mCurrentMtx[0][3] == 4 && J3DSys::sTexCoordScaleTable[2].field_0x00 == 11,
                "Scene retirement must restore the initial shared context");
        GDSetCurrent(nullptr);
        OSUnlockMutex(&model);
        OSUnlockMutex(&heap);
    }

    void wait_preserves_context(const JKRHeap::Handle& first,
                                const JKRHeap::Handle& second) {
        std::atomic<bool> competing{false}, entered{false}, device_progress{false};
        std::future<void> resource, device;
        GDLObj caller{}, scene_dl{}, resource_dl{};
        GDSetCurrent(&caller);
        j3dSys.mFlags = 0x42;
        {
            const JKRHeap::CurrentHeapScope allocation(*(first));
            const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
            const J3DSys::ContextScope scene;
            require_scheduler_enabled();
            GDSetCurrent(&scene_dl);
            j3dSys.mFlags = 0x84;
            {
                const aurora::allocation::HostAllocationScope host;
                resource = std::async(std::launch::async, [&] {
                    const aurora::os::GuestThreadExecutionScope guest;
                    competing.store(true, std::memory_order_release);
                    const JKRHeap::CurrentHeapScope allocation(*(second));
                    const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
                    const J3DSys::ContextScope commands;
                    entered.store(true, std::memory_order_release);
                    require(JKRHeap::sCurrentHeap == &(*second), "Resource owner must select its actual retained heap");
                    GDSetCurrent(&resource_dl);
                    j3dSys.mFlags = 0x100;
                });
                device = std::async(std::launch::async, [&] {
                    const aurora::os::GuestThreadExecutionScope guest;
                    const aurora::os::GuestInterruptExecutionScope interrupt;
                    device_progress.store(true, std::memory_order_release);
                });
            }
            bool competition_started, device_completed;
            {
                // The actual boundary used by FIFO backpressure: the CPU is
                // released while original resource locks remain owned.
                const aurora::os::GuestThreadWaitScope wait;
                competition_started = wait_until([&] { return competing.load(std::memory_order_acquire); });
                device_completed = device.wait_for(2s) == std::future_status::ready;
            }
            require(competition_started && device_completed && device_progress.load(std::memory_order_acquire),
                    "Device interrupt and another guest owner must progress during the host wait");
            device.get();
            require(!entered.load(std::memory_order_acquire), "Waiting resource owner must not overwrite active scene context");
            require(__GDCurrentDL == &scene_dl && j3dSys.mFlags == 0x84 &&
                        JKRHeap::sCurrentHeap == &(*first), "Device wait must preserve active GD, J3D and heap ownership");
        }
        require(resource.wait_for(2s) == std::future_status::ready, "Resource owner must resume after scene retirement");
        resource.get();
        require(__GDCurrentDL == &caller && j3dSys.mFlags == 0x42, "Both owners must restore their caller context");
        GDSetCurrent(nullptr);
    }

    void heap_before_j3d(const JKRHeap::Handle& domain) {
        std::atomic<bool> heap_owned{false}, release_resource{false}, observed_order{false};
        OSThread* scene_thread;
        {
            const aurora::os::GuestThreadExecutionScope guest;
            scene_thread = OSGetCurrentThread();
        }
        auto resource = std::async(std::launch::async, [&] {
            const aurora::os::GuestThreadExecutionScope guest;
            const JKRHeap::CurrentHeapScope allocation(*(domain));
            const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
            heap_owned.store(true, std::memory_order_release);
            {
                const aurora::os::GuestThreadWaitScope wait;
                require(wait_until([&] { return release_resource.load(std::memory_order_acquire); }),
                        "Test observer must release the resource owner");
            }
            const J3DSys::CommandScope commands;
            require_scheduler_enabled();
        });
        require(wait_until([&] { return heap_owned.load(std::memory_order_acquire); }), "Resource owner must hold the heap first");
        auto observer = std::async(std::launch::async, [&] {
            const bool blocked = wait_until([&] {
                const aurora::os::GuestThreadExecutionScope guest;
                if (scene_thread->mutex != &MR::MutexHolder<1>::sMutex) return false;
                observed_order.store(MR::MutexHolder<0>::sMutex.thread != scene_thread, std::memory_order_release);
                return true;
            });
            release_resource.store(true, std::memory_order_release);
            require(blocked, "Scene scope must wait for the pre-existing heap owner");
        });
        {
            // Matches SceneScheduler: J3D scope precedes the nested callback's
            // allocation scope, while another resource already owns heap1.
            const J3DSys::ContextScope scene;
            require(observed_order.load(std::memory_order_acquire), "Scene scope must acquire heap1 before J3D0");
        }
        observer.get();
        resource.get();
    }
}

int main() {
    try {
        OSInit();
        original_interrupt_snapshot();
        nested_original_mutex_and_exception();
        auto runtime = smgpc::test::create_native_root_heap(16U << 20);
        auto first = smgpc::test::create_native_solid_heap(runtime, 4U << 20);
        auto second = smgpc::test::create_native_solid_heap(runtime, 4U << 20);
        wait_preserves_context(first, second);
        heap_before_j3d(second);
        std::cout << "PASS J3D device waits, original recursive mutexes, heap lock order, context unwind and static interrupt snapshots\n";
    } catch (const std::exception& error) {
        OSRestoreInterrupts(TRUE);
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
