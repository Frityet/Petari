# RailCoin full-route runtime checkpoint

Captured 2026-08-06 UTC from the Korean disc through Aurora. This checkpoint exercises the normal user-facing sequence rather than a direct-stage boot:

```text
title -> file select -> prologue picturebook -> HeavensDoorGalaxy
```

The existing title/file-select input script was extended with five separated A presses for the original picturebook stops at animation frames 350, 700, 1050, 1400, and 1748. The picturebook completed and the story executor requested `HeavensDoorGalaxy` at application frame 9724. No stage placement, actor coordinate, or rail behavior was bypassed.

## Result

- Process exit: 0 after the requested frame-10350 screenshot.
- SQLite trace validator: passed.
- Trace records: 2,556 total; 553 render packets; 1,991 semantic events.
- Screenshot: 640x480; nonblack ratio 0.980612.
- Placement report: 242 total; 164 created; 72 intentionally ignored; 6 blocked.
- Improvement over the preceding placement checkpoint: both former `RailCoin` blockers are now original-factory actors, reducing blocked objects from 8 to 6.
- The frame-10350 trace contains 24 `Coin` model packets, confirming that the two rail groups produced their member actors rather than only constructing empty group hosts.

Both placements retained their independent per-zone CommonPath data:

| Zone | placement row | CommonPath ID | path row | points | first point |
| --- | ---: | ---: | ---: | ---: | --- |
| `HeavensDoorMiddleZone` | 10 | 0 | 0 | 4 | `[2866.89, 6079.10, -8913.95]` |
| `HeavensDoorInsideZone` | 19 | 1 | 1 | 4 | `[42824.36, -14782.88, -3654.88]` |

The remaining six unsupported placements are three `DemoRabbit` actors, one `StarPieceGroup`, one `FlipPanelObserver`, and one `YellowChipGroup`. Those are recorded as the next generalized actor/service work; they were not replaced by model or route-specific stand-ins.

## Validation command

```text
smg-pc-trace-validate-sqlite \
  --require-emulator pc-port \
  --require-frame 10350 \
  --require-record-type frame,render_packet,semantic_event \
  --require-semantic-events \
  --min-render-packets 1 \
  frame-10350.trace.sqlite

trace  status  trace_id  frame_index  emulator  record_count  render_packet  semantic_event  layout_runtime
...    passed  1         10350        pc-port   2556          553            1991            0
```

The full 28 MiB SQLite trace was deliberately not checked in. Its content hash is retained below, while the screenshot, complete placement report, and compact summaries are preserved here.

## Root and PC-source integrity

- RMGK02 default `ninja`: passed.
- Rebuilt DOL and original DOL SHA-1: `54b71431af0d509097bfdef4ec28617afc487e89`.
- Coin/Rail/PartsModel PC Game sources are byte-exact to the regular decomp except for the Linux include-case correction in `RailCoin.cpp` and final-newline-only differences in four files. Importing the declaration-only `NameObjExecuteHolder.hpp` also lets `PartsModel.cpp` remain byte-exact.
- Host-only rail, fixed-position, matrix, area, binder, shadow, gravity, scene-object, and event behavior remains in shared compatibility/providers; no HeavensDoor IDs or coordinates were introduced there.
- The conservative whole-tree audit currently classifies 54 Game files as byte-exact and 8 as compile-only. Its full TSV inventory and the still-required migration list are preserved in `source-closeness-audit/`; this checkpoint's newly imported rail/Coin files dominate the exact/compile-only additions, while older PC infrastructure remains explicitly classified `compat-temporary`.

## Artifact hashes

```text
f9f006ab8b62696ccd4c7745a341b78a4faed8730c0dd7035a3ab93278481360  frame-10350.png
68cc4b2f612ac4944f9bc956d8a2c69e29f103098bb4087ab76f91c12da134b5  placement.md
f99106a3fd89edc1073b1c2ba82e017aa63cd6f437dc9c034f55ea0dd43f22e9  frame-10350.trace.sqlite (not checked in)
```

The screenshot shows the current host camera/model limitations clearly: the stage and particle field render, while the default scene camera and several generic fallback models remain visually incorrect. This is runtime proof of route/actor/rail integration, not a claim of final visual parity.
