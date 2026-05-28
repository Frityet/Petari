#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <RVLFaceLib.h>

namespace smgpc::runtime {

    class NandFileSystemService;

    struct RflMiiEntry {
        s32 index = 0;
        RFLDataSource source = RFLDataSource_Official;
        bool available = true;
        std::string name;
        std::string creator;
        RFLCreateID create_id{};
        u32 sex = 0U;
        u32 bmonth = 0U;
        u32 bday = 0U;
        u32 color = 0U;
        bool favorite = true;
        u32 height = 64U;
        u32 build = 64U;
        GXColor skin_color{255U, 224U, 189U, 255U};
    };

    struct RflDbStatus {
        bool nand_bound = false;
        bool db_present = false;
        bool fallback_used = false;
        bool async_pending = false;
        bool resource_initialized = true;
        bool deluxe_textures = false;
        std::size_t byte_count = 0U;
        std::size_t entry_count = 0U;
        std::uint64_t loaded_frame = 0U;
        RFLErrcode last_error = RFLErrcode_DBNodata;
        s32 last_reason = 0;
    };

    enum class RflOperationKind {
        LoadBegin,
        LoadComplete,
        LoadFailed,
        Persist,
        AdditionalInfo,
        SearchOfficial,
        CheckAvailable,
        InitResource,
        InitCharModel,
        SetExpression,
        MakeIcon,
        DrawModel,
        MiiSelectPage,
    };

    struct RflOperationTrace {
        RflOperationKind kind = RflOperationKind::LoadComplete;
        std::uint64_t frame_index = 0U;
        std::string path;
        RFLDataSource source = RFLDataSource_Official;
        s32 index = -1;
        RFLErrcode result = RFLErrcode_Success;
        std::size_t byte_count = 0U;
        std::size_t entry_count = 0U;
        bool db_present = false;
        bool fallback_used = false;
        bool async_pending = false;
        bool texture_available = false;
        u16 width = 0U;
        u16 height = 0U;
        RFLExpression expression = RFLExp_Normal;
        u32 expression_flags = RFLExpFlag_Normal;
        std::size_t page_index = 0U;
        std::size_t page_count = 0U;
        std::size_t icon_count = 0U;
        bool selected = false;
        bool prohibited = false;
    };

    struct RflIconTexture {
        RFLErrcode result = RFLErrcode_Unknown;
        RFLDataSource source = RFLDataSource_Official;
        u16 index = 0U;
        u16 width = 0U;
        u16 height = 0U;
        RFLExpression expression = RFLExp_Normal;
        GXColor background{};
        bool texture_available = false;
        std::vector<std::uint8_t> rgb5a3;
    };

    enum class RflMiiSelectIconKind {
        Special,
        Mii,
    };

    struct RflMiiSelectIconKey {
        RflMiiSelectIconKind kind = RflMiiSelectIconKind::Special;
        s32 id = 0;
        RFLDataSource source = RFLDataSource_Official;
    };

    struct RflMiiSelectSpecialIcon {
        s32 id = 0;
        std::string name;
        bool valid = true;
        bool texture_available = true;
    };

    struct RflMiiSelectIcon {
        RflMiiSelectIconKey key{};
        std::string name;
        bool favorite = false;
        bool selectable = true;
        bool prohibited = false;
        bool selected = false;
        bool texture_available = false;
        std::size_t page_index = 0U;
        std::size_t slot_index = 0U;
        bool has_create_id = false;
        RFLCreateID create_id{};
    };

    struct RflMiiSelectPageState {
        std::size_t icons_per_page = 8U;
        std::size_t page_index = 0U;
        std::size_t page_count = 0U;
        std::size_t icon_count = 0U;
        bool has_selected_icon = false;
        bool has_selected_create_id = false;
        RFLCreateID selected_create_id{};
        std::vector<RflMiiSelectIcon> icons;
    };

    class RflService final {
    public:
        RflService();
        explicit RflService(NandFileSystemService &nand);

        void begin_frame(std::uint64_t frame_index);
        void set_nand(NandFileSystemService &nand);
        void set_initialized(bool initialized);
        void set_error(bool error);
        [[nodiscard]] std::size_t work_size(bool deluxe_textures) const;
        [[nodiscard]] RFLErrcode init_resources(void *work_buffer, const void *resource_buffer, std::size_t resource_size, bool deluxe_textures,
                                                bool async);
        void exit();
        [[nodiscard]] bool available() const;
        [[nodiscard]] RFLErrcode async_status() const;
        [[nodiscard]] s32 last_reason() const;
        void request_async_load(std::uint64_t delay_frames = 1U);
        void clear_for_reload();
        void set_miis(std::vector<RflMiiEntry> miis);
        void add_or_replace_mii(RflMiiEntry mii);
        void persist_to_nand();

        [[nodiscard]] std::vector<std::uint8_t> serialize_miis() const;
        [[nodiscard]] bool load_from_bytes(std::span<const std::uint8_t> bytes);

        [[nodiscard]] bool is_initialized() const;
        [[nodiscard]] bool has_error() const;
        [[nodiscard]] const RflDbStatus &db_status() const;
        [[nodiscard]] std::span<const RflMiiEntry> valid_miis() const;
        [[nodiscard]] std::span<const RflOperationTrace> trace() const;
        [[nodiscard]] RFLErrcode additional_info(RFLAdditionalInfo &info, RFLDataSource source, const RFLMiddleDB *db, u16 index) const;
        [[nodiscard]] bool search_official_data(const RFLCreateID &id, u16 &index) const;
        [[nodiscard]] bool is_available_official_data(u16 index) const;
        [[nodiscard]] std::size_t model_buffer_size(RFLResolution resolution, u32 expression_flags) const;
        [[nodiscard]] RFLErrcode init_char_model(RFLCharModel &model, RFLDataSource source, const RFLMiddleDB *db, u16 index, void *work,
                                                 RFLResolution resolution, u32 expression_flags) const;
        void set_model_expression(RFLCharModel &model, RFLExpression expression) const;
        void draw_model(const RFLCharModel *model) const;
        [[nodiscard]] RflIconTexture make_icon_texture(RFLDataSource source, const RFLMiddleDB *db, u16 index, RFLExpression expression,
                                                       const RFLIconSetting &setting) const;
        [[nodiscard]] RFLErrcode make_icon(void *buffer, RFLDataSource source, const RFLMiddleDB *db, u16 index, RFLExpression expression,
                                           const RFLIconSetting &setting) const;
        [[nodiscard]] RflMiiSelectPageState mii_select_page_state(std::span<const RflMiiSelectSpecialIcon> special_icons,
                                                                  std::optional<RflMiiSelectIconKey> prohibited,
                                                                  std::size_t page_index,
                                                                  std::optional<RflMiiSelectIconKey> selected) const;
        void clear_trace();

    private:
        void ensure_loaded() const;
        void rebuild_valid_miis() const;
        void replace_cache(std::vector<RflMiiEntry> miis, RflDbStatus status, bool manual_override) const;
        void push_trace(RflOperationTrace trace) const;
        [[nodiscard]] const RflMiiEntry *find_entry(RFLDataSource source, u16 index) const;
        [[nodiscard]] static std::vector<RflMiiEntry> fallback_miis();

        NandFileSystemService *_nand = nullptr;
        bool _initialized = true;
        bool _forced_error = false;
        bool _resource_initialized = true;
        bool _deluxe_textures = false;
        s32 _last_reason = 0;
        bool _async_pending = false;
        std::uint64_t _frame_index = 0U;
        std::uint64_t _async_complete_frame = 0U;
        mutable bool _cache_loaded = false;
        mutable bool _manual_override = false;
        mutable RflDbStatus _status;
        mutable std::vector<RflMiiEntry> _miis;
        mutable std::vector<RflMiiEntry> _valid_miis;
        mutable std::vector<RflOperationTrace> _trace;
    };

}  // namespace smgpc::runtime
