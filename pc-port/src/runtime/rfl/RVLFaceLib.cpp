#include <RVLFaceLib.h>

#include "runtime/RuntimeContext.hpp"

#include <array>
#include <cstring>

namespace {
    RFLCallback s_icon_draw_done_callback = nullptr;

    [[nodiscard]] smgpc::runtime::RuntimeContext *active_runtime() {
        return smgpc::runtime::RuntimeContext::try_instance();
    }
}  // namespace

extern "C" u32 RFLGetWorkSize(BOOL deluxeTex) {
    if (auto *runtime = active_runtime()) {
        return static_cast<u32>(runtime->rfl().work_size(deluxeTex != FALSE));
    }

    return static_cast<u32>(smgpc::runtime::RflService::work_size(deluxeTex != FALSE));
}

extern "C" RFLErrcode RFLInitResAsync(void *workBuffer, void *resBuffer, u32 resSize, BOOL deluxeTex) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().init_resources(workBuffer, resBuffer, resSize, deluxeTex != FALSE, true);
    }
    return RFLErrcode_NotAvailable;
}

extern "C" RFLErrcode RFLInitRes(void *workBuffer, void *resBuffer, u32 resSize, BOOL deluxeTex) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().init_resources(workBuffer, resBuffer, resSize, deluxeTex != FALSE, false);
    }
    return RFLErrcode_NotAvailable;
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
    return FALSE;
}

extern "C" RFLErrcode RFLGetAsyncStatus(void) {
    if (auto *runtime = active_runtime()) {
        return runtime->rfl().async_status();
    }
    return RFLErrcode_NotAvailable;
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
    return RFLErrcode_NotAvailable;
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

    return static_cast<u32>(smgpc::runtime::RflService::model_buffer_size(resolution, expressionFlags));
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
    (void)model;
    (void)matrix;
}

extern "C" void RFLSetExpression(RFLCharModel *model, RFLExpression expression) {
    if (model == nullptr) {
        return;
    }
    if (auto *runtime = active_runtime()) {
        runtime->rfl().set_model_expression(*model, expression);
    }
}

extern "C" RFLExpression RFLGetExpression(const RFLCharModel *model) {
    return model != nullptr ? model->expression : RFLExp_Normal;
}

GXColor RFLGetFavoriteColor(RFLFavoriteColor color) {
    constexpr auto colors = std::array<GXColor, RFLFavoriteColor_Max>{
        GXColor {184U, 64U, 48U, 255U},
        GXColor {240U, 120U, 40U, 255U},
        GXColor {248U, 216U, 32U, 255U},
        GXColor {128U, 200U, 40U, 255U},
        GXColor {0U, 116U, 40U, 255U},
        GXColor {32U, 72U, 152U, 255U},
        GXColor {64U, 160U, 216U, 255U},
        GXColor {232U, 96U, 120U, 255U},
        GXColor {112U, 44U, 168U, 255U},
        GXColor {72U, 56U, 24U, 255U},
        GXColor {224U, 224U, 224U, 255U},
        GXColor {24U, 24U, 20U, 255U},
    };
    return colors[static_cast<std::size_t>(color)];
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
