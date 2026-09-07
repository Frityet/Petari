#pragma once

#include <aurora/nand.hpp>
#include <aurora/sysconf.hpp>
#include <revolution/sc.h>
#include <array>
#include <optional>

namespace smgpc::runtime {
    // The runtime owns this after loading its actual NAND files. This owns the
    // decoded SDK catalog and lends the real OS product-memory range until
    // retirement. It never derives settings from a renderer/camera or disc ID.
    class SystemConfigService final {
    public:
        explicit SystemConfigService(aurora::NandFileSystem&);
        ~SystemConfigService();
        SystemConfigService(const SystemConfigService&) = delete;
        SystemConfigService& operator=(const SystemConfigService&) = delete;
        void reload();
        [[nodiscard]] bool find_array(void*, std::uint32_t, SCItemID) const;
        [[nodiscard]] bool find_integer(void*, SCItemID, aurora::SysConf::Type) const;
        [[nodiscard]] bool replace_array(const void*, std::uint32_t, SCItemID);
        [[nodiscard]] bool replace_u8(std::uint8_t, SCItemID);
        [[nodiscard]] bool dirty() const noexcept;
        [[nodiscard]] bool index_valid() const noexcept;
        [[nodiscard]] const aurora::SysConf& document() const noexcept;
        [[nodiscard]] static SystemConfigService& require_active();
        [[nodiscard]] static SystemConfigService* active() noexcept;
    private:
        void load();
        [[nodiscard]] const aurora::SysConf::Entry* find(SCItemID) const;
        void erase(SCItemID);
        [[nodiscard]] bool create(SCItemID, aurora::SysConf::Type, std::span<const std::uint8_t>);
        aurora::NandFileSystem* _nand;
        aurora::SysConf _document;
        std::array<std::optional<std::size_t>, SC_ITEM_ID_MAX_PLUS1> _indices{};
        std::array<std::uint8_t, 0x100> _previous_product{};
        bool _index_valid = false;
        bool _dirty = false;
    };
}
