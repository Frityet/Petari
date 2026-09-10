#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/GameSystemStationedArchiveLoader.hpp"
#include "Game/System/HeapMemoryWatcher.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"

namespace GameSystemFunction {
    void requestChangeArchivePlayer(bool isPlayerMario) {
        SingletonHolder<GameSystem>::get()->mStationedArchiveLoader->requestChangeArchivePlayer(isPlayerMario);
    }
    bool isEndChangeArchivePlayer() {
        return SingletonHolder<GameSystem>::get()->mStationedArchiveLoader->isDone();
    }
}

namespace MR {
    void requestChangeArchivePlayer(bool isPlayerMario) {
        GameSystemFunction::requestChangeArchivePlayer(isPlayerMario);
    }
    void waitEndChangeArchivePlayer() {
        while (!GameSystemFunction::isEndChangeArchivePlayer()) {
        }
    }
    JKRExpHeap* getStationedHeapNapa() {
        return SingletonHolder<HeapMemoryWatcher>::get()->mStationedHeapNapa;
    }
    JKRExpHeap* getStationedHeapGDDR3() {
        return SingletonHolder<HeapMemoryWatcher>::get()->mStationedHeapGDDR;
    }
    JKRSolidHeap* getSceneHeapNapa() {
        return SingletonHolder<HeapMemoryWatcher>::get()->mSceneHeapNapa;
    }
    JKRHeap* getHeapNapa(const JKRHeap* heap) {
        return SingletonHolder<HeapMemoryWatcher>::get()->getHeapNapa(heap);
    }
    JKRHeap* getHeapGDDR3(const JKRHeap* heap) {
        return SingletonHolder<HeapMemoryWatcher>::get()->getHeapGDDR3(heap);
    }
    JKRHeap* getAproposHeapForSceneArchive(f32 maxFreeSizeRate) {
        JKRHeap* fileCacheHeap = SingletonHolder<HeapMemoryWatcher>::get()->mFileCacheHeap;
        if (fileCacheHeap != nullptr) {
            f32 freeSize = fileCacheHeap->getTotalFreeSize();
            f32 size = fileCacheHeap->mSize;
            f32 workFreeSizeRate = freeSize / size;
            if (workFreeSizeRate < maxFreeSizeRate)
                return SingletonHolder<HeapMemoryWatcher>::get()->mSceneHeapGDDR;
        }
        return fileCacheHeap;
    }
    void adjustHeapSize(JKRExpHeap* heap, const char*) {
        heap->adjustSize();
    }
}
