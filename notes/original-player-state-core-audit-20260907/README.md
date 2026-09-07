# Original Mario state core review, 2026-09-07

This is a separate correction checkpoint after the seven-state reference recovery
`7c1c6b055`. The earlier constructor/vtable/layout proof remains intact. Changes
are confined to decomp `MarioHang.cpp` and `MarioTeresa.cpp`; native source and
activation remain with the parent owner-graph lane.

## Demonstrated behavior corrections

`MarioHang::update` at `0x802F9548`:

- Retail tests movement-word-0 bit29 (`_1D`) after `tryClimb`, then returns before
  stick/drop processing when set. Historical source tested bit2 (`_2`). The
  corrected source selects the original flag.
- In state0, retail sets the drop flag when ungrounded and still calls
  `calcDistToCeilHead`. Historical `!grounded || query` skipped that call. The
  query writes the real `_460` collision Triangle through its line intersection,
  so retaining it preserves collision state as well as the returned distance.
- Ordered matrix branches, separate exit guards, the last-move vector snapshot,
  and the original state/direction switch structure are restored from the
  instruction listing. The function improves69.73→98.18%; actual sizes are
  retail1,888/candidate1,884 bytes.

`MarioTeresa::addTeresaVerticalVelocity` at `0x8030EC98`:

- Retail computes `sin(pi * disappearanceRatio)` through the first JMath table
  component with signed sine handling. Historical source used cosine, changing
  the falling-speed bound. At ratio0 the original factor is1, while the cosine
  version produces1.5. The constants and table selection were inspected directly
  in the retail object; changing to sine improves85.87→97.84%.

`checkWind` and `checkWallCeilReflect` at `0x8030F48C` and `0x8030F604`:

- Retail accumulates `(wind * 0.1f) * 1.5f`, preserving two rounded vector scales.
  Historical `wind * 0.15f` can produce a different binary32 result.
- Retail positive wall reflection computes `(normal * speed) * 1.5f`.
  Historical `normal * (1.5f * speed)` changes the rounding order.
- `float-order-examples.json` gives concrete one-ULP differences for these
  expressions. These examples illustrate arithmetic, not executed native Mario
  gameplay.

## Original call and ownership semantics

`startTeresaMode` and `getHitWallNorm` now use the actual `getPlayer()` owner
accesses and retail per-wall branch structure; both match100%. The prior wall
method also added a null-triangle shortcut not present in retail. The original
collision flags imply valid corresponding Triangle ownership.

Ground/wall reflection now projects `_34` in place, and near-ground control
performs the repeated original shadow-normal queries. The corresponding body
scores improve, without an alternate state owner or a host-specific condition.

`procControl` restores the original velocity copy, stick query, discarded dot
call, and two sequential direction scales. Retail does **not** assign that dot
return to the direction multiplier; the recovered source deliberately preserves
that fact. The result improves76.39→98.64% at544/544 bytes.

`procDrop` restores the retail last-move/gravity query, height computation,
in-place velocity projection, and final projection/clamp call. The clamp result
is explicitly unused in retail; no guessed braking field or output was added.
It improves41.36→87.62% at692/668 bytes. Residual branch/register differences
remain; this score is not claimed as full behavioral equivalence.

## Evidence and limits

`baseline/` freezes both source files at the named reference checkpoint.
`refresh-proof.py` freshly compiles both baseline and corrected complete Wii TUs
with the configured compiler, compares them to the newly split verified RMGK01
retail objects from the previous checkpoint, and records both object sizes.
`function-proof.json` lists changed scores and any regressions;
`validation-commands.json` records exact commands/results. Retail disassemblies
and the initial compiled disassemblies are retained for review.

The independent MarioActor animation-method review is tracked separately and
will be appended after its corrections and fresh object proof are ready.
Other lower-score bodies remain subject to further instruction review. This
checkpoint does not establish native state activation, player movement, camera
behavior, or a playable Gateway demonstration.
