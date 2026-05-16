#include "Game/Map/FileSelectFunc.hpp"

#include "Game/Map/FileSelectIconID.hpp"
#include "compat/FileSelectFuncCompat.hpp"

namespace {

    constexpr u32 RFL_NAME_LEN = 10;

    static const char* sIconNameMessageID[] = {
        "System_FileSelect_Icon000", "System_FileSelect_Icon001", "System_FileSelect_Icon002",
        "System_FileSelect_Icon003", "System_FileSelect_Icon004",
    };

}  // namespace

namespace FileSelectFunc {

    u32 getMiiNameBufferSize() {
        return RFL_NAME_LEN + 1;
    }

    void copyMiiName(u16* pName, const FileSelectIconID& rIcon) {
        if (rIcon.isFellow()) {
            const char* pMessageId = sIconNameMessageID[rIcon.getFellowID()];

            smgpc::game::compat::copy_file_select_fellow_name(pName, pMessageId, getMiiNameBufferSize());
        } else if (rIcon.isMii()) {
            smgpc::game::compat::copy_file_select_mii_name(pName, rIcon.getMiiIndex(), getMiiNameBufferSize());
        }
    }

}  // namespace FileSelectFunc
