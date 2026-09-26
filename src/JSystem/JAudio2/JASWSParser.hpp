#pragma once
#include <aurora/endian.hpp>

#include "JSystem/JSupport/JSupport.hpp"

class JKRHeap;
class JASWaveBank;
struct JASBasicWaveBank;
struct JASSimpleWaveBank;

class JASWSParser {
public:
    template < class T >
    class TOffset {
    public:
        T* ptr(void const* param_0) const {
            return JSUConvertOffsetToPtr< T >(param_0, mOffset);
        }

    private:
        /* 0x0 */ aurora::endian::AlignedBigEndian<u32> mOffset;
    };

    struct TCtrlWave {
        union {
            struct {
                /* 0x0 */ aurora::endian::AlignedBigEndian<u16> mWaveGroupId;
                /* 0x2 */ aurora::endian::AlignedBigEndian<u16> mWaveId;
            };
            /* 0x0 */ aurora::endian::AlignedBigEndian<u32> mData;
        };
    };

    struct TWave {
        /* 0x00 */ u8 _00;
        /* 0x01 */ u8 mFormat;
        /* 0x02 */ u8 mBaseKey;
        /* 0x04 */ aurora::endian::AlignedBigEndian<f32> mSampleRate;
        /* 0x08 */ aurora::endian::AlignedBigEndian<u32> mAWStartOffs;
        /* 0x0C */ aurora::endian::AlignedBigEndian<u32> mAWLength;
        /* 0x10 */ aurora::endian::AlignedBigEndian<u32> mLoopFlags;
        /* 0x14 */ aurora::endian::AlignedBigEndian<u32> mSampleLoopStart;
        /* 0x18 */ aurora::endian::AlignedBigEndian<u32> mSampleLoopEnd;
        /* 0x1C */ aurora::endian::AlignedBigEndian<u32> mSampleCount;
        /* 0x20 */ aurora::endian::AlignedBigEndian<s16> mpLastSample;
        /* 0x22 */ aurora::endian::AlignedBigEndian<s16> mpPenultSample;
    };

    struct TWaveArchive {
        /* 0x00 */ char mFileName[0x70];
        /* 0x70 */ aurora::endian::AlignedBigEndian<int> mWaveCount;
        /* 0x74 */ TOffset< TWave > mWaveOffsets[0];
    };

    struct TWaveArchiveBank {
        /* 0x0 */ char mMagic[4];
        /* 0x4 */ aurora::endian::AlignedBigEndian<int> mWaveGroupCount;
        /* 0x8 */ TOffset< TWaveArchive > mArchiveOffsets[0];
    };

    struct TCtrl {
        /* 0x0 */ char mMagic[4];
        /* 0x4 */ aurora::endian::AlignedBigEndian<u32> mWaveCount;
        /* 0x8 */ TOffset< TCtrlWave > mCtrlWaveOffsets[0];
    };

    struct TCtrlScene {
        /* 0x0 */ char mMagic[4];
        /* 0x4 */ u8 _04[8];
        /* 0xC */ TOffset< TCtrl > mCtrlOffset;
    };

    struct TCtrlGroup {
        /* 0x0 */ char mMagic[4];
        /* 0x4 */ u8 _04[4];
        /* 0x8 */ aurora::endian::AlignedBigEndian<u32> mGroupCount;
        /* 0xC */ TOffset< TCtrlScene > mCtrlSceneOffsets[0];
    };

    /** @fabricated */
    struct THeader {
        /* 0x00 */ char mMagic[4];
        /* 0x04 */ aurora::endian::AlignedBigEndian<int> mSize;
        /* 0x08 */ aurora::endian::AlignedBigEndian<int> mId;
        /* 0x0C */ aurora::endian::AlignedBigEndian<u32> mWaveTableSize;
        /* 0x10 */ TOffset< TWaveArchiveBank > mArchiveBankOffset;
        /* 0x14 */ TOffset< TCtrlGroup > mCtrlGroupOffset;
    };

    static u32 getGroupCount(void const*);
    static JASWaveBank* createWaveBank(void const*, JKRHeap*);
    static JASBasicWaveBank* createBasicWaveBank(void const*, JKRHeap*);
    static JASSimpleWaveBank* createSimpleWaveBank(void const*, JKRHeap*);

    static u32 sUsedHeapSize;
};
