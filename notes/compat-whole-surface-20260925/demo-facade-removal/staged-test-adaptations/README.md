# Minimal HEAD-base Demo test migration

These four saved C++ files are derived from committed versions at `ec94f1d5d996729c4e166b395cb5b90fcedc5336`. They contain only the minimal migration needed to remove the deleted Demo facades. They are intended as replacement Git index blobs for their corresponding `tests/<basename>` paths.

The actual working files in `tests/` were never written. Their preexisting fixture rewrites remain byte-for-byte preserved and uncommitted. `validation.json` records matching before/after SHA-256 values. No Git index mutation, staging, build, or runtime test was performed by this preparation.

`demo-head-base-migration.patch` is the source diff against the stated commit. It deliberately contains removed facade names in deletion lines; none of the four saved C++ adaptations contains a retired facade reference.

Changes:

- ActorEventCameraTests: use the existing bound scene to create the actual DemoDirector; remove the now-unused DVD helper parameter.
- CenterScreenBlurRealOrAbsentTests: create DemoDirector after holder binding; query simple casts through the actual DemoDirectorOwnership diagnostic.
- GravityRealOrAbsentTests: create DemoDirector after each holder binding, including both same-holder generations; remove only facade-specific fixture members.
- NameObjFactoryPlacementTests: create DemoDirector through the existing execution fixture's holder.

The preexisting standalone fixture limitations are outside this minimal commit adaptation. Real DemoDirector construction still requires the original resource/lifecycle prerequisites, including a GameResourceRuntime and DemoSheet archive. These files have source-level checks only; this is not a claim that the older HEAD fixtures pass runtime tests.
