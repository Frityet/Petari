#pragma once

#include "revolution/types.h"

#ifndef NO_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define NO_INLINE __attribute__((noinline))
#else
#define NO_INLINE
#endif
#endif

constexpr s32 WPAD_CHAN0 = 0;
constexpr s32 WPAD_CHAN1 = 1;
constexpr s32 WPAD_CHAN2 = 2;
constexpr s32 WPAD_CHAN3 = 3;
constexpr s32 WPAD_MAX_CONTROLLERS = 4;

constexpr u32 WPAD_BUTTON_LEFT = 0x0001;
constexpr u32 WPAD_BUTTON_RIGHT = 0x0002;
constexpr u32 WPAD_BUTTON_DOWN = 0x0004;
constexpr u32 WPAD_BUTTON_UP = 0x0008;
constexpr u32 WPAD_BUTTON_PLUS = 0x0010;
constexpr u32 WPAD_BUTTON_2 = 0x0100;
constexpr u32 WPAD_BUTTON_1 = 0x0200;
constexpr u32 WPAD_BUTTON_B = 0x0400;
constexpr u32 WPAD_BUTTON_A = 0x0800;
constexpr u32 WPAD_BUTTON_MINUS = 0x1000;
constexpr u32 WPAD_BUTTON_Z = 0x2000;
constexpr u32 WPAD_BUTTON_C = 0x4000;
constexpr u32 WPAD_BUTTON_HOME = 0x8000;
constexpr u32 KPAD_BUTTON_MASK = 0x0000ffff;
constexpr u32 KPAD_BUTTON_RPT = 0x80000000;

struct _GXColor {
    u8 r = 0U;
    u8 g = 0U;
    u8 b = 0U;
    u8 a = 0U;
};

using GXColor = _GXColor;
using GXBool = BOOL;

constexpr GXBool GX_FALSE = 0;
constexpr GXBool GX_TRUE = 1;

enum _GXTexFmt : u32 {
    _GX_TF_CTF = 0x20,
    _GX_TF_ZTF = 0x10,
    GX_TF_I4 = 0x0,
    GX_TF_I8 = 0x1,
    GX_TF_IA4 = 0x2,
    GX_TF_IA8 = 0x3,
    GX_TF_RGB565 = 0x4,
    GX_TF_RGB5A3 = 0x5,
    GX_TF_RGBA8 = 0x6,
    GX_TF_C4 = 0x8,
    GX_TF_C8 = 0x9,
    GX_TF_C14X2 = 0xA,
    GX_TF_CMPR = 0xE,
    GX_CTF_R4 = 0x0 | _GX_TF_CTF,
    GX_CTF_RA4 = 0x2 | _GX_TF_CTF,
    GX_CTF_RA8 = 0x3 | _GX_TF_CTF,
    GX_CTF_YUVA8 = 0x6 | _GX_TF_CTF,
    GX_CTF_A8 = 0x7 | _GX_TF_CTF,
    GX_CTF_R8 = 0x8 | _GX_TF_CTF,
    GX_CTF_G8 = 0x9 | _GX_TF_CTF,
    GX_CTF_B8 = 0xA | _GX_TF_CTF,
    GX_CTF_RG8 = 0xB | _GX_TF_CTF,
    GX_CTF_GB8 = 0xC | _GX_TF_CTF,
    GX_TF_Z8 = 0x1 | _GX_TF_ZTF,
    GX_TF_Z16 = 0x3 | _GX_TF_ZTF,
    GX_TF_Z24X8 = 0x6 | _GX_TF_ZTF,
    GX_CTF_Z4 = 0x0 | _GX_TF_ZTF | _GX_TF_CTF,
    GX_CTF_Z8M = 0x9 | _GX_TF_ZTF | _GX_TF_CTF,
    GX_CTF_Z8L = 0xA | _GX_TF_ZTF | _GX_TF_CTF,
    GX_CTF_Z16L = 0xC | _GX_TF_ZTF | _GX_TF_CTF,
    GX_TF_A8 = GX_CTF_A8,
};

using GXTexFmt = _GXTexFmt;

enum _GXTlut : u32 {
    GX_TLUT0 = 0,
    GX_TLUT1,
    GX_TLUT2,
    GX_TLUT3,
    GX_TLUT4,
    GX_TLUT5,
    GX_TLUT6,
    GX_TLUT7,
    GX_TLUT8,
    GX_TLUT9,
    GX_TLUT10,
    GX_TLUT11,
    GX_TLUT12,
    GX_TLUT13,
    GX_TLUT14,
    GX_TLUT15,
    GX_BIGTLUT0,
    GX_BIGTLUT1,
    GX_BIGTLUT2,
    GX_BIGTLUT3,
};

using GXTlut = _GXTlut;

enum _GXTlutFmt : u32 {
    GX_TL_IA8 = 0x0,
    GX_TL_RGB565 = 0x1,
    GX_TL_RGB5A3 = 0x2,
};

using GXTlutFmt = _GXTlutFmt;

enum _GXTexMapID : u32 {
    GX_TEXMAP0 = 0,
    GX_TEXMAP1,
    GX_TEXMAP2,
    GX_TEXMAP3,
    GX_TEXMAP4,
    GX_TEXMAP5,
    GX_TEXMAP6,
    GX_TEXMAP7,
    GX_MAX_TEXMAP,
    GX_TEXMAP_NULL = 0xff,
    GX_TEX_DISABLE = 0x100,
};

using GXTexMapID = _GXTexMapID;

enum _GXTexWrapMode : u32 {
    GX_CLAMP = 0,
    GX_REPEAT,
    GX_MIRROR,
};

using GXTexWrapMode = _GXTexWrapMode;

enum _GXTexFilter : u32 {
    GX_NEAR = 0,
    GX_LINEAR,
    GX_NEAR_MIP_NEAR,
    GX_LIN_MIP_NEAR,
    GX_NEAR_MIP_LIN,
    GX_LIN_MIP_LIN,
};

using GXTexFilter = _GXTexFilter;

enum _GXAnisotropy : u32 {
    GX_ANISO_1 = 0,
    GX_ANISO_2,
    GX_ANISO_4,
};

using GXAnisotropy = _GXAnisotropy;

struct _GXTexObj {
    u32 dummy[8]{};
};

using GXTexObj = _GXTexObj;

struct _GXRenderModeObj {
    u32 viTVmode = 0U;
    u16 fbWidth = 640U;
    u16 efbHeight = 456U;
    u16 xfbHeight = 456U;
    u16 viXOrigin = 0U;
    u16 viYOrigin = 0U;
    u16 viWidth = 640U;
    u16 viHeight = 456U;
    u32 xFBmode = 0U;
    u8 field_rendering = 0U;
    u8 aa = 0U;
    u8 sample_pattern[12][2]{};
    u8 vfilter[7]{};
};

using GXRenderModeObj = _GXRenderModeObj;

enum _GXFBClamp : u32 {
    GX_CLAMP_NONE = 0,
    GX_CLAMP_TOP = 1,
    GX_CLAMP_BOTTOM = 2,
};

using GXFBClamp = _GXFBClamp;

enum _GXGamma : u32 {
    GX_GM_1_0 = 0,
    GX_GM_1_7 = 1,
    GX_GM_2_2 = 2,
};

using GXGamma = _GXGamma;

enum _GXCopyMode : u32 {
    GX_COPY_PROGRESSIVE = 0,
    GX_COPY_INTLC_EVEN = 2,
    GX_COPY_INTLC_ODD = 3,
};

using GXCopyMode = _GXCopyMode;

enum _GXLightID : u32 {
    GX_LIGHT_NULL = 0x000,
    GX_LIGHT0 = 0x001,
    GX_LIGHT1 = 0x002,
    GX_LIGHT2 = 0x004,
    GX_LIGHT3 = 0x008,
    GX_LIGHT4 = 0x010,
    GX_LIGHT5 = 0x020,
    GX_LIGHT6 = 0x040,
    GX_LIGHT7 = 0x080,
    GX_MAX_LIGHT = 0x100,
};

using GXLightID = _GXLightID;

constexpr s32 WPAD_ERR_NONE = 0;
constexpr s32 WPAD_ERR_NO_CONTROLLER = -1;

constexpr s32 NAND_RESULT_OK = 0;
constexpr s32 NAND_RESULT_ACCESS = -1;
constexpr s32 NAND_RESULT_ALLOC_FAILED = -2;
constexpr s32 NAND_RESULT_BUSY = -3;
constexpr s32 NAND_RESULT_CORRUPT = -4;
constexpr s32 NAND_RESULT_ECC_CRIT = -5;
constexpr s32 NAND_RESULT_EXISTS = -6;
constexpr s32 NAND_RESULT_INVALID = -8;
constexpr s32 NAND_RESULT_MAXBLOCKS = -9;
constexpr s32 NAND_RESULT_MAXFD = -10;
constexpr s32 NAND_RESULT_MAXFILES = -11;
constexpr s32 NAND_RESULT_NOEXISTS = -12;
constexpr s32 NAND_RESULT_NOTEMPTY = -13;
constexpr s32 NAND_RESULT_OPENFD = -14;
constexpr s32 NAND_RESULT_AUTHENTICATION = -15;
constexpr s32 NAND_RESULT_MAXDEPTH = -16;
constexpr s32 NAND_RESULT_UNKNOWN = -64;
constexpr s32 NAND_RESULT_FATAL_ERROR = -128;

struct KPADVec2 {
    f32 x = 0.0F;
    f32 y = 0.0F;
};

struct KPADVec3 {
    f32 x = 0.0F;
    f32 y = 0.0F;
    f32 z = 0.0F;
};

struct KPADStatus {
    u32 hold = 0U;
    u32 trig = 0U;
    u32 release = 0U;
    KPADVec3 acc{};
    f32 acc_value = 0.0F;
    f32 acc_speed = 0.0F;
    KPADVec2 pos{};
    KPADVec2 vec{};
    f32 speed = 0.0F;
    KPADVec2 horizon{};
    KPADVec2 hori_vec{};
    f32 hori_speed = 0.0F;
    f32 dist = 0.0F;
    f32 dist_vec = 0.0F;
    f32 dist_speed = 0.0F;
    KPADVec2 acc_vertical{};
    s32 wpad_err = WPAD_ERR_NO_CONTROLLER;
    s32 dpd_valid_fg = 0;
};

struct DVDFileInfo {
    s32 entry_num = -1;
    u32 length = 0U;
    u32 position = 0U;
    void *internal = nullptr;
};

[[nodiscard]] OSTime OSGetTime();
[[nodiscard]] s64 OSTicksToSeconds(OSTime ticks);
[[nodiscard]] s32 KPADRead(s32 channel, KPADStatus sampling_bufs[], u32 length);
[[nodiscard]] s32 DVDConvertPathToEntrynum(const char *path);
[[nodiscard]] BOOL DVDOpen(const char *path, DVDFileInfo *file_info);
[[nodiscard]] BOOL DVDClose(DVDFileInfo *file_info);
[[nodiscard]] u32 DVDGetLength(const DVDFileInfo *file_info);
[[nodiscard]] s32 DVDReadPrio(DVDFileInfo *file_info, void *destination, s32 length, s32 offset, s32 priority);
void GXSetDispCopySrc(u16 left, u16 top, u16 width, u16 height);
void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height);
void GXSetDispCopyDst(u16 width, u16 height);
void GXSetTexCopyDst(u16 width, u16 height, GXTexFmt format, GXBool mipmap);
void GXSetDispCopyFrame2Field(GXCopyMode mode);
void GXSetCopyClamp(GXFBClamp clamp);
[[nodiscard]] u16 GXGetNumXfbLines(u16 efb_height, f32 y_scale);
[[nodiscard]] f32 GXGetYScaleFactor(u16 efb_height, u16 xfb_height);
[[nodiscard]] u32 GXSetDispCopyYScale(f32 vertical_scale);
void GXSetCopyClear(GXColor clear_color, u32 clear_z);
void GXSetCopyFilter(GXBool aa, const u8 sample_pattern[12][2], GXBool vertical_filter, const u8 vfilter[7]);
void GXSetDispCopyGamma(GXGamma gamma);
[[nodiscard]] u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 max_lod);
void GXInitTexObj(GXTexObj *obj, void *image_ptr, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrap_s, GXTexWrapMode wrap_t,
                  GXBool mipmap);
void GXInitTexObjLOD(GXTexObj *obj, GXTexFilter min_filter, GXTexFilter mag_filter, f32 min_lod, f32 max_lod, f32 lod_bias,
                     GXBool bias_clamp, GXBool do_edge_lod, GXAnisotropy max_aniso);
void GXLoadTexObj(GXTexObj *obj, GXTexMapID id);
void GXCopyDisp(void *dest, GXBool clear);
void GXCopyTex(void *dest, GXBool clear);
void GXPixModeSync();
void DCFlushRange(void *ptr, u32 size);
