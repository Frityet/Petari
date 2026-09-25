# Gateway ascent and spin: completed route evidence

The actual original GameSystem completed **36,000 frames**, exited **0**, and was reaped normally after **630.148 seconds**. `route.json` confirms no timeout, bounded completion and the same binary SHA-256 before/after: `037dbc1201722dccc734c34e3e39eb1ff750fedde5f1599e0ea54be9854687b8`. Launch source checkpoint: `412329362327ef61402a1ebf03e7a4cb3427f038`. This run does not validate source changes made after that binary was built.

The run demonstrates all three original rabbit catch groups, the authored tower/Rosalina appearance, controller-only ascent to the tower top, a clear rendered view of Rosalina during the tutorial, subsequent spin use breaking crystals, and first launch-star appearance. **No launch ride, first Grand Star pickup or galaxy completion was observed within the frame budget.**

| Milestone | First sampled frame |
| --- | ---: |
| Bush rabbit1090 caught / Tico conversation complete | 3300 / 3670 |
| Pipe rabbit1085 caught / Tico conversation complete | 4850 / 5220 |
| Hole rabbit1095 caught / final Tico conversation complete | 9450 / 9940 |
| Tower crystal1013 appears | 10110 |
| Rosalina1060 alive and unhidden | 10410 |
| Stair operator starts / reaches waypoint27 | 10970 / 15580 |
| Tutorial: Rosetta repositioned, Tico903 appears | 15700 |
| Rosetta and Tico903 retire, Mario normal state | 18710 |
| Scripted A triggers in crystal prompt / MarioTalk ends | 29460,29590 / 29600 |
| MarioMagic after ordinary X / crystal1013 breaks | 33190 / 33200 |
| Crystal1013 dead | 33250 |
| Crystals1016 and1020 enter Break | 35950 |
| SuperSpinDriver864 appears / enters Wait | 35960 / 35980 |
| Last actor sample / completed frames | 35990 / 36000 |

The original spin-permission member is **not directly traced**. Initially its grant was unproven; later the parent's ordinary physical X input, the actual MarioMagic transition, and cage break demonstrate working spin behavior. `crystal-x-ui.jpg` captures Mario and shards from the first crystal break. Earlier X during dialogue was not a valid spin check, and the unclear result of an earlier physical Return press does not establish broken keyboard input. The two later scripted A presses have actual trace triggers and are followed by MarioTalk ending.

`spin-tutorial-ui.jpg` visibly shows Rosalina and Mario with the original dialogue, resolving the previous unobstructed-Rosalina evidence gap. `after-tutorial-ui.jpg` records the later normal scene. The late launch-star reveal was seen by the parent, but its temporary capture disappeared before saving; there is **no** launch-star-revealed image artifact to cite. Retained trace evidence shows driver864 Appear→Wait. It had only just entered Wait before the run ended, so failure to ride it within this budget is not evidence of a driver failure.

`milestone-snapshots.jsonl.gz` contains **28 exact selected JSONL records**, preserving all fields/actors at those frames. It is not a compressed copy of the full trace. `route-evidence.json` lists the selected frames, concise actor summaries, terminal data and per-artifact SHA-256 hashes. Full source-trace hash (taken after terminal report): `4b7d5ef3c84b6425c41f4f956e42ea5420ea6bbbf2a23cf3520d772105b81da0`; final complete source record is frame35990. Sampling every10 frames means listed transition times are first observed samples, not exact sub-frame trigger timestamps.

The evidence uses ordinary scripted controller buttons/stick plus parent-operated physical keyboard input. This summarization task only read logs/images and wrote these compact evidence artifacts; it performed no controller actions, memory/position/nerve writes, production edits, builds or tests. Audio, retail visual parity, exhaustive physics behavior and later galaxy progression are outside this claim.
