# FileSelector Mii font compatibility bridge

## Outcome

The host now provides a real `nw4r::ut::Font` / `nw4r::ut::ResFont` compatibility surface for the exact retail
`FileSelector::createMiiFont` call shape. `ResFont::SetResource(void*)` validates and parses the installed BRFNT through
the existing `BrfntFont` parser, exposes retail metrics and encoded glyph-sheet pointers, and preserves live
`SetResource` / `RemoveResource` state. Unsupported versions, truncated resources, malformed resources, missing glyph
output storage, and absent resources fail explicitly.

`MR::setTextBoxFontRecursive` is implemented in `src/compat` and routes into `LayoutRuntime`. Matching BRLYT text boxes
hold a weak live binding to the external font state; rendering uses the parsed external BRFNT in the ordinary layout
text rasterizer. Removing or destroying the font makes the binding absent instead of retaining a copied or fallback
font. Reinstalling a valid resource on the same `ResFont` reactivates the binding.

No FileSelector factory path or exact FileSelector source was enabled by this work. The only `src/Game` edit is the
missing retail-compatible declaration in `Game/Util/LayoutUtil.hpp`. The protected SaveIcon and TriggerChecker files
were not touched.

## Retail resources used

- `/workspaces/pcport/orig/RMGK02/files/LayoutData/MiiFont.arc`
  - SHA-256: `328c06b966ca40a36635a5abacc1f2dc87db89171fd289cc1c13ebf38ffe3e8e`
- `/workspaces/pcport/orig/RMGK02/files/LayoutData/FileInfo.arc`
  - SHA-256: `c3e77384e65d1f0d2aa50513869ea9f91a12d0bb17452a0ed01e9e18d855991a`
- extracted `MiiFont26.brfnt`
  - SHA-256: `44ac37eac8fcebe885ee3c94797af243242e374936f30cd6510b8e61a87d3355`
  - decompressed size: `3,654,044` bytes
  - RFNT version: `1.4`

The exact parsed retail metrics asserted by the test are 27x33 font/cell size, ascent/baseline 26, descent 7,
line-feed 33, I4 sheets at 256x512, UTF-16 encoding, and default widths 0/27/27.

The test independently reparses `blyt/fileinfo.brlyt` and walks pane parents. The retail `FileName` pane has exactly
two descendant text boxes, `ShaName` and `TxtName`; both receive the external Mii font. U+FFFF and literal `?` produce
identical dimensions, nontransparent-pixel counts, and full RGBA FNV-1a hashes for both rasterized descendants after
`SetAlternateChar('?')`.

## Verification

See `verification.log` for the commands and captured outcomes. The principal debug target and real-disc execution pass
2/2, the resource-only release facet passes 2/2 under ASan/UBSan, and the exact untouched FileSelector translation
unit compiles against the new headers/providers with Clang. GCC's only syntax-audit blocker remains its pre-existing
rejection of the decomp-style suffix placement of `NO_INLINE` at FileSelector.cpp:63; Clang accepts it with a
GCC-compatibility warning.

## Owned source files

- `src/nw4r/ut/Font.h` (new)
- `src/nw4r/ut/ResFont.h` (new)
- `src/JSystem/JKernel/JKRMemArchive.hpp` (new include wrapper)
- `src/compat/Nw4rFontCompat.cpp` (new)
- `src/compat/LayoutFontCompat.cpp` (new)
- `src/layout/LayoutRuntime.hpp`
- `src/layout/LayoutRuntime.cpp`
- `src/Game/Util/LayoutUtil.hpp` (declaration only)
- `tests/MiiFontCompatTests.cpp` (new)
- `tests/xmake.lua` (only the `smg-pc-mii-font-compat-tests` target block; shared file also contains another agent's target)
