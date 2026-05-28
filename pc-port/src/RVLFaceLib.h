#pragma once

#include <revolution.h>

constexpr u32 RFL_NAME_LEN = 10U;
constexpr u32 RFL_CREATOR_LEN = 10U;
constexpr u32 RFL_CREATEID_LEN = 8U;
constexpr u32 RFL_DB_CHAR_MAX = 100U;

using RFLCallback = void (*)(void);

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

enum RFLExpression {
    RFLExp_Normal,
    RFLExp_Smile,
    RFLExp_Anger,
    RFLExp_Sorrow,
    RFLExp_Surprise,
    RFLExp_Blink,
    RFLExp_OpenMouth,
    RFLExp_Max
};

enum RFLExpFlag {
    RFLExpFlag_Normal = 1U << RFLExp_Normal,
    RFLExpFlag_Smile = 1U << RFLExp_Smile,
    RFLExpFlag_Anger = 1U << RFLExp_Anger,
    RFLExpFlag_Sorrow = 1U << RFLExp_Sorrow,
    RFLExpFlag_Surprise = 1U << RFLExp_Surprise,
    RFLExpFlag_Blink = 1U << RFLExp_Blink,
    RFLExpFlag_OpenMouth = 1U << RFLExp_OpenMouth
};

enum RFLFavoriteColor {
    RFLFavoriteColor_Red,
    RFLFavoriteColor_Orange,
    RFLFavoriteColor_Yellow,
    RFLFavoriteColor_YellowGreen,
    RFLFavoriteColor_Green,
    RFLFavoriteColor_Blue,
    RFLFavoriteColor_SkyBlue,
    RFLFavoriteColor_Pink,
    RFLFavoriteColor_Purple,
    RFLFavoriteColor_Brown,
    RFLFavoriteColor_White,
    RFLFavoriteColor_Black,
    RFLFavoriteColor_Max
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

struct RFLCharModel {
    RFLDataSource source = RFLDataSource_Official;
    RFLMiddleDB *middleDB = nullptr;
    u16 index = 0U;
    RFLResolution resolution = RFLResolution_64;
    u32 expressionFlags = RFLExpFlag_Normal;
    RFLExpression expression = RFLExp_Normal;
    void *work = nullptr;
    Mtx matrix{};
    BOOL initialized = FALSE;
};

enum RFLIconBGType {
    RFLIconBG_Favorite,
    RFLIconBG_Direct
};

struct RFLIconSetting {
    u16 width = 0U;
    u16 height = 0U;
    RFLIconBGType bgType = RFLIconBG_Favorite;
    GXColor bgColor{};
    BOOL drawXluOnly = FALSE;
};

struct RFLDrawSetting {
    u8 lightEnable = 0U;
    u32 lightMask = 0U;
    u32 diffuse = 0U;
    u32 attn = 0U;
    GXColor ambColor{};
    u8 compLoc = 0U;
};

struct RFLDrawCoreSetting {
    u8 txcGenNum = 0U;
    u32 txcID = 0U;
    GXTexMapID texMapID = GX_TEXMAP0;
    u8 tevStageNum = 0U;
    u32 tevSwapTable = 0U;
    u32 tevKColorID = 0U;
    u32 tevOutRegID = 0U;
    u32 posNrmMtxID = 0U;
    u8 reverseCulling = 0U;
};

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
u32 RFLGetWorkSize(BOOL deluxeTex);
RFLErrcode RFLInitResAsync(void *workBuffer, void *resBuffer, u32 resSize, BOOL deluxeTex);
RFLErrcode RFLInitRes(void *workBuffer, void *resBuffer, u32 resSize, BOOL deluxeTex);
void RFLExit(void);
BOOL RFLAvailable(void);
RFLErrcode RFLGetAsyncStatus(void);
s32 RFLGetLastReason(void);
RFLErrcode RFLWaitAsync(void);

RFLErrcode RFLGetAdditionalInfo(RFLAdditionalInfo *info, RFLDataSource source, RFLMiddleDB *db, u16 index);
BOOL RFLSearchOfficialData(const RFLCreateID *id, u16 *index);
BOOL RFLIsAvailableOfficialData(u16 index);

u32 RFLGetModelBufferSize(RFLResolution resolution, u32 expressionFlags);
RFLErrcode RFLInitCharModel(RFLCharModel *model, RFLDataSource source, RFLMiddleDB *db, u16 index, void *work, RFLResolution resolution,
                            u32 expressionFlags);
void RFLSetMtx(RFLCharModel *model, const Mtx matrix);
void RFLSetExpression(RFLCharModel *model, RFLExpression expression);
RFLExpression RFLGetExpression(const RFLCharModel *model);
void RFLLoadDrawSetting(const RFLDrawSetting *setting);
void RFLDrawOpa(const RFLCharModel *model);
void RFLDrawXlu(const RFLCharModel *model);
void RFLLoadVertexSetting(const RFLDrawCoreSetting *setting);
void RFLLoadMaterialSetting(const RFLDrawCoreSetting *setting);
void RFLDrawOpaCore(const RFLCharModel *model, const RFLDrawCoreSetting *setting);
void RFLDrawXluCore(const RFLCharModel *model, const RFLDrawCoreSetting *setting);
void RFLDrawShape(const RFLCharModel *model);

RFLErrcode RFLMakeIcon(void *buffer, RFLDataSource source, RFLMiddleDB *db, u16 index, RFLExpression expression,
                       const RFLIconSetting *setting);
void RFLSetIconDrawDoneCallback(RFLCallback callback);
}

GXColor RFLGetFavoriteColor(RFLFavoriteColor color);
