#pragma once

#include <revolution/types.h>
#include <cstdarg>

class MessageEditorMessageTag;

namespace ReplaceTagProcessor {
    u32 Replace(wchar_t*, const wchar_t*);
    u32 ReplaceArgs(wchar_t*, const wchar_t*, va_list);
    u32 exePictureGroup(wchar_t*, const MessageEditorMessageTag&);
    u32 exeStringGroup(wchar_t*, const MessageEditorMessageTag&, va_list);
    u32 exeNumberGroup(wchar_t*, const MessageEditorMessageTag&, va_list);
    u32 exeLocalizeGroup(wchar_t*, const MessageEditorMessageTag&);
    u32 exeLocalizeGroupPlayerName(wchar_t*, u8);
    u32 exeRaceTimeGroup(wchar_t*, const MessageEditorMessageTag&);

    struct Impl {
        struct GroupFunctionInfo {
            u8 mGroup;
            u32 (*mFunction)(wchar_t*, const MessageEditorMessageTag&);
        };
        static const GroupFunctionInfo sGroupFunctionTable[];
        static const GroupFunctionInfo* findGroupFunctionInfo(int);
    };

    struct ImplArgs {
        struct GroupFunctionInfo {
            u8 mGroup;
            u32 (*mFunction)(wchar_t*, const MessageEditorMessageTag&, va_list);
        };
        static const GroupFunctionInfo sGroupFunctionTable[];
        static const GroupFunctionInfo* findGroupFunctionInfo(int);
    };
};  // namespace ReplaceTagProcessor

namespace ReplaceTagFunction {
    u32 ReplaceArgs(wchar_t*, s32, const wchar_t*, ...);
};  // namespace ReplaceTagFunction
