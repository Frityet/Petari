#pragma once

#include <revolution.h>

constexpr u32 RFL_NAME_LEN = 10U;
constexpr u32 RFL_CREATOR_LEN = 10U;
constexpr u32 RFL_CREATEID_LEN = 8U;

enum RFLDataSource {
    RFLDataSource_Official,
    RFLDataSource_Hidden,
    RFLDataSource_Controller1,
    RFLDataSource_Controller2,
    RFLDataSource_Controller3,
    RFLDataSource_Controller4,
    RFLDataSource_Default,
    RFLDataSource_Middle,
    RFLDataSource_Max
};

enum RFLErrcode {
    RFLErrcode_Success,
    RFLErrcode_NotAvailable,
    RFLErrcode_NANDCommandfail,
    RFLErrcode_Loadfail,
    RFLErrcode_Savefail,
    RFLErrcode_Fatal,
    RFLErrcode_Busy,
    RFLErrcode_Broken,
    RFLErrcode_Exist,
    RFLErrcode_DBFull,
    RFLErrcode_DBNodata,
    RFLErrcode_Controllerfail,
    RFLErrcode_NWC24Fail,
    RFLErrcode_MaxFiles,
    RFLErrcode_MaxBlocks,
    RFLErrcode_WrongParam,
    RFLErrcode_NoFriends,
    RFLErrcode_Isolation,
    RFLErrcode_Unknown = 0xFF
};

enum RFLResolution {
    RFLResolution_64 = 64,
    RFLResolution_128 = 128,
    RFLResolution_256 = 256,
    RFLResolution_64M = 64 | 32,
    RFLResolution_128M = 128 | 64 | 32,
    RFLResolution_256M = 256 | 128 | 64 | 32
};

struct RFLCreateID {
    u8 data[RFL_CREATEID_LEN]{};
};

struct RFLMiddleDB;

struct RFLAdditionalInfo {
    u16 name[RFL_NAME_LEN + 1U]{};
    u16 creator[RFL_CREATOR_LEN + 1U]{};
    RFLCreateID createID{};
    u32 sex : 1;
    u32 bmonth : 4;
    u32 bday : 5;
    u32 color : 4;
    u32 favorite : 1;
    u32 height : 7;
    u32 build : 7;
    u32 reserved : 3;
    GXColor skinColor{};
};

extern "C" {
RFLErrcode RFLGetAdditionalInfo(RFLAdditionalInfo* info, RFLDataSource source, RFLMiddleDB* db, u16 index);
BOOL RFLSearchOfficialData(const RFLCreateID* id, u16* index);
BOOL RFLIsAvailableOfficialData(u16 index);
}
