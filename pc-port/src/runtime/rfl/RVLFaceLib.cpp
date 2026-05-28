#include <RVLFaceLib.h>

#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace {
    RFLCallback s_icon_draw_done_callback = nullptr;

    [[nodiscard]] smgpc::runtime::RuntimeContext *active_runtime() {
        return smgpc::runtime::RuntimeContext::try_instance();
    }

    [[nodiscard]] smgpc::runtime::RflService fallback_rfl_service() {
        return smgpc::runtime::RflService();
    }
}  // namespace

extern "C" u32 RFLGetWorkSize(BOOL deluxeTex) {
    if (auto *runtime = active_runtime()) {
        return static_cast<u32>(runtime->rfl().work_size(deluxeTex != FALSE));
    }

    auto rfl = fallback_rfl_service();
    return static_cast<u32>(rfl.work_size(deluxeTex != FALSE));
}

extern "C" RFLErrcode RFLInitResAsync(void *workBuffer, void *resBuffer, u32 resSize, BOOL deluxeTex) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().init_resources(workBuffer, resBuffer, resSize, deluxeTex != FALSE, true);
    }
    return workBuffer != nullptr && resBuffer != nullptr && resSize != 0U ? RFLErrcode_Success : RFLErrcode_WrongParam;
}

extern "C" RFLErrcode RFLInitRes(void *workBuffer, void *resBuffer, u32 resSize, BOOL deluxeTex) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().init_resources(workBuffer, resBuffer, resSize, deluxeTex != FALSE, false);
    }
    return workBuffer != nullptr && resBuffer != nullptr && resSize != 0U ? RFLErrcode_Success : RFLErrcode_WrongParam;
}

extern "C" void RFLExit(void) {
    if (auto *runtime = active_runtime()) {
        runtime->rfl().exit();
    }
}

extern "C" BOOL RFLAvailable(void) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().available() ? TRUE : FALSE;
    }
    return TRUE;
}

extern "C" RFLErrcode RFLGetAsyncStatus(void) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().async_status();
    }
    return RFLErrcode_Success;
}

extern "C" s32 RFLGetLastReason(void) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().last_reason();
    }
    return 0;
}

extern "C" RFLErrcode RFLWaitAsync(void) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().async_status();
    }
    return RFLErrcode_Success;
}

extern "C" RFLErrcode RFLGetAdditionalInfo(RFLAdditionalInfo *info, RFLDataSource source, RFLMiddleDB *db, u16 index) {
    if (info == nullptr) {
        return RFLErrcode_WrongParam;
    }

    std::memset(info, 0, sizeof(*info));
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().additional_info(*info, source, db, index);
    }
    return RFLErrcode_DBNodata;
}

extern "C" BOOL RFLSearchOfficialData(const RFLCreateID *id, u16 *index) {
    if (id == nullptr || index == nullptr) {
        return FALSE;
    }

    if (auto *runtime = active_runtime()) {
        return runtime->rfl().search_official_data(*id, *index) ? TRUE : FALSE;
    }
    return FALSE;
}

extern "C" BOOL RFLIsAvailableOfficialData(u16 index) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().is_available_official_data(index) ? TRUE : FALSE;
    }
    return FALSE;
}

extern "C" u32 RFLGetModelBufferSize(RFLResolution resolution, u32 expressionFlags) {
    if (auto *runtime = active_runtime()) {
        return static_cast<u32>(runtime->rfl().model_buffer_size(resolution, expressionFlags));
    }

    auto rfl = fallback_rfl_service();
    return static_cast<u32>(rfl.model_buffer_size(resolution, expressionFlags));
}

extern "C" RFLErrcode RFLInitCharModel(RFLCharModel *model, RFLDataSource source, RFLMiddleDB *db, u16 index, void *work,
                                       RFLResolution resolution, u32 expressionFlags) {
    if (model == nullptr) {
        return RFLErrcode_WrongParam;
    }
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().init_char_model(*model, source, db, index, work, resolution, expressionFlags);
    }
    return RFLErrcode_NotAvailable;
}

extern "C" void RFLSetMtx(RFLCharModel *model, const Mtx matrix) {
    if (model == nullptr || matrix == nullptr) {
        return;
    }
    std::memcpy(model->matrix, matrix, sizeof(Mtx));
}

extern "C" void RFLSetExpression(RFLCharModel *model, RFLExpression expression) {
    if (model == nullptr) {
        return;
    }
    if (auto *runtime = active_runtime()) {
        runtime->rfl().set_model_expression(*model, expression);
        return;
    }
    model->expression = expression;
}

extern "C" RFLExpression RFLGetExpression(const RFLCharModel *model) {
    return model != nullptr ? model->expression : RFLExp_Normal;
}

GXColor RFLGetFavoriteColor(RFLFavoriteColor color) {
    constexpr auto colors = std::array<GXColor, RFLFavoriteColor_Max>{
        GXColor {220U, 48U, 52U, 255U},
        GXColor {238U, 126U, 42U, 255U},
        GXColor {245U, 206U, 73U, 255U},
        GXColor {159U, 204U, 62U, 255U},
        GXColor {73U, 174U, 74U, 255U},
        GXColor {47U, 102U, 201U, 255U},
        GXColor {64U, 180U, 220U, 255U},
        GXColor {235U, 111U, 168U, 255U},
        GXColor {134U, 86U, 185U, 255U},
        GXColor {122U, 82U, 54U, 255U},
        GXColor {238U, 238U, 238U, 255U},
        GXColor {48U, 48U, 52U, 255U},
    };
    const auto index = std::min<std::size_t>(static_cast<std::size_t>(color), colors.size() - 1U);
    return colors[index];
}

extern "C" void RFLLoadDrawSetting(const RFLDrawSetting *) {
}

extern "C" void RFLDrawOpa(const RFLCharModel *model) {
    if (auto *runtime = active_runtime()) {
        runtime->rfl().draw_model(model);
    }
}

extern "C" void RFLDrawXlu(const RFLCharModel *model) {
    if (auto *runtime = active_runtime()) {
        runtime->rfl().draw_model(model);
    }
}

extern "C" void RFLLoadVertexSetting(const RFLDrawCoreSetting *) {
}

extern "C" void RFLLoadMaterialSetting(const RFLDrawCoreSetting *) {
}

extern "C" void RFLDrawOpaCore(const RFLCharModel *model, const RFLDrawCoreSetting *) {
    RFLDrawOpa(model);
}

extern "C" void RFLDrawXluCore(const RFLCharModel *model, const RFLDrawCoreSetting *) {
    RFLDrawXlu(model);
}

extern "C" void RFLDrawShape(const RFLCharModel *model) {
    RFLDrawOpa(model);
}

extern "C" RFLErrcode RFLMakeIcon(void *buffer, RFLDataSource source, RFLMiddleDB *db, u16 index, RFLExpression expression,
                                  const RFLIconSetting *setting) {
    if (setting == nullptr) {
        return RFLErrcode_WrongParam;
    }
    if (auto *runtime = active_runtime()) {
        const auto result = runtime->rfl().make_icon(buffer, source, db, index, expression, *setting);
        if (result == RFLErrcode_Success && s_icon_draw_done_callback != nullptr) {
            s_icon_draw_done_callback();
        }
        return result;
    }
    return RFLErrcode_NotAvailable;
}

extern "C" void RFLSetIconDrawDoneCallback(RFLCallback callback) {
    s_icon_draw_done_callback = callback;
}
