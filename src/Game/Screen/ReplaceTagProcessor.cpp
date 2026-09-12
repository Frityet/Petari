#include "Game/Screen/ReplaceTagProcessor.hpp"
#include "Game/Screen/MessageEditorMessageTag.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <cstdio>

extern "C" int swprintf(wchar_t*, size_t, const wchar_t*, ...);

namespace ReplaceTagProcessor {
    const Impl::GroupFunctionInfo Impl::sGroupFunctionTable[] = {
        {5, exeLocalizeGroup}, {9, exeRaceTimeGroup}, {3, exePictureGroup}, {0, nullptr},
    };

    const ImplArgs::GroupFunctionInfo ImplArgs::sGroupFunctionTable[] = {
        {7, exeStringGroup}, {6, exeNumberGroup}, {0, nullptr},
    };

    const Impl::GroupFunctionInfo* Impl::findGroupFunctionInfo(int group) {
        for (const GroupFunctionInfo* info = sGroupFunctionTable; info->mFunction != nullptr; ++info) {
            if (info->mGroup == group) {
                return info;
            }
        }
        return nullptr;
    }

    const ImplArgs::GroupFunctionInfo* ImplArgs::findGroupFunctionInfo(int group) {
        for (const GroupFunctionInfo* info = sGroupFunctionTable; info->mFunction != nullptr; ++info) {
            if (info->mGroup == group) {
                return info;
            }
        }
        return nullptr;
    }

    u32 Replace(wchar_t* output, const wchar_t* input) {
        wchar_t* start = output;
        while (*input != L'\0') {
            if (MR::isMessageEditorNextTag(input)) {
                *output = L'\0';
                return output - start;
            }

            if (*input == 0x1A) {
                MessageEditorMessageTag tag(input + 1);
                const Impl::GroupFunctionInfo* info = Impl::findGroupFunctionInfo(static_cast< u8 >(tag.mMessage[0]));
                if (info != nullptr) {
                    output += info->mFunction(output, tag);
                } else {
                    MR::copyMemory(output, input, (tag.getSkipLength() + 1) * sizeof(wchar_t));
                    output += tag.getSkipLength() + 1;
                }
                input += tag.getSkipLength() + 1;
            } else {
                *output++ = *input++;
            }
        }
        *output = L'\0';
        return output - start;
    }

    u32 ReplaceArgs(wchar_t* output, const wchar_t* input, va_list args) {
        wchar_t* start = output;
        while (*input != L'\0') {
            if (MR::isMessageEditorNextTag(input)) {
                *output = L'\0';
                return output - start;
            }

            if (*input == 0x1A) {
                MessageEditorMessageTag tag(input + 1);
                const ImplArgs::GroupFunctionInfo* info = ImplArgs::findGroupFunctionInfo(static_cast< u8 >(tag.mMessage[0]));
                if (info != nullptr) {
                    output += info->mFunction(output, tag, args);
                } else {
                    MR::copyMemory(output, input, (tag.getSkipLength() + 1) * sizeof(wchar_t));
                    output += tag.getSkipLength() + 1;
                }
                input += tag.getSkipLength() + 1;
            } else {
                *output++ = *input++;
            }
        }
        *output = L'\0';
        return output - start;
    }

    u32 exePictureGroup(wchar_t* output, const MessageEditorMessageTag& tag) {
        wchar_t header[2];
        header[0] = ((tag.getTagLength() + 2) << 8) | static_cast< u8 >(tag.mMessage[0]);
        header[1] = tag.mMessage[1];
        if (static_cast< u16 >(tag.mMessage[1] + L'0') == L'[') {
            header[1] = MR::isPlayerLuigi() ? 0x1C : 0x12;
        }
        *output++ = 0x1A;
        MR::copyMemory(output, header, tag.getTagLength() / 2 * sizeof(wchar_t));
        return tag.getTagLength() / 2 + 1;
    }

    u32 exeStringGroup(wchar_t* output, const MessageEditorMessageTag& tag, va_list args) {
        const wchar_t* string = nullptr;
        va_list copy;
        va_copy(copy, args);
        for (u32 i = 0; i <= tag.getParam32(1); ++i) {
            string = va_arg(copy, const wchar_t*);
        }
        va_end(copy);
        return swprintf(output, 256, L"%ls", string);
    }

    u32 exeNumberGroup(wchar_t* output, const MessageEditorMessageTag& tag, va_list args) {
        s32 number = 0;
        va_list copy;
        va_copy(copy, args);
        for (u32 i = 0; i <= tag.getParam32(1); ++i) {
            number = va_arg(copy, s32);
        }
        va_end(copy);
        switch (tag.mMessage[1]) {
        case 5:
            return swprintf(output, 256, L"%02d", number);
        case 6:
            return swprintf(output, 256, L"%03d", number);
        case 7:
            return swprintf(output, 256, L"%04d", number);
        case 8:
            return swprintf(output, 256, L"%05d", number);
        case 9:
            return swprintf(output, 256, L"%06d", number);
        default:
            return swprintf(output, 256, L"%d", number);
        }
    }

    u32 exeLocalizeGroup(wchar_t* output, const MessageEditorMessageTag& tag) {
        switch (tag.mMessage[1]) {
        case 0:
            return exeLocalizeGroupPlayerName(output, tag.getParam8(0));
        default:
            return 0;
        }
    }

    u32 exeLocalizeGroupPlayerName(wchar_t* output, u8 form) {
        char messageId[256];
        if (MR::isPlayerLuigi()) {
            snprintf(messageId, sizeof(messageId), "System_PlayerName1%02d", form);
        } else {
            snprintf(messageId, sizeof(messageId), "System_PlayerName0%02d", form);
        }
        const wchar_t* message = MR::getGameMessageDirect(messageId);
        s32 length = MR::getStringLengthWithMessageTag(message);
        MR::copyString(output, message, length + 1);
        return length;
    }

    u32 exeRaceTimeGroup(wchar_t* output, const MessageEditorMessageTag& tag) {
        if (tag.mMessage[1] == 5) {
            MR::makeRaceCurrentTimeString(output);
        } else {
            MR::makeRaceBestTimeString(output, tag.mMessage[1]);
        }
        return 8;
    }
};  // namespace ReplaceTagProcessor

namespace ReplaceTagFunction {
    u32 ReplaceArgs(wchar_t* output, s32 size, const wchar_t* input, ...) {
        va_list args;
        va_start(args, input);
        wchar_t buffer[512];
        s32 length = ReplaceTagProcessor::ReplaceArgs(buffer, input, args);
        if (length >= size) {
            buffer[size - 1] = L'\0';
            size = length;
        }
        MR::copyString(output, buffer, size);
        va_end(args);
        return size;
    }
};  // namespace ReplaceTagFunction
