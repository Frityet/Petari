#include "JSystem/JKernel/JKRAram.hpp"
#include "JSystem/JKernel/JKRAramPiece.hpp"
#include "JSystem/JKernel/JKRAramStream.hpp"
#include "JSystem/JKernel/JKRDecomp.hpp"
#include "JSystem/JKernel/JKRArchive.hpp"
#include "JSystem/JKernel/JKRDvdFile.hpp"
#include "JSystem/JSupport/JSUFileStream.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <aurora/guest_thread.hpp>
#include <aurora/allocation.hpp>
#include <dolphin/ar.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class MemoryFile final : public JKRFile {
public:
    explicit MemoryFile(std::span<const u8> data) : data_(data) { mIsAvailable = true; }
    bool open(const char*) override { return mIsAvailable; }
    void close() override { mIsAvailable = false; }
    s32 getFileSize() const override { return data_.size(); }
    s32 writeData(const void*, s32, s32) override { return -1; }
    s32 readData(void* output, s32 size, s32 offset) override {
        if (!mIsAvailable || offset < 0 || size < 0 || static_cast<std::size_t>(offset) > data_.size()) return -1;
        const auto count = std::min<std::size_t>(size, data_.size() - offset);
        std::memcpy(output, data_.data() + offset, count);
        return count;
    }
private:
    std::span<const u8> data_;
};

struct AllocatingCallback {
    OSMessageQueue proceed{};
    OSMessage message{};
    alignas(32) std::array<u8, 32> input{};
    bool entered = false;
    bool retirementStarted = false;
    bool allocated = false;
};
AllocatingCallback* allocatingCallback;

void allocate_during_retirement() {
    auto& state = *allocatingCallback;
    state.entered = true;
    OSMessage message;
    OSReceiveMessage(&state.proceed, &message, OS_MESSAGE_BLOCK);
    require(state.retirementStarted, "retirement must be waiting for the borrowed callback");
    auto* value = new u32(0x12345678);
    state.allocated = JKRHeap::getRootHeap()->find(value) && *value == 0x12345678;
    delete value;
}

void* transfer_while_callback_waits(void* argument) {
    auto& state = *static_cast<AllocatingCallback*>(argument);
    ARStartDMA(ARAM_DIR_MRAM_TO_ARAM, reinterpret_cast<uintptr_t>(state.input.data()), 0x4000, state.input.size());
    return nullptr;
}
void* release_retiring_callback(void* argument) {
    auto& state = *static_cast<AllocatingCallback*>(argument);
    require(state.retirementStarted, "lower-priority release worker must wait until retirement blocks");
    OSSendMessage(&state.proceed, &state, OS_MESSAGE_BLOCK);
    return nullptr;
}

void retire_with_allocating_callback(std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    AllocatingCallback state;
    allocatingCallback = &state;
    OSInitMessageQueue(&state.proceed, &state.message, 1);
    OSThread transfer{}, release{};
    alignas(32) std::array<u8, 0x8000> transferStack{}, releaseStack{};
    {
        const aurora::allocation::ClientAllocationScope allocations({true, true});
        ARRegisterDMACallback(&allocate_during_retirement);
        require(OSCreateThread(&transfer, &transfer_while_callback_waits, &state,
                    transferStack.data() + transferStack.size(), transferStack.size(), 15, 0),
                "actual transfer worker required");
    }
    require(OSCreateThread(&release, &release_retiring_callback, &state,
                releaseStack.data() + releaseStack.size(), releaseStack.size(), 17, 0),
            "actual retirement acknowledgement worker required");
    OSResumeThread(&transfer);
    require(state.entered, "higher-priority DMA callback must start before resume returns");
    OSResumeThread(&release);
    state.retirementStarted = true;
    heaps.reset();
    require(state.allocated, "callback allocation must finish before the root heap is locked and released");
    require(OSJoinThread(&transfer, nullptr) && OSJoinThread(&release, nullptr),
            "callback retirement must leave both actual workers joinable");
    allocatingCallback = nullptr;
}

unsigned dmaCallbacks;
unsigned arqCallbacks;
void dma_done() { ++dmaCallbacks; }
void arq_done(uintptr_t request) {
    require(reinterpret_cast<ARQRequest*>(request)->owner == 73, "ARQ callback must preserve actual request identity");
    ++arqCallbacks;
}

void transfers(JKRAram& manager) {
    alignas(32) std::array<u8, 96> input;
    alignas(32) std::array<u8, 96> output{};
    for (std::size_t i = 0; i < input.size(); ++i) input[i] = static_cast<u8>(i * 37 + 11);
    auto* block = JKRAllocFromAram(input.size(), JKRAramHeap::HEAD);
    auto* tail = JKRAllocFromAram(33, JKRAramHeap::TAIL);
    require(block != nullptr && tail != nullptr && block->getAddress() == manager.getGraphMemory(),
            "actual original ARAM heap must allocate from its reserved graph range");
    require(tail->getSize() == 64 && tail->isTempMemory(), "tail allocation must retain its original rounding and classification");
    require(JKRAramPcs(0, reinterpret_cast<uintptr_t>(input.data()), block->getAddress(), input.size(), block),
            "original JKR worker must acknowledge a completed write");
    ARStartDMA(ARAM_DIR_ARAM_TO_MRAM, block->getAddress(), reinterpret_cast<uintptr_t>(output.data()), output.size());
    require(input == output, "actual retained MEM2 bytes must round-trip through the SDK transfer boundary");

    dmaCallbacks = arqCallbacks = 0;
    ARRegisterDMACallback(&dma_done);
    output.fill(0);
    ARStartDMA(ARAM_DIR_ARAM_TO_MRAM, block->getAddress(), reinterpret_cast<uintptr_t>(output.data()), output.size());
    require(input == output && dmaCallbacks == 1, "DMA callback must follow the completed real copy");
    ARQRequest request{};
    ARQPostRequest(&request, 73, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH, block->getAddress(),
                   reinterpret_cast<uintptr_t>(output.data()), output.size(), &arq_done);
    require(arqCallbacks == 1 && request.dest == reinterpret_cast<uintptr_t>(output.data()),
            "ARQ request must preserve host address width");
    const auto before = output;
    bool rejected = false;
    try {
        ARQPostRequest(&request, 73, ARQ_TYPE_ARAM_TO_MRAM, ARQ_PRIORITY_HIGH, ARGetSize() - 32,
                       reinterpret_cast<uintptr_t>(output.data()), output.size(), &arq_done);
    } catch (const std::out_of_range&) { rejected = true; }
    require(rejected && arqCallbacks == 1 && output == before,
            "out-of-range ARAM must reject the full transfer without changing bytes or reporting completion");
    ARRegisterDMACallback(nullptr);

    MemoryFile file(input);
    JSUFileInputStream stream(&file);
    JKRSetAramTransferBuffer(nullptr, 32, manager.mHeap);
    u32 written = 0;
    auto* command = JKRStreamToAram_Async(&stream, block->getAddress(), input.size(), 0, nullptr, &written);
    require(JKRStreamToAram_Sync(command, FALSE) == command && written == input.size(),
            "original stream worker must finish every real file chunk");
    delete command;
    output.fill(0);
    ARStartDMA(ARAM_DIR_ARAM_TO_MRAM, block->getAddress(), reinterpret_cast<uintptr_t>(output.data()), output.size());
    require(output == input && stream.getPosition() == static_cast<s32>(input.size()),
            "original file stream and ARAM worker must transfer identical data");
    JKRFreeToAram(tail);
    JKRFreeToAram(block);
    require(JKRAramHeap::sAramList.getNumLinks() == 1,
            "freeing graph allocations must coalesce into the original sentinel");
}

void decompression() {
    const std::array<u8, 24> yaz{'Y','a','z','0',0,0,0,10,0,0,0,0,0,0,0,0,0xe8,'A','B','C',0x40,2,'!',0};
    std::array<u8, 10> output{};
    require(JKRDecomp::orderSync(const_cast<u8*>(yaz.data()), output.data(), output.size(), 0),
            "actual decompression worker must acknowledge completion");
    require(std::memcmp(output.data(), "ABCABCABC!", output.size()) == 0,
            "original Yaz0 literal/back-reference decoding must preserve its output");
    const std::array<u8, 28> yay{'Y','a','y','0',0,0,0,8,0,0,0,20,0,0,0,20,0xff,0,0,0,'S','Y','S','T','E','M','!','!'};
    require(JKRDecomp::orderSync(const_cast<u8*>(yay.data()), output.data(), 8, 0),
            "actual Yay0 work must complete through the same worker");
    require(std::memcmp(output.data(), "SYSTEM!!", 8) == 0, "original Yay0 tables must decode with Wii byte order");

    const auto fetch = [](std::span<const u8> source, const char* expected, u32 expanded, int compression) {
        for (const u32 capacity : {u32(3), expanded, expanded + 4}) {
            std::array<u8, 20> guarded;
            guarded.fill(0xA5);
            const auto count = std::min(expanded, capacity);
            require(JKRMemArchive::fetchResource_subroutine(const_cast<u8*>(source.data()), source.size(),
                        guarded.data() + 1, capacity, compression) == count,
                    "actual archive fetch must report the bounded decoded byte count");
            require(guarded.front() == 0xA5 && std::memcmp(guarded.data() + 1, expected, count) == 0 &&
                        std::all_of(guarded.begin() + 1 + count, guarded.end(), [](u8 byte) { return byte == 0xA5; }),
                    "archive fetch must preserve decoded content and bytes outside the reported output");
        }
    };
    const std::array<u8, 6> plain{'A','R','C','H','I','V'};
    fetch(plain, "ARCHIV", plain.size(), JKR_COMPRESSION_NONE);
    fetch(yay, "SYSTEM!!", 8, JKR_COMPRESSION_SZP);
    fetch(yaz, "ABCABCABC!", 10, JKR_COMPRESSION_SZS);
}

void cycle(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps, bool originalSizes) {
    using namespace smgpc::compat;
    auto domain = JkrAllocationDomain::create(heaps, 512U * 1024U);
    auto* oldSystem = domain->heap().becomeSystemHeap();
    const auto oldCount = JKRThread::sThreadList.getNumLinks();
    JKRAram* manager;
    {
        const JkrAllocationScope allocation(domain);
        manager = JKRAram::create(originalSizes ? 0xE00000 : 0x100000, 0xFFFFFFFF, 8, 7, 3);
    }
    require(JKRAram::getManager() == manager && JKRAramStream::getManager() != nullptr && JKRDecomp::getManager() != nullptr,
            "original create must publish all three actual SDK workers");
    require(JKRThread::sThreadList.getNumLinks() == oldCount + 3,
            "manager creation must retain precisely its actual worker cohort");
    require(ARGetStorageAddress() == OSGetMEM2ArenaLo(), "ARAM must borrow the original retained MEM2 arena");
    if (originalSizes) {
        require(manager->getGraphMemSize() == 0xFFFFC000 && manager->getAudioMemSize() == 0xE00000,
                "actual GameSystem arguments must preserve original 32-bit ARAM accounting");
    } else {
        transfers(*manager);
        decompression();
    }
    // All issued commands above have completed before their borrowed buffers,
    // queues and owner heaps are reclaimed. Base destruction joins cancellation.
    delete static_cast<JKRThread*>(JKRAramStream::getManager());
    delete JKRDecomp::getManager();
    delete manager;
    require(JKRAram::getManager() == nullptr && JKRAramStream::getManager() == nullptr && JKRDecomp::getManager() == nullptr,
            "actual manager destruction must retire its singleton identities");
    require(JKRThread::sThreadList.getNumLinks() == oldCount && JKRAramHeap::sAramList.getNumLinks() == 0,
            "manager retirement must leave no worker or ARAM block list entries");
    ARReset();
    oldSystem->becomeSystemHeap();
}
}

int main() {
    try {
        const aurora::os::GuestThreadExecutionScope execution;
        OSInit();
        auto heaps = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
        heaps->prepare_mem2_arena(18U * 1024U * 1024U);
        const auto originalFree = heaps->root_heap().getTotalFreeSize();
        auto* arenaEnd = OSGetMEM2ArenaHi();
        for (unsigned i = 0; i < 3; ++i) {
            cycle(heaps, false);
            require(heaps->root_heap().getTotalFreeSize() == originalFree, "ARAM worker cohort must reclaim its full heap");
        }
        OSSetMEM2ArenaHi(static_cast<u8*>(OSGetMEM2ArenaLo()) + 0xE00000);
        cycle(heaps, true);
        OSSetMEM2ArenaHi(arenaEnd);
        std::array<u32, 1> lengths{};
        ARInit(lengths.data(), lengths.size());
        retire_with_allocating_callback(heaps);
        require(!ARCheckInit() && ARGetStorageAddress() == nullptr,
                "releasing the MEM2 owner must clear every borrowed ARAM address");
        std::cout << "Original ARAM workers, file streaming, Yaz0/Yay0, real memory transfers and repeated retirement passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
