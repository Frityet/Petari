#include "runtime/SystemConfigService.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace smgpc::runtime {
    namespace {
        using Type = aurora::SysConf::Type;
        std::atomic<SystemConfigService*> active_service;
        struct InterruptScope {
            BOOL enabled = OSDisableInterrupts();
            ~InterruptScope() { OSRestoreInterrupts(enabled); }
        };
        // Literal original scsystem.c NameAndIDTbl order.
        constexpr std::array<std::string_view, SC_ITEM_ID_MAX_PLUS1> names{
            "IPL.CB", "IPL.AR", "IPL.ARN", "IPL.CD", "IPL.CD2", "IPL.DH", "IPL.E60", "IPL.EULA",
            "IPL.FRC", "IPL.IDL", "IPL.INC", "IPL.LNG", "IPL.NIK", "IPL.PC", "IPL.PGS", "IPL.SSV",
            "IPL.SADR", "IPL.SND", "IPL.UPT", "NET.CNF", "NET.CTPC", "NET.PROF", "NET.WCPC", "NET.WCFG",
            "DEV.BTM", "DEV.VIM", "DEV.CTC", "DEV.DSM", "BT.DINF", "BT.CDIF", "BT.SENS", "BT.SPKV",
            "BT.MOT", "BT.BAR", "DVD.CNF", "WWW.RST"
        };
        constexpr std::string_view conf_path = "/shared2/sys/SYSCONF";
        constexpr std::string_view product_path = "/title/00000001/00000002/data/setting.txt";
        constexpr std::size_t sdk_data_end = aurora::SysConf::FileSize - 4 - 70;
        bool valid_id(SCItemID id) { return static_cast<std::uint32_t>(id) < names.size(); }
        bool array_type(Type type) { return type == Type::SmallArray || type == Type::BigArray; }
        std::size_t packed_size(const aurora::SysConf::Entry& entry) {
            return 1 + entry.name.size() + entry.data.size() +
                   (entry.type == Type::SmallArray ? 1 : entry.type == Type::BigArray ? 2 : 0);
        }
        std::size_t data_end(const aurora::SysConf& doc) {
            std::size_t end = 6 + 2 * (doc.entries().size() + 1);
            for (const auto& entry : doc.entries()) end += packed_size(entry);
            return end;
        }
        std::uint8_t* product_memory() {
            if (OSBaseAddress == 0 || OSGetPhysicalMemSize() < 0x3900)
                throw std::logic_error("System configuration requires initialized original OS memory");
            return static_cast<std::uint8_t*>(OSPhysicalToCached(0x3800));
        }
    }

    SystemConfigService::SystemConfigService(aurora::NandFileSystem& nand) : _nand(&nand) {
        compat::JkrHostAllocationScope host;
        InterruptScope interrupts;
        if (active_service.load()) throw std::logic_error("A system configuration owner is already installed");
        std::copy_n(product_memory(), _previous_product.size(), _previous_product.begin());
        load();
        active_service.store(this);
    }
    SystemConfigService::~SystemConfigService() {
        compat::JkrHostAllocationScope host;
        InterruptScope interrupts;
        active_service.store(nullptr);
        std::copy(_previous_product.begin(), _previous_product.end(), product_memory());
    }
    SystemConfigService* SystemConfigService::active() noexcept { return active_service.load(); }
    SystemConfigService& SystemConfigService::require_active() {
        auto* service = active();
        if (!service) throw std::logic_error("Original SC lookup requires an installed system configuration owner");
        return *service;
    }
    void SystemConfigService::load() {
        // Match SC's fixed-size NAND reads: a short/missing SYSCONF read is
        // cleared before parsing; malformed full data fails its runtime index.
        auto bytes = _nand->read_file(conf_path);
        auto product = _nand->read_file(product_path);
        aurora::SysConf decoded;
        bool valid = true;
        if (bytes && bytes->size() >= aurora::SysConf::FileSize) {
            try {
                decoded = aurora::SysConf::decode(std::span(*bytes).first(aurora::SysConf::FileSize));
                valid = data_end(decoded) <= sdk_data_end;
            } catch (const std::invalid_argument&) { valid = false; }
            if (!valid) decoded = {};
        }
        std::array<std::optional<std::size_t>, SC_ITEM_ID_MAX_PLUS1> indices{};
        if (valid) {
            for (std::size_t id = 0; id < names.size(); ++id) {
                for (std::size_t i = 0; i < decoded.entries().size(); ++i) {
                    if (decoded.entries()[i].name == names[id]) { indices[id] = i; break; }
                }
            }
        }
        _document = std::move(decoded);
        _indices = indices;
        _index_valid = valid;
        _dirty = false;
        // Original failed/short product reads retain untouched boot bytes.
        auto* output = product_memory();
        if (product) std::copy_n(product->begin(), std::min(product->size(), _previous_product.size()), output);
        output[0xFF] = 0; // Original FinishFromReload's terminating byte.
    }
    void SystemConfigService::reload() {
        compat::JkrHostAllocationScope host;
        InterruptScope interrupts;
        load();
    }
    const aurora::SysConf::Entry* SystemConfigService::find(SCItemID id) const {
        if (!valid_id(id) || !_index_valid || !_indices[id]) return nullptr;
        return &_document.entries()[*_indices[id]];
    }
    bool SystemConfigService::find_array(void* output, std::uint32_t size, SCItemID id) const {
        InterruptScope interrupts;
        const auto* entry = find(id);
        if (!output || !entry || !array_type(entry->type) || entry->data.size() != size) return false;
        std::memcpy(output, entry->data.data(), size);
        return true;
    }
    bool SystemConfigService::find_integer(void* output, SCItemID id, Type type) const {
        InterruptScope interrupts;
        const auto* entry = find(id);
        if (!entry || entry->type != type) return false;
        if (type == Type::Byte) std::memcpy(output, entry->data.data(), 1);
        else if (type == Type::Long) {
            const auto& b = entry->data;
            const std::uint32_t value = (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) |
                                        (std::uint32_t(b[2]) << 8) | b[3];
            std::memcpy(output, &value, sizeof(value));
        } else throw std::logic_error("Unsupported original SC integer accessor type");
        return true;
    }
    void SystemConfigService::erase(SCItemID id) {
        const auto index = *_indices[id];
        _document.erase_at(index);
        for (auto& item : _indices) {
            if (item && *item == index) item.reset();
            else if (item && *item > index) --*item;
        }
        _dirty = true;
    }
    bool SystemConfigService::create(SCItemID id, Type type, std::span<const std::uint8_t> bytes) {
        if (!valid_id(id) || !_index_valid || bytes.empty() || bytes.size() > 65536) return false;
        const auto record_size = 1 + names[id].size() + bytes.size() +
                                 (type == Type::SmallArray ? 1 : type == Type::BigArray ? 2 : 0);
        if (2 + record_size > sdk_data_end - data_end(_document)) return false;
        aurora::SysConf::Entry entry{type, std::string(names[id]), {bytes.begin(), bytes.end()}};
        _document.append(std::move(entry));
        _indices[id] = _document.entries().size() - 1;
        _dirty = true;
        return true;
    }
    bool SystemConfigService::replace_array(const void* data, std::uint32_t size, SCItemID id) {
        compat::JkrHostAllocationScope host;
        InterruptScope interrupts;
        if (!data) return false;
        if (const auto* existing = find(id)) {
            if (array_type(existing->type) && existing->data.size() == size) {
                if (std::memcmp(existing->data.data(), data, size) != 0) {
                    auto replacement = *existing;
                    replacement.data.assign(static_cast<const std::uint8_t*>(data), static_cast<const std::uint8_t*>(data) + size);
                    _document.replace_at(*_indices[id], std::move(replacement));
                    _dirty = true;
                }
                return true;
            }
            erase(id);
        }
        if (size == 0 || size > 65536) return false;
        return create(id, size <= 256 ? Type::SmallArray : Type::BigArray,
                      {static_cast<const std::uint8_t*>(data), size});
    }
    bool SystemConfigService::replace_u8(std::uint8_t value, SCItemID id) {
        compat::JkrHostAllocationScope host;
        InterruptScope interrupts;
        if (const auto* existing = find(id)) {
            if (existing->type == Type::Byte) {
                if (existing->data.front() != value) {
                    auto replacement = *existing;
                    replacement.data.front() = value;
                    _document.replace_at(*_indices[id], std::move(replacement));
                    _dirty = true;
                }
                return true;
            }
            erase(id);
        }
        return create(id, Type::Byte, {&value, 1});
    }
    bool SystemConfigService::dirty() const noexcept { return _dirty; }
    bool SystemConfigService::index_valid() const noexcept { return _index_valid; }
    const aurora::SysConf& SystemConfigService::document() const noexcept { return _document; }
}
