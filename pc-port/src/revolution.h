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

constexpr s32 WPAD_ERR_NONE = 0;
constexpr s32 WPAD_ERR_NO_CONTROLLER = -1;

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
