# FileSelector / Mii activation

The seven excluded sources already contain their complete existing implementations. MiiFaceParts and MiiFaceRecipe source/header pairs are identical to current decomp; MiiFacePartsHolder constructor and all methods are present, with existing CP932 and native spelling adaptations. Their reported missing constructor and icon-method symbols are build exclusions, not missing Game implementations. Existing FileSelector/FileSelectItem native adaptations were preserved instead of being overwritten with older differing donor math.

Root enables `Map/FileSelectEffect.cpp`, `Map/FileSelectItem.cpp`, `Map/FileSelector.cpp`, `Screen/FileSelectInfo.cpp`, `NPC/MiiFacePartsHolder.cpp`, `NPC/MiiFaceParts.cpp`, and `NPC/MiiFaceRecipe.cpp` in Game xmake. No build wiring was edited by this lane.

Actual edits:
- Restored exact donor inline `WipeFade::setColor` in its existing header.
- Added a native-width FileSelectFunc `copyMiiName(wchar_t*, ...)` overload. It reads the original RFL UTF-16-unit API and widens each unit into a native text slot, matching the project's native BMG representation.
- FileSelector demo-name and Mii confirmation paths now use that native-width overload instead of reinterpreting 16-bit arrays as native wchar_t buffers.
- FileSelectInfo widens the original u16 input per unit rather than memcpying half-width units into its wchar_t name buffer.

Snapshots preserve all earlier native and round17 dirty edits. No builds, tests, or additional test cases were run/added. Source whitespace checks passed. Parent owns compilation and short integrated smoke.
