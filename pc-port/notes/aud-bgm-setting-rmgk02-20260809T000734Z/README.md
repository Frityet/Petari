# AudBgmSetting RMGK02 reconstruction

## Scope

Reconstructed the ordinary root-decomp unit:

- `include/Game/AudioLib/AudBgmSetting.hpp`
- `src/Game/AudioLib/AudBgmSetting.cpp`

No `pc-port/src`, Aurora, project configuration, symbol maps, split files, staging, or commits were touched. This notes directory is intentionally under the ignored `pc-port/notes` evidence tree.

## Result

The unit implements all nine retail getters and all four retail scalar tables. The source object is exact against RMGK02:

- `.text`: 300 bytes (`0x12c`), 100% objdiff
- `.rodata`: 4,112 bytes (`0x1010`), 100% objdiff
- all nine functions: 100% individually
- `cBgmSettingInfo`: 840 bytes, exact
- mute-state table: 1,520 bytes, exact section placement/content
- extra-chord table: 16 bytes, exact section placement/content
- `cMultiBgmSet`: 1,736 bytes, exact

The two formerly unnamed retail data labels now have semantic C++ member names. Their bytes and offsets remain exact; objdiff consequently reports the whole `.rodata` section at 100%, while the individual target labels do not have same-named source symbols.

## Recovered layout

```cpp
struct BgmSettingInfo {
    s32 mMuteStateIndex;
    s32 mExtraChordIndex;
};

struct MultiBgmInfo {
    u32 mSeqId;
    u32 mStreamId;
    f32 mBeatMul;
    u32 mIntroBeats;
    u32 mLoopBeats;
    u32 mLoopStartSamples;
    u32 mLoopEndSamples;
};
```

Tables:

- 105 `BgmSettingInfo` records
- 19 mute records × 8 states × 10 bytes
- one extra-chord record × 8 `u16` values
- 62 `MultiBgmInfo` records

The File Select multi-BGM record is index `0x23`:

```text
seq=0x01000038 stream=0x02000026 beat=1.0
intro=32 loop=64 loopStart=590742 loopEnd=1772279
```

## Primary evidence

- `build/RMGK02/asm/Game/AudioLib/AudBgmSetting.s`
- `build/RMGK02/obj/Game/AudioLib/AudBgmSetting.o`
- `build/RMGK02/main.elf`
- `orig/RMGK02/sys/main.dol`
- `config/RMGK01/symbols.txt`
- `config/RMGK01/splits.txt`

Retail unit boundaries are text `0x80030A74..0x80030BA0` and rodata `0x8052FA48..0x80530A58`. The retail object has only 22 HA/LO text relocations, all addressing its four tables; rodata has no pointer relocations or dynamic initialization.

## Full-build boundary

`configure.py` deliberately remains `Object(NonMatching, "Game/AudioLib/AudBgmSetting.cpp")` because project configuration was outside this lane. The focused source object proves exactness. The normal protected retail-link lane also remains SHA-exact, as recorded in `verification.log`.

## PC-port mirror

After exact verification, the root header/source were copied byte-for-byte to:

- `pc-port/src/Game/AudioLib/AudBgmSetting.hpp`
- `pc-port/src/Game/AudioLib/AudBgmSetting.cpp`

Both pairs are registered in `pc-port/tests/GameSourceMirrorTests.cpp`. No compatibility wrapper or port-only conditional was added to either Game file.

Mirror SHA-256 values:

```text
15ba6fea690862e08b1a02aea0bbfe34627d2601c88d6157cf6cce060319a19e  AudBgmSetting.hpp
1146fd19123dffac431bd0966464b6f9a300cb1733be3f117726b324767ed42c  AudBgmSetting.cpp
```
