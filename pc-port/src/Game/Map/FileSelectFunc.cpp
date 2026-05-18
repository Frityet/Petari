#include "Game/Map/FileSelectFunc.hpp"

#include "Game/Map/FileSelectIconID.hpp"
#include "Game/Util/MessageUtil.hpp"

#include <RVLFaceLib.h>

#include <algorithm>

namespace {
    constexpr const char* sIconNameMessageID[] = {
        "System_FileSelect_Icon000",
        "System_FileSelect_Icon001",
        "System_FileSelect_Icon002",
        "System_FileSelect_Icon003",
        "System_FileSelect_Icon004",
    };

    void clear_name(u16* pName) {
        if (pName != nullptr) {
            std::fill_n(pName, FileSelectFunc::getMiiNameBufferSize(), u16{});
        }
    }

    void copy_wide_name(u16* pName, const wchar_t* pSource) {
        clear_name(pName);
        if (pName == nullptr || pSource == nullptr) {
            return;
        }

        for (auto i = std::size_t{}; i + 1U < FileSelectFunc::getMiiNameBufferSize() && pSource[i] != L'\0'; ++i) {
            pName[i] = static_cast<u16>(pSource[i]);
        }
    }
}  // namespace

namespace FileSelectFunc {
    u32 getMiiNameBufferSize() {
        return RFL_NAME_LEN + 1U;
    }

    void copyMiiName(u16* pName, const FileSelectIconID& rIcon) {
        clear_name(pName);
        if (pName == nullptr) {
            return;
        }

        if (rIcon.isFellow()) {
            copy_wide_name(pName, MR::getGameMessageDirect(sIconNameMessageID[rIcon.getFellowID()]));
            return;
        }

        if (rIcon.isMii()) {
            auto info = RFLAdditionalInfo{};
            const auto err = RFLGetAdditionalInfo(&info, RFLDataSource_Official, nullptr, rIcon.getMiiIndex());
            if (err == RFLErrcode_Success) {
                std::copy_n(info.name, getMiiNameBufferSize(), pName);
            }
        }
    }
}  // namespace FileSelectFunc
