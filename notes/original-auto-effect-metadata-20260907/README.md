# Native original auto-effect metadata — 2026-09-07

`smg-pc-original-auto-effect-metadata-tests` exercises the production restored
AutoEffectInfo, AutoEffectGroup, AutoEffectGroupHolder, and createAutoEffect
implementations. The fixture opens the actual disc named by `SMGPC_REAL_DISC`,
constructs ParticleResourceOwnership and its original ParticleResourceHolder,
and reads the raw AutoEffectList.bcsv from the retained Effect.arc as the oracle.

Every authored row passes through the original group construction graph. The
test compares string/null semantics, effective name, Follow/Affect/BCK flags,
offsets, frame intervals, scale/rate/light values, draw categories, and literal
native RGBA channels. Reinitialization verifies flags are cleared and set while
unrelated high bits retain their original values. Colors with no authored value
are marked invalid; reinitialization does not require their old payload to clear.

Group names are grouped independently with ASCII case folding, as authored game
identifiers use ASCII case. The test discovers both distinct spellings and
case-insensitive group counts from this disc. It checks original append order,
lookup identity, duplicate/missing queries, exact per-group allocation capacity,
and batches at the original holder's 256 slots. It exercises queries at capacity
without attempting the original unchecked 257th insertion.

Every distinct UniqueName is resolved using the newly recovered original
createAutoEffect implementation. The result is compared with the first authored
matching row, the unused first argument is varied, and simultaneous calls must
produce separate record objects. Metadata strings remain borrowed from the
process JMapInfo owner. The metadata allocation domain and production resource
owner retire before their original root-heap free-space balance is checked.

The prior staged probe reported 2591 rows and 614 exact group spellings. The
production resource owner counts case-insensitive groups, so those spellings
must not be mistaken for the holder's capacity or its unique group count. Fresh
run output records the current disc counts; they are not inserted as expected
values in this test.

Build and real-disc execution are coordinated by the parent task. This test
checks authored metadata and native ownership, not emitter simulation, particle
drawing, actor behavior, or complete gameplay.
