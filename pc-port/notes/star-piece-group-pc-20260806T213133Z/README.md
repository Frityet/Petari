# StarPieceGroup PC frontier evidence

This frontier restores the original `StarPieceGroup` factory path for the
HeavensDoor scenario-1 placement without stage, zone, row, path, or route
special-casing. The original group source remains byte-identical to the root
decomp; host-only StarPiece behavior and SDK gaps live in compatibility code.

## Source identity

`artifacts/source-identity.tsv` records identical SHA-256 hashes for the five
root/PC source pairs imported by this frontier:

- `Game/MapObj/StarPieceGroup.cpp`
- `Game/MapObj/StarPieceGroup.hpp`
- `Game/MapObj/StarPiece.hpp`
- `Game/Util/TriangleFilter.hpp`
- `JSystem/J3DGraphBase/J3DStruct.hpp`

The original factory aliases `StarPieceGroup` and `StarPieceFlow` both create
`StarPieceGroup` and retain the original null archive callback. The group
therefore requests no synthetic `StarPieceGroup` archive; each child initializes
the real `StarPiece` model through the normal actor/model compatibility path.

## Disc placement

`artifacts/disc-placement-row.txt` was extracted read-only from `RMGK01.iso`
with the cloned Dolphin `dolphin-tool`, then decoded with
`smg-pc-bcsv-probe`. It confirms row 7 of
`HeavensDoorMiddleZone.arc/jmp/placement/common/objinfo` is
`StarPieceGroup`, has `Obj_arg0=10`, position `(0, 2120.580322, 0)`, unit
scale, no rail, and no switches. `Obj_arg1=-1` correctly preserves the
source constructor's default circle radius.

## Verification

- `xmake -j4 smg-pc`: passed.
- `xmake -j4 smg-pc-aurora-native-tests`: passed.
- `xmake run smg-pc-aurora-native-tests`: 22/22 passed, including factory
  aliases/archive behavior, native ten-child creation, TRS circle positions,
  kill/reset/revive, declaration tracking, and teardown.
- Real-disc gateway smoke: passed through the main title, file select, all five
  picturebook advances, and the HeavensDoor handoff at frame 10350.
- Placement summary moved from 167 created / 3 blocked to exactly 168 created /
  2 blocked, with 242 total and 72 intentionally ignored.
- The placement report and semantic trace identify exactly one created
  `StarPieceGroup` in `HeavensDoorMiddleZone`, source row 7, using
  `support_reason=original_factory` and an empty group archive.
- Runtime semantic events register ten child models and ten body sensors; the
  captured trace contains ten `StarPiece` render packets at its stage frame.
- Captured frame: `gateway/gateway_handoff/gateway_handoff-frame-10350.png`
  (`640x480`, nonblack ratio `0.98057`).
- Trace validation passed with 573 render packets and 1862 semantic events.

The full application log, placement report, trace SQLite database, screenshot,
validator log, save data, and manifests are retained under `gateway/`.
