# Original water membership query

Recovered `MR::getWaterAreaObj(WaterInfo*, const TVec3f&)` in root
`src/Game/Util/AreaObjUtil.cpp` before changing any PC caller. Rebuilt with the
original GC/3.0a3 compiler and current `configure.py` Game flags. All 28 retail
instructions at RMGK01 `0x80400964` match after eight checked relocations.
The oracle also checks the actual `Water` string and current disc DOL SHA1.

Run `python3 pc-port/notes/water-area-restoration-20260903/verify.py` from the
repository root. The verifier uses the committed OnlyCamera ELF/DOL readers;
build objects and regenerated evidence go under ignored `build/`.

The routine clears the complete WaterInfo, queries the real area container for
`Water`, records the matched WaterArea pointer and returns true if found. If
there is no area match, it returns the result of the original
`WaterAreaFunction::tryInOceanArea`. This preserves both the clearing order and
the priority of authored water areas over ocean actors. It is a boolean result,
not an area pointer.

This is a root decompilation checkpoint. It is not yet copied into the active
PC utility provider: the PC scene still lacks a complete WaterAreaHolder and
ocean actor query closure. In particular, the existing explicit unavailable
`MR::isInWater` boundary is not replaced with a fixed false result. The original
holder query itself returns false when its real scene object is absent, but an
incomplete installed water system must not be represented as an empty one.

This dependency was found through Mario's original jump/swim query path. It
applies to all water users and contains no Gateway-specific policy. Follow-up
work must install real area/holder ownership and ocean shape queries before
calling it from the PC original movement lifecycle.
