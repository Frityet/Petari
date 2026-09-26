#include "JSystem/JAudio2/JAISound.hpp"
#include "JSystem/JAudio2/JAISoundChild.hpp"
#include "JSystem/JAudio2/JAIStreamDataMgr.hpp"
#include "JSystem/JAudio2/JAIStreamMgr.hpp"
#include "JSystem/JAudio2/JAUSeqCollection.hpp"
#include "JSystem/JAudio2/JAUSoundTable.hpp"
#include "NativeHeapFixture.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

static void testOriginalResourceByteOrder() {
    static_assert(sizeof(JAUSoundTableItem) == 4 && sizeof(JAUSoundTableSe) == 6);
    static_assert(sizeof(JAUSoundTableBgm) == 8 && sizeof(JAUSoundTableStream) == 8);
    static_assert(sizeof(JAUSeqCollectionData) == 12);
    // These are serialized Wii bytes, never native-endian structure fixtures.
    alignas(4) const u8 soundBytes[] = {
        'B', 'S', 'T', ' ', 0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 16,
        0, 0, 0, 1, 0, 0, 0, 24,
        0, 0, 0, 1, 0, 0, 0, 32,
        0, 0, 0, 3, 0, 0, 0, 0,
        0x50, 0, 0, 52, 0x60, 0, 0, 60, 0x70, 0, 0, 68,
        127, 254, 0x12, 0x34, 0x56, 0x78, 0, 0,
        123, 234, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd,
        111, 222, 0x34, 0x56, 0, 0, 0, 76, 'a', 's', 't', 0,
    };
    JAUSoundTable sounds(false);
    sounds.init(soundBytes);
    assert(sounds.isValid() && sounds.getResource() == soundBytes);
    assert(sounds.getTypeID(JAISoundID(0, 0, 0)) == 0x50);
    assert(sounds.getTypeID(JAISoundID(0, 0, 1)) == 0x60);
    assert(sounds.getTypeID(JAISoundID(0, 0, 2)) == 0x70);
    const auto *se = reinterpret_cast<const JAUSoundTableSe *>(sounds.getData(JAISoundID(0, 0, 0)));
    const auto *bgm = reinterpret_cast<const JAUSoundTableBgm *>(sounds.getData(JAISoundID(0, 0, 1)));
    const auto *stream = reinterpret_cast<const JAUSoundTableStream *>(sounds.getData(JAISoundID(0, 0, 2)));
    assert(se->mAudibleSw == 0x1234 && se->mSoundSw == 0x5678 && se->mVolume == 254);
    assert(bgm->mResourceId == 0x2345 && bgm->mChordResId == 0x6789 && bgm->mSoundSw == 0xabcd);
    assert(stream->mChannelCtrl == 0x3456 && stream->mStreamFileNameOffset == 76);
    assert(sounds.getData(JAISoundID(0, 0, 3)) == nullptr);
    assert(sounds.getData(JAISoundID(0, 1, 0)) == nullptr);
    assert(sounds.getData(JAISoundID(1, 0, 0)) == nullptr);

    alignas(4) const u8 nameBytes[] = {
        'B', 'S', 'T', 'N', 0, 0, 0, 56, 0, 0, 0, 0, 0, 0, 0, 16,
        0, 0, 0, 1, 0, 0, 0, 24,
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 36,
        0, 0, 0, 1, 0, 0, 0, 48, 0, 0, 0, 52,
        'c', 'a', 't', 0, 's', 'e', '1', 0,
    };
    JAUSoundNameTable names(false);
    names.init(nameBytes);
    assert(names.getNumGroups_inSection(0) == 1 && names.getNumItems_inGroup(0, 0) == 1);
    assert(std::strcmp(names.getName(JAISoundID(0, 0, 0)), "se1") == 0);
    assert(names.getNumGroups_inSection(1) == -1 && names.getName(JAISoundID(0, 0, 1)) == nullptr);

    alignas(4) const u8 sequenceBytes[] = {
        'S', 'C', 0, 2, 0, 0, 0, 40, 0, 0, 0, 16, 0, 0, 0, 24,
        0, 0, 0, 1, 0, 0, 0, 36,
        0, 0, 0, 2, 0, 0, 0, 37, 0, 0, 0, 39,
        0xff, 0x80, 0x02, 0xff,
    };
    JAUSeqCollection sequences;
    sequences.init(sequenceBytes);
    JAISeqData data(nullptr, 0);
    assert(sequences.getSeqData(0, 0, &data) && data.data == sequenceBytes && data.offset == 36);
    assert(sequences.getSeqData(1, 1, &data) && data.data == sequenceBytes && data.offset == 39);
    assert(!sequences.getSeqData(2, 0, &data) && !sequences.getSeqData(0, 1, &data));
    JAISeqDataRegion region;
    assert(sequences.getSeqDataRegion(&region) && region.addr == sequenceBytes && region.size == sizeof(sequenceBytes));
    std::puts("[pass] Original BST, BSTN and BSC readers consume unchanged big-endian resources");
}

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
    testOriginalResourceByteOrder();
    testStreamLifetime();
}
