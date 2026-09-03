# Original VI boot capability and RenderMode selection

The concrete defect is a circular progressive-selection dependency. Aurora `VIGetDTVStatus()` returned whether the currently configured mode was progressive. The original game asks for DTV status **before** choosing/configuring its render mode. An interlaced boot therefore returned false and blocked progressive selection even when the actual saved preference enabled it. Native `WiiVideoService::dtv_status()` repeated the same error.

## Retail evidence and startup order

RMGK01 revision 0 DOL SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. Existing root `src/RVL_SDK/vi/vi.c::VIGetDTVStatus` at 0x804B5E6C (60 bytes) reads hardware halfword **0xCC00206E** and returns its low bit under interrupt protection. It does not consult scan mode. The stored hardware bit is the connected component/DTV-output signal; Dolphin's actual VI register declaration (`VideoInterface.h::UVIDTVStatus`) independently names bit 0 `component_plugged`. Its configuration choice is not used as a native Game rule here.

Original `VIGetScanMode` consults the scan/clock registers (0x36 and 0x01), a separate state. The root `VIInit` initializes disabled hardware to NTSC/interlace, otherwise imports existing scan and boot TV state; it does not change the DTV connection bit. Original VIInit is idempotent. This patch does not attempt its entire register lifecycle.

Original `__start` calls `OSInit`. In the normal non-NAND-boot/non-reboot branch, OSInit calls `SCInit` and waits while `SCCheckStatus()==1` before returning. Game `main` then calls VIInit; VIInit itself reads SC display offset. `GameSystem::init` constructs GameSystemObjHolder (its language also queries SC), creates GX, and calls holder init. Holder init calls `initRenderMode`, which copies the result of unchanged `MR::getSuitableRenderMode` and then creates JUTVideo. JUTVideo calls VIInit again and applies the selected render mode. Thus SC and platform VI connection/boot state exist before mode choice, and mode choice precedes JUTVideo/capture/camera resources.

At retail 0x803A7274 (308 bytes), the selection order is SC aspect, VI DTV connection, conditionally SC progressive, then VI TV family. Native and root RenderMode.cpp are byte-identical. The unchanged source selects:

| Input family | DTV and progressive preference both enabled | Otherwise |
|---|---|---|
| NTSC / MPAL | NTSC progressive table | NTSC interlaced table |
| PAL | EURGB60 progressive table | PAL interlaced unless SC EURGB60 enabled |
| EURGB60 | EURGB60 progressive table | EURGB60 interlaced table |

The aspect setting selects the corresponding table entry independently. A missing SC record still uses the original progressive-off / 4:3 defaults; capability alone never enables the preference. CameraContext's aspect continues to come from the original screen/SC path, not the host window or DTV status.

## TV family normalization

Retail `VIGetTvFormat` at 0x804B5E0C (96 bytes), with jump table 0x805FD770, maps current modes `[0..8]` to `[0,1,2,0,1,5,0,0,0]`. **EURGB60 remains 5**; it is not normalized to PAL or NTSC. Debug variants map to their NTSC/PAL families. The actual binary preserves its current format for values above 8. The staged provider applies that exact normalization; WiiVideoService delegates its TV-family query to the SDK getter too, avoiding a second divergent decoder.

`verify-retail.py` checks the verified DOL hash, status-register load/masks and interrupt calls, every TV dispatch destination, the out-of-range branch, RenderMode call order and exact root/native RenderMode source identity. The three disassemblies and `retail-evidence.json` retain the inspected evidence. No new root decompilation or Game algorithm modification was needed.

## Bounded general correction

New public `aurora/vi.hpp` provides `set_dtv_connected(bool)` and `dtv_connected()`. It represents independent platform output connection capability with atomic storage. The desktop default is connected because its output can present progressive frames; a platform owner can explicitly disconnect it. VIInit/VIConfigure and changes to SYSCONF do not mutate this physical/platform input. VIGetDTVStatus reads this value, and the native video service reads the same SDK result.

This is intentionally separate from the requested scan mode and user preference. It introduces no Game checks, stage names, automatic setting edits, or rule that derives capability from imported SYSCONF. It also does not claim full original register timing: native VIConfigure still publishes immediately and VIFlush has no shadow-register/retrace latch. Original VIConfigure additionally checks PAL-family boot compatibility and retains boot NTSC/MPAL TV identity; those broader existing state gaps are documented, not silently approximated in this patch.

## Validation and activation

`python3 build/original-vi-boot-audit-20260903/verify-native.py` compiles the full staged VI implementation, full staged WiiVideoService, unchanged actual Game RenderMode source and fixture, and links against the existing actual SC/native owner archives. No shared xmake build or GPU was run.

All four CPU groups pass:

1. Desktop DTV capability before any progressive configuration; all three scan modes with both connection states; VIInit preserves connection; native service agrees with SDK.
2. All nine retail TV mappings across all three scan modes, including EURGB60 and native service delegation.
3. 192 actual original RenderMode choices across four TV families, three initial scans, both DTV states and each progressive/aspect/EURGB60 preference. Tests check exact original table dimensions, SF/DF selection, and that selection itself does not configure VI early.
4. Missing and invalid SC records retain original defaults despite connected output and an initially progressive scan.

`aurora.patch` applies in pc-port/aurora and changes only new aurora/vi.hpp plus vi.cpp; it preserves the parent's VI forwarding header/constants/enum work. `native.patch` applies at root and changes two WiiVideoService query bodies plus adds OriginalViRenderModeTests.cpp. Both pass apply checks. Parent owns normal target registration, production application, RuntimeContext startup and GPU validation.
# Regular native integration

The correction is now selected in Aurora and WiiVideoService. The regular
`smg-pc-original-vi-render-mode-tests` build passes all four groups, including
the 192 original selection combinations. `regular-build.log` and
`regular-test.log` retain the results.

The actual RuntimeContext construction fixture also passes with the real RVZ
and Metal after this correction: its first startup imports progressive-off
4:3 settings, and its second imports progressive-on 16:9 settings. The original
render-mode selector chooses interlaced then progressive from the independently
connected desktop output. Failure cleanup and full owner retirement still pass;
see `runtime-build.log` and `runtime-test.log`.
