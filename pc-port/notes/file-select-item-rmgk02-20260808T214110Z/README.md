# FileSelectItem RMGK02 reconstruction evidence

Date: 2026-08-08 UTC
Branch: `pcp-aurora`
Starting commit: `466916e2b`
Objdiff unit: `main/Game/Map/FileSelectItem`

## Scope

The reconstruction itself changed this root decompilation slice:

- `include/Game/Map/FIleSelectItem.hpp` (the repository's existing historical casing)
- `src/Game/Map/FileSelectItem.cpp`

The verified files are now also mirrored byte-for-byte as
`pc-port/src/Game/Map/FileSelectItem.{hpp,cpp}` (the PC header keeps its
correctly cased include path). The PC translation unit remains explicitly
xmake-excluded and unregistered until the real dependencies in
`pc-blockers.md` exist. Mirroring preserves source parity; it does not claim a
runtime closure or add a fallback.

## Result

The translation unit now contains every RMGK02 gameplay symbol. Objdiff reports zero unmatched target symbols and a final primary `.text` score of **99.80353%**, up from **76.852104%**.

Recovered or completed behavior includes:

- point-versus-cylinder collision used by pointer rotation;
- per-frame base matrices, planet offset, model/Mii scaling, blink update, and file-number screen projection;
- retail turn-to-front squared easing and its early return;
- pointer-drag rotation using the retail `0.03f` gain and `[-25, 25]` clamp;
- pointed/unused random ME selection;
- fellow/Mii/planet visibility transitions;
- Open, Vanish, Copy, and Complete effect emission/deletion;
- retail sound identifiers;
- inline empty/constant nerve bodies, matching the absence of standalone retail functions.

All reconstructed visibility and effect functions are 100% matched. `control` is 99.624%, `updateRotate` is 99.40821% with the exact 1656-byte target size, and the collision helper is 99.90909%.

The target and rebuilt `.data` sections are both 1016 bytes and their raw bytes are identical. Objdiff's 36.68639% `.data` metric is solely its relocation-target pairing score for local/vtable symbols, not a raw-data mismatch.

## Artifact identity

```text
83ad9cab285f84baf80688f652a5d9b65ef82abc  build/RMGK02/asm/Game/Map/FileSelectItem.s
e413adf95eda643ebdc0ee53845780a7ffd97f42  build/RMGK02/src/Game/Map/FileSelectItem.o
54b71431af0d509097bfdef4ec28617afc487e89  build/RMGK02/main.dol
```

The complete target disassembly remains at `build/RMGK02/asm/Game/Map/FileSelectItem.s` (3459 lines). Critical excerpts are preserved in `target-disassembly-excerpts.s`.

## Verification summary

```text
ninja build/RMGK02/src/Game/Map/FileSelectItem.o  PASS
objdiff-cli main/Game/Map/FileSelectItem           PASS (99.80353% .text)
ninja                                               PASS
ninja build/RMGK02/ok                               PASS
sha1sum -c config/RMGK02/build.sha1                 PASS
git diff --check                                    PASS
```

The RMGK02 DOL remains byte-identical because this translation unit is not yet selected as a linked matching unit in the project configuration.
