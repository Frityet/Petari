#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "JSystem/JKernel/JKRDecomp.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/allocation.hpp>
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    using aurora::endian::read_big;
    using Bytes = std::span<const std::uint8_t>;

    void require_range(Bytes data, std::size_t offset, std::size_t size) {
        if (offset > data.size() || size > data.size() - offset) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive metadata extends outside its retained resource");
        }
    }
}  // namespace

JKRMemArchive::JKRMemArchive() : JKRArchive(nullptr) {
}

JKRMemArchive::JKRMemArchive(const smgpc::resource::RarcArchive &archive)
    : JKRArchive(&archive), mHeader(reinterpret_cast<RarcHeader *>(const_cast<u8 *>(archive.bytes().data()))),
      mFileDataStart(const_cast<u8 *>(archive.file_data_start())) {
    publish_mount(archive.bytes().data());
}

JKRMemArchive::JKRMemArchive(smgpc::resource::RarcArchive &&archive) : JKRArchive(nullptr) {
    aurora::allocation::HostAllocationScope host;
    auto owned = std::make_shared<smgpc::resource::RarcArchive>(std::move(archive));
    attach_archive(owned);
    mOwnedArchive = std::move(owned);
    mHeader = reinterpret_cast<RarcHeader *>(const_cast<u8 *>(mOwnedArchive->bytes().data()));
    mFileDataStart = const_cast<u8 *>(mOwnedArchive->file_data_start());
    publish_mount(mOwnedArchive->bytes().data());
}

void JKRMemArchive::publish_mount(const void *identity) {
    VolumeLock lock;
    mEntryNum = reinterpret_cast<std::uintptr_t>(identity);
    mMountMode = MOUNT_MODE_MEM;
    mLoaderType = 0x52415243;
    _34 = 1;
    if (gCurrentFileLoader == nullptr) {
        gCurrentFileLoader = this;
        sCurrentDirID = 0;
    }
    prependVolumeList(&mLoaderLink);
    mIsMounted = true;
}

JKRMemArchive::~JKRMemArchive() {
    aurora::allocation::HostAllocationScope host;
    if (mIsMounted) {
        removeVolumeList(&mLoaderLink);
        mIsMounted = false;
    }
    // Parsed metadata borrows the fixed buffer; discard it before releasing
    // ownership through the original heap that allocated the supplied bytes.
    attach_archive(nullptr);
    mOwnedArchive.reset();
    if (_6C && mHeader != nullptr)
        JKRHeap::free(mHeader, mHeap);
    mHeader = nullptr;
    mFileDataStart = nullptr;
}

s32 JKRMemArchive::fetchResource_subroutine(u8 *pSrc, u32 srcSize, u8 *pDst, u32 dstSize, int compression) {
    switch (compression) {
    case JKR_COMPRESSION_NONE:
        if (srcSize > dstSize) {
            srcSize = dstSize;
        }
        std::memcpy(pDst, pSrc, srcSize);
        return srcSize;
    case JKR_COMPRESSION_SZP:
    case JKR_COMPRESSION_SZS: {
        u32 size = JKRDecompExpandSize(pSrc);
        if (size > dstSize) {
            size = dstSize;
        }
        JKRDecomp::orderSync(pSrc, pDst, size, 0);
        return size;
    }
    default:
        OSPanic(__FILE__, 723, "??? bad sequence\n");
        break;
    }
    return 0;
}

bool JKRMemArchive::mountFixed(void *data, JKRMemBreakFlag breakFlag) {
    if (data == nullptr)
        return false;
    // This SDK entry point has no length argument: its caller supplies a valid
    // RARC header and the declared buffer. Native callers can use the span API.
    const Bytes header(static_cast<const u8 *>(data), sizeof(RarcHeader));
    return mountFixed(Bytes(static_cast<const u8 *>(data), read_big<u32>(header, 4)), breakFlag);
}

bool JKRMemArchive::mountFixed(Bytes bytes, JKRMemBreakFlag breakFlag) {
    aurora::allocation::HostAllocationScope host;
    VolumeLock lock;
    if (bytes.data() == nullptr)
        return false;
    if (check_mount_already(reinterpret_cast<std::uintptr_t>(bytes.data())) != nullptr)
        return false;
    if (mIsMounted)
        return false;
    require_range(bytes, 0, sizeof(RarcHeader));
    const u32 size = read_big<u32>(bytes, 4);
    require_range(bytes, 0, size);
    if (size < 0x40)
        aurora::throw_host_exception<std::invalid_argument>("Fixed archive is smaller than its RARC header and info block");
    auto *heap = JKRHeap::findFromRoot(const_cast<u8 *>(bytes.data()));
    if (breakFlag == JKR_MEM_BREAK_FLAG_1 && heap == nullptr)
        aurora::throw_host_exception<std::invalid_argument>("Owned fixed archives require an actual JKR buffer allocation");
    auto parsed = std::make_shared<smgpc::resource::RarcArchive>(
        smgpc::resource::RarcArchive::from_borrowed(bytes.first(size)));
    attach_archive(parsed);
    mOwnedArchive = std::move(parsed);
    mHeap = heap;
    mHeader = reinterpret_cast<RarcHeader *>(const_cast<u8 *>(bytes.data()));
    mFileDataStart = const_cast<u8 *>(mOwnedArchive->file_data_start());
    _6C = breakFlag == JKR_MEM_BREAK_FLAG_1;
    publish_mount(bytes.data());
    return true;
}

void *JKRMemArchive::fetchResource(SDIFileEntry *pFile, u32 *pSize) {
    if (pFile->mFileData == nullptr) {
        pFile->mFileData = mFileDataStart + pFile->mDataOffset;
    }

    if (pSize != nullptr) {
        *pSize = pFile->mDataSize;
    }

    return pFile->mFileData;
}
