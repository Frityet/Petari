# Exact File Select support-source sync

## Outcome

The PC `Game/` copies for the five support actors used directly by the exact
`FileSelector` are now byte-identical to their decomp sources:

- `BackButton`
- `BrosButton`
- `InformationMessage`
- `MiiConfirmIcon`
- `SysInfoWindow`

`FileSelectInfo` is also byte-identical on both sides after correcting its
constructor. `mNameBufferSize` is an element count, as its later `copyMemory`
use requires; the allocation is still zeroed using the actual host byte count.
The change takes the RMGK02 `FileSelectInfo` object from a 99.452194% fuzzy
match / 94.038246% matched code to 100% fuzzy, code, and function match.

No factory, route, or runtime activation was added. No compatibility fallback
was introduced. `SaveIcon` and `TriggerChecker` were deliberately excluded
because they are protected, independently dirty user work.

## Mechanical boundaries

`GameSourceMirrorTests.cpp` now checks the eleven newly frozen paths in this
slice. Together with its pre-existing `FileSelectInfo.hpp` check, that is twelve
File Select support mirror pairs. `FileSelectInfo.cpp` was also added to the
existing exact File Select host-compile target so that byte identity cannot hide
a host syntax regression.

See `frozen-paths.txt` for the exact commit review/staging scope and
`verification.log` for the focused proof.
