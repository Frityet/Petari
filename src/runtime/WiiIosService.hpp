#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

namespace smgpc::runtime {

    constexpr s32 IOS_RESULT_OK = 0;
    constexpr s32 IOS_RESULT_INVALID = -4;
    constexpr s32 IOS_RESULT_NO_ENTRY = -6;

    constexpr s32 DI_RESULT_SUCCESS = 1;
    constexpr s32 DI_RESULT_DRIVE_ERROR = 2;
    constexpr s32 DI_RESULT_COVER_CLOSED = 4;
    constexpr s32 DI_RESULT_READ_TIMED_OUT = 16;
    constexpr s32 DI_RESULT_SECURITY_ERROR = 32;
    constexpr s32 DI_RESULT_VERIFY_ERROR = 64;
    constexpr s32 DI_RESULT_BAD_ARGUMENT = 128;

    constexpr u32 DI_IOCTL_DVD_LOW_READ_DISK_ID = 0x70U;
    constexpr u32 DI_IOCTL_DVD_LOW_READ = 0x71U;
    constexpr u32 DI_IOCTL_DVD_LOW_WAIT_FOR_COVER_CLOSE = 0x79U;
    constexpr u32 DI_IOCTL_DVD_LOW_GET_COVER_REGISTER = 0x7AU;
    constexpr u32 DI_IOCTL_DVD_LOW_GET_LENGTH = 0x83U;
    constexpr u32 DI_IOCTL_DVD_LOW_GET_COVER_STATUS = 0x88U;
    constexpr u32 DI_IOCTL_DVD_LOW_OPEN_PARTITION = 0x8BU;
    constexpr u32 DI_IOCTL_DVD_LOW_CLOSE_PARTITION = 0x8CU;
    constexpr u32 DI_IOCTL_DVD_LOW_GET_STATUS_REGISTER = 0x95U;
    constexpr u32 DI_IOCTL_DVD_LOW_REQUEST_ERROR = 0xE0U;

    constexpr u32 DI_COVER_STATUS_NO_DISC = 1U;
    constexpr u32 DI_COVER_STATUS_DISC_INSERTED = 2U;

    enum class WiiIosDeviceKind {
        FileSystem,
        ES,
        DI,
        STM,
        USB_OH1,
    };

    enum class WiiIosRequestKind {
        Open,
        Close,
        Read,
        Write,
        Ioctl,
        Ioctlv,
    };

    struct WiiIosDevice {
        std::string path;
        WiiIosDeviceKind kind = WiiIosDeviceKind::FileSystem;
    };

    struct WiiIosRequest {
        std::uint64_t id = 0U;
        WiiIosRequestKind kind = WiiIosRequestKind::Open;
        std::string device_path;
        s32 file_handle = -1;
        u32 command = 0U;
        std::vector<std::uint8_t> input;
        std::vector<std::uint8_t> output;
        std::size_t output_size = 0U;
        std::uint64_t submitted_frame = 0U;
        std::uint64_t completion_frame = 0U;
        bool completed = false;
        s32 result = IOS_RESULT_INVALID;
    };

    struct WiiDiState {
        bool disc_inserted = true;
        bool partition_open = false;
        std::uint64_t partition_offset = 0U;
        u32 status_register = 0U;
        u32 cover_register = DI_COVER_STATUS_DISC_INSERTED;
        u32 last_length = 0U;
        u32 last_error = 0U;
    };

    class WiiIosService final {
    public:
        WiiIosService();

        void begin_frame(std::uint64_t frame_index);
        [[nodiscard]] bool has_device(std::string_view path) const;
        [[nodiscard]] std::optional<WiiIosDevice> device(std::string_view path) const;
        [[nodiscard]] std::span<const WiiIosDevice> devices() const;
        [[nodiscard]] s32 open_device(std::string_view path);
        [[nodiscard]] s32 close_device(s32 file_handle);
        [[nodiscard]] std::optional<std::string_view> device_path(s32 file_handle) const;
        [[nodiscard]] std::uint64_t submit_open(std::string_view path, std::uint64_t delay_frames = 1U);
        [[nodiscard]] std::uint64_t submit_close(s32 file_handle, std::uint64_t delay_frames = 1U);
        [[nodiscard]] std::uint64_t submit_ioctl(s32 file_handle, u32 command, std::span<const std::uint8_t> input,
                                                 std::size_t output_size, std::uint64_t delay_frames = 1U);
        [[nodiscard]] std::uint64_t submit_ioctlv(s32 file_handle, u32 command, std::span<const std::uint8_t> input,
                                                  std::size_t output_size, std::uint64_t delay_frames = 1U);
        [[nodiscard]] std::optional<WiiIosRequest> request(std::uint64_t id) const;
        [[nodiscard]] std::optional<s32> request_result(std::uint64_t id) const;
        [[nodiscard]] const WiiDiState &di_state() const;
        void set_disc_inserted(bool inserted);
        [[nodiscard]] std::span<const WiiIosRequest> requests() const;
        [[nodiscard]] std::span<const WiiIosRequest> completed_requests() const;
        void clear_trace();

    private:
        [[nodiscard]] s32 result_for_request(WiiIosRequest &request);
        [[nodiscard]] s32 result_for_di_request(WiiIosRequest &request);
        [[nodiscard]] s32 result_for_di_ioctl(WiiIosRequest &request);
        [[nodiscard]] s32 result_for_di_ioctlv(WiiIosRequest &request);
        [[nodiscard]] std::string path_for_handle(s32 file_handle) const;
        [[nodiscard]] std::uint64_t submit_request(WiiIosRequest request, std::uint64_t delay_frames);
        void write_output_u32(WiiIosRequest &request, u32 value) const;
        void complete_ready_requests();

        std::uint64_t _frame_index = 0U;
        std::uint64_t _next_request_id = 1U;
        s32 _next_file_handle = 1;
        std::vector<WiiIosDevice> _devices;
        std::map<s32, std::string> _open_files;
        std::vector<WiiIosRequest> _requests;
        std::vector<WiiIosRequest> _completed_requests;
        WiiDiState _di;
    };

}  // namespace smgpc::runtime
