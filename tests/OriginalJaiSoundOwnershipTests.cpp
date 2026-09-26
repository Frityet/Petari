#include "JSystem/JAudio2/JAISound.hpp"
#include "JSystem/JAudio2/JAISoundChild.hpp"
#include "JSystem/JAudio2/JAIStreamDataMgr.hpp"
#include "JSystem/JAudio2/JAIStreamMgr.hpp"
#include "NativeHeapFixture.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

static void testStreamLifetime() {
    const aurora::os::GuestThreadExecutionScope execution;
    auto heap = smgpc::test::create_native_root_heap(1024 * 1024);
    auto arena = smgpc::test::create_native_solid_heap(heap, 256 * 1024);
    JASDram = static_cast<JKRSolidHeap *>(arena.get());
    JAIStream::newMemPool(2);
    JAISoundChild::newMemPool(6);
    // Exercise cancellation before DVD preparation. Full transport, PCM output,
    // pause and resume use the real-disc original-process SFX fixture.
    struct Files : JAIStreamDataMgr {
        s32 getStreamFileEntry(JAISoundID) override {
            return 1;
        }
    } files;
    struct Aram : JAIStreamAramMgr {
        void *newStreamAram(u32 *) override {
            assert(false);
            return nullptr;
        }
        bool deleteStreamAram(uintptr_t) override {
            assert(false);
            return false;
        }
    } aram;
    JAIStreamMgr manager(false);
    manager.setStreamDataMgr(&files);
    manager.setStreamAramMgr(&aram);
    JAISoundHandle handle;
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        // The retail function returns zero even after attaching the stream.
        (void)manager.startSound(JAISoundID(0x02000001), &handle, nullptr);
        assert(handle.isSoundAttached() && !handle->isPrepared() && manager.getNumActiveStreams() == 1);
        auto *stream = handle->asStream();
        assert(stream && stream->getNumChild() == 6);
        for (int i = 0; i < stream->getNumChild(); ++i)
            assert(stream->getChild(i));
        handle->lockWhenPrepared();
        assert(stream->mStatus.getState() == JAISoundStatus_::State_LOCK_PREPARE);
        handle->unlockIfLocked();
        handle->stop();
        manager.calc();
        assert(!handle.isSoundAttached() && !manager.isActive());
    }
    JASDram = nullptr;
    std::puts("[pass] Original stream pool allocation, child ownership and cancellation before preparation");
}

int main() {
    static_assert(sizeof(JAISoundHandle) == sizeof(JAISound *));
    JAISoundID id(2, 13, 0x1234);
    assert(u32(id) == 0x020D1234 && id.getGroupID() == 13 && id.getWaveID() == 0x1234);
    JAISoundStatus_ status;
    status.init();
    status.pause(true);
    assert(status._0.value == 0x40);
    status._0.value = 0x80;
    assert(status.isMute() && !status.isPaused());
    status.setAnimationState(3);
    u8 flags = 0;
    std::memcpy(&flags, &status.mState.flags, sizeof(flags));
    assert(flags == 0x30 && status.getAnimationState() == 3);
    status.init();
    assert(status.lockWhenPrepared() == 1 && status.getState() == JAISoundStatus_::State_LOCK_PREPARE);
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_PREPARE);
    status.setReadyLocked();
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_READY);
    std::puts("[pass] original JAISoundID layout and JAISoundStatus flags/prepare transitions");
    testStreamLifetime();
}
