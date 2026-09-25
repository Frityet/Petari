# Original Gateway route evidence

The fresh-save original GameSystem completed **18,000 frames**, exited **0**, and was reaped normally after 327.151 seconds. All three catches and their following Tico conversations completed; Rosalina was first sampled alive and unhidden at **frame 7100**. These are observations from the running game, driven only by ordinary controller input.

The executable SHA256 was `c08e1e4cd27cfa9d059ed3b627bcbc0be301d73f95ab3912ea823fab1488ad55` before and after the run. This is the executable **before** all donor restorations in this batch: both `XanimePlayer` conditions, `DemoExecutor` clipping, `TalkBalloon` vector behavior, `RunawayTico::init` color initialization and Rosetta's spin-demo1 sound predicate. Their separate post-change 120-frame smoke must not be presented as a repeat of this full route.

## First sampled milestones

| Group | Captured phase | Rabbit dialogue ended / Toss begins | Rabbit Stop | Tico talk begins | Following Tico dialogue complete |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bush, rabbit1090 / Tico1075 | 3170 | 3300 | 3370 | 3430 | 3540 |
| Pipe, rabbit1085 / Tico1069 | 4360 | 4490 | 4560 | 4620 | 4730 |
| Hole, rabbit1095 / Tico1081 | 6140 | 6270 | 6340 | 6390 | 6630 |

Trace sampling is every ten frames: these are first observations, not exact internal event frames. For the hole rabbit, the first sampled captured state is already CaughtTalk; its brief preceding Caught state was not sampled. The final Tico enters WhiteOut at 6630 and WhiteIn at 6720.

- **6800:** tower spherical-range plane gravity 459 first appears alive; priority 1, range 2300. At 6790 it is dead/unappeared, although its separate `activated` flag is already true. A different box-range plane 460 is active at scene initialization and is not this tower event.
- **7100:** Rosetta 1060 is alive, unhidden and unclipped, with animation/view calculation enabled. Operator records `rosetta_alive_not_hidden` and ends its three-rabbit route.
- **13620:** Mario first selects plane 459/priority 1, with actual on-ground bit `_1` set (`movement_low_word & 0x40000000`), no bound-actor override, and ground triangle host 741. Those flags also hold at 16000, 17700 and the last sample 17990.

The filtered trace does not independently name host 741. Its sampled prism vertices match the archived original `HeavensDoorAppearStepAAfter` collision geometry transformed by the authored zone matrix within 0.41 world units, supporting the stairs identification; the supervisor's frame 16000 image inspection independently places Mario on stairs. Grounding is based on the actual movement flag plus triangle, not a cached triangle alone. Exact vectors, flags, residuals and the geometry-source hash are retained in `route-evidence.json`.

## Approach and visual limit

The separate approach controller stopped at **17700**, **1130.205 units** from Rosalina, and published neutral input. It did not complete the ascent or reach her subsequent 500-unit spin-get trigger. Mario's stop position was `(14916.277344, -9164.566406, 8240.895508)`; the last game sample 17990 is `(14916.831055, -9164.089844, 8241.563477)`, distance 1130.324.

The root supervisor inspected `route-frame16000.png`: Mario and a Luma are on the stairs, with **part of Rosalina's torso visible at the top behind the parapet**. This is not an unobstructed Rosalina view or visual-parity proof. No spin-get or Grand Star completion is claimed.

## Compact raw evidence

`route-evidence.json` records lifecycle, binary, source artifact hashes and detailed milestones. `milestone-snapshots.jsonl.gz` contains **27 exact, unmodified original trace lines**, including catch/dialogue transitions, plane appearance and its preceding sample, Rosalina appearance and its preceding sample, gravity selection, frame 16000, approach stop 17700, and last sample 17990. Requested frames all exist in the source trace. Gzip uses mtime 0.

Full source trace SHA256 (verified against `artifact-manifest.json`): `4e0b38cbc0c0d0bdf31fe3fe21ccf1b0d77183486476ec585049f93d330836d1`. Compact snapshot SHA256 and compressed size/hash are recorded in `route-evidence.json`; the full 113,610,139-byte trace need not be committed to preserve these selected observations.

This task only read logs/source declarations and wrote evidence summaries/snapshots. It did not run a build, test, game or controller.
