#include "Game/System/RenderMode.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/SystemConfigService.hpp"
#include "runtime/WiiVideoService.hpp"
#include <aurora/aurora.h>
#include <aurora/sysconf.hpp>
#include <aurora/vi.hpp>
#include <array>
#include <iostream>
#include <stdexcept>

namespace aurora { extern AuroraConfig g_config; }
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct RestoreCapability {
    bool previous = aurora::vi::dtv_connected();
    ~RestoreCapability() { aurora::vi::set_dtv_connected(previous); VIConfigure(nullptr); }
};
GXRenderModeObj boot_mode(u32 format, u32 scan) {
    GXRenderModeObj mode{};
    mode.viTVmode = static_cast<VITVMode>(VI_TVMODE(format, scan));
    mode.fbWidth = 640;
    mode.efbHeight = 456;
    return mode;
}
void independent_connection() {
    RestoreCapability restore;
    VIConfigure(nullptr);
    VIInit();
    require(VIGetScanMode() == VI_INTERLACE && VIGetDTVStatus() == 1,
            "desktop capability exists before a progressive scan has been selected");
    smgpc::runtime::WiiVideoService service;
    for (bool connected : {false, true}) {
        aurora::vi::set_dtv_connected(connected);
        for (u32 scan : {VI_INTERLACE, VI_NON_INTERLACE, VI_PROGRESSIVE}) {
            auto mode = boot_mode(VI_NTSC, scan);
            service.configure(&mode);
            require(VIGetScanMode() == scan && VIGetDTVStatus() == connected && service.dtv_status() == connected,
                    "SDK and native video service expose one connection independent of configured scan");
            VIInit();
            require(VIGetDTVStatus() == connected, "VI initialization does not change platform connection");
        }
    }
}
void tv_formats() {
    smgpc::runtime::WiiVideoService service;
    constexpr std::array<u32, 9> expected{0, 1, 2, 0, 1, 5, 0, 0, 0};
    for (u32 format = 0; format < expected.size(); ++format) {
        for (u32 scan : {VI_INTERLACE, VI_NON_INTERLACE, VI_PROGRESSIVE}) {
            const auto mode = boot_mode(format, scan);
            service.configure(&mode);
            require(VIGetTvFormat() == expected[format] && service.tv_format() == expected[format], "SDK TV family follows actual retail nine-entry dispatch table");
        }
    }
}
void original_selection() {
    RestoreCapability restore;
    std::size_t count = 0;
    for (u32 tv : {VI_NTSC, VI_PAL, VI_MPAL, VI_EURGB60}) {
        for (u32 scan : {VI_INTERLACE, VI_NON_INTERLACE, VI_PROGRESSIVE}) {
            for (bool connected : {false, true}) {
                for (bool progressive : {false, true}) {
                    for (bool wide : {false, true}) {
                        for (bool rgb60 : {false, true}) {
                            aurora::SysConf document;
                            document.replace_integer("IPL.PGS", aurora::SysConf::Type::Byte, progressive);
                            document.replace_integer("IPL.AR", aurora::SysConf::Type::Byte, wide);
                            document.replace_integer("IPL.E60", aurora::SysConf::Type::Byte, rgb60);
                            aurora::NandFileSystem nand;
                            nand.write_file("/shared2/sys/SYSCONF", document.encode());
                            smgpc::runtime::SystemConfigService configuration(nand);
                            auto initial = boot_mode(tv, scan);
                            VIConfigure(&initial);
                            aurora::vi::set_dtv_connected(connected);
                            const GXRenderModeObj* selected = MR::getSuitableRenderMode();
                            const bool expected_progressive = progressive && connected;
                            const bool pal50 = tv == VI_PAL && !expected_progressive && !rgb60;
                            const u32 expected_tv = pal50 ? VI_PAL : (tv == VI_PAL || tv == VI_EURGB60) ? VI_EURGB60 : VI_NTSC;
                            const auto expected_scan = expected_progressive ? VI_PROGRESSIVE : VI_INTERLACE;
                            require(selected && selected->viTVmode == VI_TVMODE(expected_tv, expected_scan),
                                    "unchanged original RenderMode selects from connection, SC preference and TV family");
                            require(selected->fbWidth == 640 && selected->efbHeight == 456 &&
                                    selected->xfbHeight == (pal50 ? 542 : 456) &&
                                    selected->viWidth == (pal50 ? (wide ? 682 : 666) : (wide ? 686 : 670)) &&
                                    selected->xFBmode == (expected_progressive ? VI_XFBMODE_SF : VI_XFBMODE_DF),
                                    "actual original mode tables retain authored dimensions, aspect index and framebuffer policy");
                            require(VIGetScanMode() == scan, "selecting a mode does not configure it early");
                            VIConfigure(selected);
                            require(VIGetScanMode() == expected_scan && VIGetDTVStatus() == connected,
                                    "applying the selected mode changes scan, not platform connection");
                            ++count;
                        }
                    }
                }
            }
        }
    }
    require(count == 192, "all scan, connection, SC progressive, aspect, RGB60 and TV combinations executed");
}
void defaults_and_invalid_config() {
    RestoreCapability restore;
    aurora::NandFileSystem nand;
    smgpc::runtime::SystemConfigService configuration(nand);
    const auto initial = boot_mode(VI_NTSC, VI_PROGRESSIVE);
    VIConfigure(&initial);
    aurora::vi::set_dtv_connected(true);
    require(MR::getSuitableRenderMode()->viTVmode == VI_TVMODE_NTSC_INT &&
                MR::getSuitableRenderMode()->viWidth == 670,
            "missing settings preserve actual original progressive-off and 4:3 defaults even with capability");
    aurora::SysConf document;
    document.replace_integer("IPL.PGS", aurora::SysConf::Type::Byte, 2);
    document.replace_integer("IPL.AR", aurora::SysConf::Type::Byte, 2);
    nand.write_file("/shared2/sys/SYSCONF", document.encode());
    configuration.reload();
    require(MR::getSuitableRenderMode()->viTVmode == VI_TVMODE_NTSC_INT &&
                MR::getSuitableRenderMode()->viWidth == 670,
            "invalid stored bytes retain original SDK normalization before mode selection");
}
}
int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime process;
        independent_connection(); std::cout << "PASS independent desktop DTV connection and service delegation\n";
        tv_formats(); std::cout << "PASS all nine retail TV-format mappings across three scan modes\n";
        original_selection(); std::cout << "PASS 192 original RenderMode selections with actual SC data\n";
        defaults_and_invalid_config(); std::cout << "PASS missing and invalid SC original selection defaults\n";
    } catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
