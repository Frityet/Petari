#include "runtime/SystemConfigService.hpp"
#include <aurora/aurora.h>
#include <aurora/sysconf.hpp>
#include <revolution/sc.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    using Type = aurora::SysConf::Type;
    using Service = smgpc::runtime::SystemConfigService;
    constexpr auto config_path = "/shared2/sys/SYSCONF";
    constexpr auto product_path = "/title/00000001/00000002/data/setting.txt";
    void require(bool good, const char* message) { if (!good) throw std::runtime_error(message); }
    std::array<u8, 256> encrypted(std::string_view text) {
        std::array<u8, 256> result{};
        std::uint32_t seed = 0x73B5DBFA;
        for (std::size_t i = 0; i < text.size(); ++i) {
            result[i] = static_cast<u8>(text[i]) ^ static_cast<u8>(seed);
            require(result[i] != 0, "fixture text does not use an ambiguous zero cipher byte");
            seed = (seed << 1) | (seed >> 31);
        }
        return result;
    }
    void write(aurora::NandFileSystem& nand, const aurora::SysConf& doc) {
        const auto bytes = doc.encode();
        nand.write_file(config_path, bytes);
    }
    void byte(aurora::SysConf& doc, const char* name, u8 value, Type type = Type::Byte) {
        doc.replace_integer(name, type, value);
    }
    void defaults() {
        aurora::NandFileSystem nand;
        Service service(nand);
        require(service.index_valid() && !service.dirty(), "missing SYSCONF clears then parses an empty SDK catalog");
        require(SCGetAspectRatio() == 0 && SCGetDisplayOffsetH() == 0 && SCGetEuRgb60Mode() == 0 &&
                    SCGetProgressiveMode() == 0 && SCGetScreenSaverMode() == 1 && SCGetSoundMode() == 1 &&
                    SCGetCounterBias() == 189388800 && SCGetBtDpdSensibility() == 2 && SCGetWpadMotorMode() == 1 &&
                    SCGetWpadSensorBarPosition() == 0 && SCGetWpadSpeakerVolume() == 89,
                "all original numeric defaults run unchanged");
        require(SCGetProductArea() == -1 && SCGetProductGameRegion() == -1 && SCGetLanguage() == 1,
                "absent product data uses original unknown-area and English-language fallback");
        SCIdleModeInfo idle{0xA5, 0x5A};
        require(!SCGetIdleMode(&idle) && idle.mode == 0xA5 && idle.led == 0x5A,
                "failed array lookup preserves caller output");
        u32 sentinel = 0xDEADBEEF;
        require(!SCFindU32Item(&sentinel, static_cast<SCItemID>(-1)) && sentinel == 0xDEADBEEF,
                "invalid SDK id leaves integer output untouched");
        require(SCSetWpadMotorMode(0) && service.dirty() && SCGetWpadMotorMode() == 0,
                "missing valid config supports original item creation");
        require(!nand.exists(config_path), "SDK replacement changes RAM without inventing persistence");
    }
    void product() {
        for (auto [name, area, game, region] : std::array{
            std::tuple{"JPN", 0, "JP", 0}, std::tuple{"USA", 1, "US", 1},
            std::tuple{"EUR", 2, "EU", 2}, std::tuple{"KOR", 6, "KR", 4},
            std::tuple{"CHN", 11, "CN", 5}, std::tuple{"ROC", 5, "ZZ", -1}}) {
            aurora::NandFileSystem nand;
            const auto bytes = encrypted(std::string("AREA=") + name + "\r\nGAME=" + game + "\r\n");
            nand.write_file(product_path, bytes);
            Service service(nand);
            require(SCGetProductArea() == area && SCGetProductGameRegion() == region,
                    "original encrypted product decoder and code tables run against real mapped memory");
            require(SCGetLanguage() == (area == 0 ? 0 : 1), "original language default depends on actual product area");
            char small[2] = {'x', 'y'};
            require(!__SCF1("AREA", small, sizeof(small)) && small[0] == name[0] && small[1] == name[1],
                    "short product buffer retains original partial-output failure semantics");
        }
    }
    void typed_ranges() {
        aurora::NandFileSystem nand;
        aurora::SysConf doc;
        byte(doc, "IPL.AR", 1, Type::Bool);
        byte(doc, "IPL.LNG", 9, Type::Bool);
        doc.replace_integer("IPL.CB", Type::Long, 0x12345678);
        doc.replace_integer("BT.SENS", Type::Long, 99);
        doc.replace({Type::BigArray, "IPL.IDL", {3, 4}});
        byte(doc, "BT.SPKV", 89);
        write(nand, doc);
        Service service(nand);
        require(SCGetAspectRatio() == 0 && SCGetLanguage() == 1,
                "BOOL resource kind must not satisfy an original byte accessor");
        require(SCGetCounterBias() == 0x12345678 && SCGetBtDpdSensibility() == 5,
                "big-endian words reach original scalar getters and range handling");
        SCIdleModeInfo idle{};
        require(SCGetIdleMode(&idle) && idle.mode == 3 && idle.led == 4,
                "both original array wire types support exact-size byte-array lookup");
        std::array<u8, 3> wrong{8, 9, 10};
        require(!SCFindByteArrayItem(wrong.data(), wrong.size(), SC_ITEM_ID_IPL_IDLE_MODE) && wrong[0] == 8,
                "array length mismatch does not partially overwrite output");
        require(SCSetWpadSpeakerVolume(89) && !service.dirty(), "identical same-type replacement does not mark dirty");
        for (unsigned value = 0; value < 256; ++value) {
            require(SCReplaceU8Item(value, SC_ITEM_ID_IPL_DISPLAY_OFFSET_H), "write raw display byte");
            const auto signed_value = value < 128 ? int(value) : int(value) - 256;
            const auto clamped = std::clamp(signed_value, -32, 32);
            require(SCGetDisplayOffsetH() == (clamped & ~1), "original signed display clamp/even behavior for all256 bytes");
        }
        require(SCSetWpadSpeakerVolume(255) && SCGetWpadSpeakerVolume() == 127,
                "setter preserves raw byte while original getter clamps volume");
        require(SCSetWpadMotorMode(2) && SCGetWpadMotorMode() == 0,
                "setter preserves raw mode while original getter accepts only one");
        require(SCReplaceU8Item(1, SC_ITEM_ID_IPL_ASPECT_RATIO) && SCGetAspectRatio() == 1,
                "type replacement removes BOOL and creates the original BYTE kind");
    }
    void arrays() {
        aurora::NandFileSystem nand;
        Service service(nand);
        SCBtDeviceInfoArray devices{};
        devices.num = 2;
        devices.info[0].bd_addr[5] = 0x42;
        devices.info[1].bd_name[63] = 0x7D;
        require(SCSetBtDeviceInfoArray(&devices), "actual device array setter creates its exact SDK structure");
        SCBtDeviceInfoArray result{};
        require(SCGetBtDeviceInfoArray(&result) && std::memcmp(&devices, &result, sizeof(result)) == 0,
                "original device array roundtrip retains all payload bytes");
        SCBtCmpDevInfoArray paired{};
        paired.num = 1;
        paired.info[0].link_key[15] = 0xB7;
        require(SCSetBtCmpDevInfoArray(&paired), "actual paired device setter");
        SCBtCmpDevInfoArray paired_result{};
        require(SCGetBtCmpDevInfoArray(&paired_result) && std::memcmp(&paired, &paired_result, sizeof(paired)) == 0,
                "actual paired device getter retains full layout");
        std::array<u8, 257> payload{};
        require(SCReplaceByteArrayItem(payload.data(), 256, SC_ITEM_ID_NET_CONFIG) &&
                    service.document().find("NET.CNF")->type == Type::SmallArray,
                "original256-byte threshold selects small-array encoding");
        require(SCReplaceByteArrayItem(payload.data(), 257, SC_ITEM_ID_NET_CONFIG) &&
                    service.document().find("NET.CNF")->type == Type::BigArray,
                "original257-byte threshold selects big-array encoding");
        require(!SCReplaceByteArrayItem(nullptr, 0, SC_ITEM_ID_NET_CONFIG) &&
                    service.document().find("NET.CNF")->data.size() == 257,
                "null replacement does not delete an existing array");
    }
    void duplicates() {
        aurora::SysConf doc;
        doc.append({Type::Byte, "IPL.AR", {0}});
        doc.append({Type::Byte, "IPL.AR", {1}});
        doc.append({Type::LongLong, "Unknown", {1, 2, 3, 4, 5, 6, 7, 8}});
        aurora::NandFileSystem nand;
        write(nand, doc);
        Service service(nand);
        require(SCGetAspectRatio() == 0, "original ID index selects the first duplicate only on load");
        std::array<u8, 2> array{4, 5};
        require(SCReplaceByteArrayItem(array.data(), array.size(), SC_ITEM_ID_IPL_ASPECT_RATIO), "type-changing replacement succeeds");
        require(SCGetAspectRatio() == 0 && service.document().entries().back().name == "IPL.AR" &&
                    service.document().entries().back().type == Type::SmallArray,
                "replacement appends its selected record, leaving the older duplicate unselected");
        require(!SCReplaceByteArrayItem(array.data(), 0, SC_ITEM_ID_IPL_ASPECT_RATIO) && service.dirty(),
                "invalid new size preserves original delete-before-create failure and dirty state");
        require(SCGetAspectRatio() == 0, "failed replacement does not silently reselect a remaining duplicate");
        require(service.document().find("Unknown")->data == std::vector<u8>({1,2,3,4,5,6,7,8}),
                "unrecognized records and original LongLong payload survive SDK mutations");
        write(nand, service.document());
        service.reload();
        require(SCGetAspectRatio() == 1 && !service.dirty(), "explicit reload rebuilds the first-match ID index");
    }
    void malformed_capacity() {
        aurora::NandFileSystem nand;
        auto malformed = aurora::SysConf{}.encode();
        malformed[0] = 'X';
        nand.write_file(config_path, malformed);
        Service service(nand);
        require(!service.index_valid() && SCGetAspectRatio() == 0 && !SCSetWpadMotorMode(1),
                "malformed full-size data clears getters but leaves original runtime index invalid");
        nand.write_file(config_path, std::span(malformed).first(64));
        service.reload();
        require(service.index_valid() && SCSetWpadMotorMode(1),
                "short NAND read follows original cleared-file parse path");
        aurora::SysConf full;
        full.append({Type::BigArray, "X", std::vector<u8>(16296, 0xA5)});
        write(nand, full);
        service.reload();
        require(service.index_valid() && !SCSetWpadMotorMode(1) && !service.dirty(),
                "original70-byte runtime-tail budget prevents an over-capacity create");
        require(service.document().find("X")->data.size() == 16296, "capacity failure preserves the unmodified existing resource");
    }
    void lifecycle() {
        auto* memory = static_cast<u8*>(OSPhysicalToCached(0x3800));
        const auto boot = encrypted("AREA=JPN\r\nGAME=JP\r\n");
        std::copy(boot.begin(), boot.end(), memory);
        memory[255] = 0xA5;
        aurora::NandFileSystem nand;
        {
            Service service(nand);
            require(SCGetProductArea() == 0 && SCGetLanguage() == 0 && memory[255] == 0,
                    "absent setting.txt retains real OS boot product data and original terminator rule");
            bool rejected = false;
            try { Service duplicate(nand); } catch (const std::logic_error&) { rejected = true; }
            require(rejected && Service::active() == &service && SCGetProductArea() == 0,
                    "overlap failure preserves existing owner and mapped product bytes");
        }
        require(Service::active() == nullptr && std::equal(boot.begin(), boot.end() - 1, memory) && memory[255] == 0xA5,
                "service retirement restores prior OS memory and unpublishes its identity");
        bool rejected = false;
        try { (void)SCGetAspectRatio(); } catch (const std::logic_error&) { rejected = true; }
        require(rejected, "missing owner is explicit and does not invent a console setting");
        std::fill_n(memory, 256, 0);
        Service recreated(nand);
        require(SCGetProductArea() == -1 && SCGetAspectRatio() == 0, "subsequent owner uses its actual backing state");
    }
}
int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        OSInit();
        defaults(); std::cout << "PASS original defaults and failed outputs\n";
        product(); std::cout << "PASS original encrypted product/region/language accessors\n";
        typed_ranges(); std::cout << "PASS exact scalar types, byte order and original range rules\n";
        arrays(); std::cout << "PASS actual SDK Bluetooth arrays and array thresholds\n";
        duplicates(); std::cout << "PASS duplicate ID indexing and delete-before-create semantics\n";
        malformed_capacity(); std::cout << "PASS malformed/short source and original SDK capacity\n";
        lifecycle(); std::cout << "PASS runtime owner publication, failure and mapped-memory restoration\n";
    } catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
