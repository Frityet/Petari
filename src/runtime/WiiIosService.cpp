#include "runtime/WiiIosService.hpp"

#include <algorithm>
#include <utility>

namespace smgpc::runtime {
    namespace {
        [[nodiscard]] u32 read_be_u32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + sizeof(u32) > bytes.size()) {
                return 0U;
            }

            return (static_cast<u32>(bytes[offset]) << 24U) | (static_cast<u32>(bytes[offset + 1U]) << 16U) |
                   (static_cast<u32>(bytes[offset + 2U]) << 8U) | bytes[offset + 3U];
        }

        [[nodiscard]] std::vector<WiiIosDevice> default_devices() {
            return {
                {.path = "/dev/fs", .kind = WiiIosDeviceKind::FileSystem},
                {.path = "/dev/es", .kind = WiiIosDeviceKind::ES},
                {.path = "/dev/di", .kind = WiiIosDeviceKind::DI},
                {.path = "/dev/stm", .kind = WiiIosDeviceKind::STM},
                {.path = "/dev/usb/oh1", .kind = WiiIosDeviceKind::USB_OH1},
            };
        }
    }  // namespace

    WiiIosService::WiiIosService() : _devices(default_devices()) {
    }

    void WiiIosService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        complete_ready_requests();
    }

    bool WiiIosService::has_device(std::string_view path) const {
        return device(path).has_value();
    }

    std::optional<WiiIosDevice> WiiIosService::device(std::string_view path) const {
        const auto it = std::ranges::find_if(_devices, [path](const auto &entry) { return entry.path == path; });
        if (it == _devices.end()) {
            return std::nullopt;
        }
        return *it;
    }

    std::span<const WiiIosDevice> WiiIosService::devices() const {
        return _devices;
    }

    s32 WiiIosService::open_device(std::string_view path) {
        if (!has_device(path)) {
            return IOS_RESULT_NO_ENTRY;
        }

        const auto file_handle = _next_file_handle++;
        _open_files[file_handle] = std::string(path);
        return file_handle;
    }

    s32 WiiIosService::close_device(s32 file_handle) {
        if (_open_files.erase(file_handle) == 0U) {
            return IOS_RESULT_INVALID;
        }
        return IOS_RESULT_OK;
    }

    std::optional<std::string_view> WiiIosService::device_path(s32 file_handle) const {
        const auto it = _open_files.find(file_handle);
        if (it == _open_files.end()) {
            return std::nullopt;
        }
        return std::string_view(it->second);
    }

    std::uint64_t WiiIosService::submit_open(std::string_view path, std::uint64_t delay_frames) {
        return submit_request(WiiIosRequest {
                                  .kind = WiiIosRequestKind::Open,
                                  .device_path = std::string(path),
                                  .file_handle = -1,
                                  .command = 0U,
                                  .input = {},
                                  .output = {},
                                  .output_size = 0U,
                              },
                              delay_frames);
    }

    std::uint64_t WiiIosService::submit_close(s32 file_handle, std::uint64_t delay_frames) {
        return submit_request(WiiIosRequest {
                                  .kind = WiiIosRequestKind::Close,
                                  .device_path = path_for_handle(file_handle),
                                  .file_handle = file_handle,
                                  .command = 0U,
                                  .input = {},
                                  .output = {},
                                  .output_size = 0U,
                              },
                              delay_frames);
    }

    std::uint64_t WiiIosService::submit_ioctl(s32 file_handle, u32 command, std::span<const std::uint8_t> input,
                                              std::size_t output_size, std::uint64_t delay_frames) {
        return submit_request(WiiIosRequest {
                                  .kind = WiiIosRequestKind::Ioctl,
                                  .device_path = path_for_handle(file_handle),
                                  .file_handle = file_handle,
                                  .command = command,
                                  .input = std::vector<std::uint8_t>(input.begin(), input.end()),
                                  .output = {},
                                  .output_size = output_size,
                              },
                              delay_frames);
    }

    std::uint64_t WiiIosService::submit_ioctlv(s32 file_handle, u32 command, std::span<const std::uint8_t> input,
                                               std::size_t output_size, std::uint64_t delay_frames) {
        return submit_request(WiiIosRequest {
                                  .kind = WiiIosRequestKind::Ioctlv,
                                  .device_path = path_for_handle(file_handle),
                                  .file_handle = file_handle,
                                  .command = command,
                                  .input = std::vector<std::uint8_t>(input.begin(), input.end()),
                                  .output = {},
                                  .output_size = output_size,
                              },
                              delay_frames);
    }

    std::optional<WiiIosRequest> WiiIosService::request(std::uint64_t id) const {
        const auto it = std::ranges::find_if(_requests, [id](const auto &entry) { return entry.id == id; });
        if (it == _requests.end()) {
            return std::nullopt;
        }
        return *it;
    }

    std::optional<s32> WiiIosService::request_result(std::uint64_t id) const {
        const auto found = request(id);
        if (!found.has_value() || !found->completed) {
            return std::nullopt;
        }
        return found->result;
    }

    const WiiDiState &WiiIosService::di_state() const {
        return _di;
    }

    void WiiIosService::set_disc_inserted(bool inserted) {
        _di.disc_inserted = inserted;
        _di.cover_register = inserted ? DI_COVER_STATUS_DISC_INSERTED : DI_COVER_STATUS_NO_DISC;
    }

    std::span<const WiiIosRequest> WiiIosService::requests() const {
        return _requests;
    }

    std::span<const WiiIosRequest> WiiIosService::completed_requests() const {
        return _completed_requests;
    }

    void WiiIosService::clear_trace() {
        _requests.clear();
        _completed_requests.clear();
    }

    s32 WiiIosService::result_for_request(WiiIosRequest &request) {
        switch (request.kind) {
        case WiiIosRequestKind::Open:
            return open_device(request.device_path);
        case WiiIosRequestKind::Close:
            return close_device(request.file_handle);
        case WiiIosRequestKind::Read:
        case WiiIosRequestKind::Write:
            return has_device(request.device_path) && request.file_handle > 0 ? IOS_RESULT_OK : IOS_RESULT_INVALID;
        case WiiIosRequestKind::Ioctl:
        case WiiIosRequestKind::Ioctlv:
            return result_for_di_request(request);
        }

        return IOS_RESULT_INVALID;
    }

    s32 WiiIosService::result_for_di_request(WiiIosRequest &request) {
        if (!has_device(request.device_path) || request.file_handle <= 0) {
            return IOS_RESULT_INVALID;
        }
        if (request.device_path != "/dev/di") {
            request.output.assign(request.output_size, 0U);
            return IOS_RESULT_OK;
        }
        if (request.kind == WiiIosRequestKind::Ioctlv) {
            return result_for_di_ioctlv(request);
        }
        return result_for_di_ioctl(request);
    }

    s32 WiiIosService::result_for_di_ioctl(WiiIosRequest &request) {
        switch (request.command) {
        case DI_IOCTL_DVD_LOW_READ_DISK_ID:
            request.output.assign(request.output_size, 0U);
            return DI_RESULT_SUCCESS;
        case DI_IOCTL_DVD_LOW_READ: {
            const auto length = read_be_u32(request.input, 4U);
            const auto position = read_be_u32(request.input, 8U);
            if (!_di.partition_open || request.output_size < length) {
                _di.last_error = static_cast<u32>(DI_RESULT_SECURITY_ERROR);
                return DI_RESULT_SECURITY_ERROR;
            }
            _di.last_length = position;
            request.output.assign(length, 0U);
            return DI_RESULT_SUCCESS;
        }
        case DI_IOCTL_DVD_LOW_WAIT_FOR_COVER_CLOSE:
            set_disc_inserted(true);
            return DI_RESULT_COVER_CLOSED;
        case DI_IOCTL_DVD_LOW_GET_COVER_REGISTER:
            write_output_u32(request, _di.cover_register);
            return DI_RESULT_SUCCESS;
        case DI_IOCTL_DVD_LOW_GET_LENGTH:
            write_output_u32(request, _di.last_length);
            return DI_RESULT_SUCCESS;
        case DI_IOCTL_DVD_LOW_GET_COVER_STATUS:
            write_output_u32(request, _di.disc_inserted ? DI_COVER_STATUS_DISC_INSERTED : DI_COVER_STATUS_NO_DISC);
            return DI_RESULT_SUCCESS;
        case DI_IOCTL_DVD_LOW_OPEN_PARTITION:
            _di.last_error = static_cast<u32>(DI_RESULT_SECURITY_ERROR);
            return DI_RESULT_SECURITY_ERROR;
        case DI_IOCTL_DVD_LOW_CLOSE_PARTITION:
            _di.partition_open = false;
            _di.partition_offset = 0U;
            return DI_RESULT_SUCCESS;
        case DI_IOCTL_DVD_LOW_GET_STATUS_REGISTER:
            write_output_u32(request, _di.status_register);
            return DI_RESULT_SUCCESS;
        case DI_IOCTL_DVD_LOW_REQUEST_ERROR:
            write_output_u32(request, _di.last_error);
            return DI_RESULT_SUCCESS;
        default:
            _di.last_error = static_cast<u32>(DI_RESULT_SECURITY_ERROR);
            return DI_RESULT_SECURITY_ERROR;
        }
    }

    s32 WiiIosService::result_for_di_ioctlv(WiiIosRequest &request) {
        switch (request.command) {
        case DI_IOCTL_DVD_LOW_OPEN_PARTITION:
            _di.partition_open = true;
            _di.partition_offset = static_cast<std::uint64_t>(read_be_u32(request.input, 4U)) << 2U;
            request.output.assign(request.output_size, 0U);
            return DI_RESULT_SUCCESS;
        default:
            _di.last_error = static_cast<u32>(DI_RESULT_BAD_ARGUMENT);
            return DI_RESULT_BAD_ARGUMENT;
        }
    }

    std::string WiiIosService::path_for_handle(s32 file_handle) const {
        if (const auto path = device_path(file_handle)) {
            return std::string(*path);
        }
        return {};
    }

    std::uint64_t WiiIosService::submit_request(WiiIosRequest request, std::uint64_t delay_frames) {
        request.id = _next_request_id++;
        request.submitted_frame = _frame_index;
        request.completion_frame = _frame_index + delay_frames;
        _requests.push_back(std::move(request));
        complete_ready_requests();
        return _requests.back().id;
    }

    void WiiIosService::write_output_u32(WiiIosRequest &request, u32 value) const {
        request.output.assign(request.output_size, 0U);
        if (request.output.size() < sizeof(u32)) {
            return;
        }

        request.output[0U] = static_cast<std::uint8_t>(value >> 24U);
        request.output[1U] = static_cast<std::uint8_t>(value >> 16U);
        request.output[2U] = static_cast<std::uint8_t>(value >> 8U);
        request.output[3U] = static_cast<std::uint8_t>(value);
    }

    void WiiIosService::complete_ready_requests() {
        for (auto &request : _requests) {
            if (request.completed || request.completion_frame > _frame_index) {
                continue;
            }

            request.result = result_for_request(request);
            request.completed = true;
            if (request.kind == WiiIosRequestKind::Open && request.result > 0) {
                request.file_handle = request.result;
            }
            _completed_requests.push_back(request);
        }
    }

}  // namespace smgpc::runtime
