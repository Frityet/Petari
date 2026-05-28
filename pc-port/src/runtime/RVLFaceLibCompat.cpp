#include <RVLFaceLib.h>

#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <cstring>
#include <span>
#include <string_view>

namespace {
    [[nodiscard]] smgpc::compat::RuntimeContext *active_runtime() {
        return smgpc::compat::RuntimeContext::try_instance();
    }

    [[nodiscard]] std::span<const smgpc::compat::RflMiiEntry> valid_miis() {
        if (auto *runtime = active_runtime()) {
            return runtime->rfl().valid_miis();
        }

        return {};
    }

    [[nodiscard]] const smgpc::compat::RflMiiEntry *find_mii_by_index(u16 index) {
        const auto entries = valid_miis();
        const auto found = std::ranges::find_if(entries, [index](const auto &entry) { return entry.index == static_cast<s32>(index); });
        return found != entries.end() ? &*found : nullptr;
    }

    [[nodiscard]] s32 create_id_index(const RFLCreateID &id) {
        auto value = s32{};
        static_assert(sizeof(value) <= RFL_CREATEID_LEN);
        std::memcpy(&value, id.data, sizeof(value));
        return value;
    }

    void write_create_id(RFLCreateID &id, s32 index) {
        std::memset(id.data, 0, sizeof(id.data));
        std::memcpy(id.data, &index, std::min(sizeof(index), sizeof(id.data)));
    }

    void copy_name(u16 *pDst, std::string_view name) {
        if (pDst == nullptr) {
            return;
        }

        std::fill_n(pDst, RFL_NAME_LEN + 1U, u16{});
        const auto utf16 = smgpc::compat::utf16_from_utf8_lossy(name);
        const auto count = std::min<std::size_t>(utf16.size(), RFL_NAME_LEN);
        for (auto i = std::size_t{}; i < count; ++i) {
            pDst[i] = static_cast<u16>(utf16[i]);
        }
    }
}  // namespace

extern "C" RFLErrcode RFLGetAdditionalInfo(RFLAdditionalInfo *info, RFLDataSource source, RFLMiddleDB *, u16 index) {
    if (info == nullptr) {
        return RFLErrcode_WrongParam;
    }

    std::memset(info, 0, sizeof(*info));

    if (source != RFLDataSource_Official && source != RFLDataSource_Default && source != RFLDataSource_Middle) {
        return RFLErrcode_DBNodata;
    }

    if (auto *runtime = active_runtime()) {
        if (!runtime->rfl().is_initialized()) {
            return RFLErrcode_NotAvailable;
        }
        if (runtime->rfl().has_error()) {
            return RFLErrcode_Broken;
        }
    }

    const auto *entry = find_mii_by_index(index);
    if (entry == nullptr) {
        return RFLErrcode_DBNodata;
    }

    copy_name(info->name, entry->name);
    write_create_id(info->createID, entry->index);
    info->color = 0U;
    info->favorite = 1U;
    info->height = 64U;
    info->build = 64U;
    info->skinColor = GXColor{255U, 224U, 189U, 255U};
    return RFLErrcode_Success;
}

extern "C" BOOL RFLSearchOfficialData(const RFLCreateID *id, u16 *index) {
    if (id == nullptr || index == nullptr) {
        return FALSE;
    }

    const auto decoded_index = create_id_index(*id);
    if (decoded_index < 0 || decoded_index > 0xffff) {
        return FALSE;
    }

    if (find_mii_by_index(static_cast<u16>(decoded_index)) == nullptr) {
        return FALSE;
    }

    *index = static_cast<u16>(decoded_index);
    return TRUE;
}

extern "C" BOOL RFLIsAvailableOfficialData(u16 index) {
    return find_mii_by_index(index) != nullptr ? TRUE : FALSE;
}
