# Production status after parent integration

The frozen patches describe their original baselines. Do not apply the whole
aggregate patch blindly over newer production sources.

- Aurora's six-file patch is active in `8c90b03`, plus the explicit aurora-os
  dependency for aurora-gx. Parent checkpoint `379ce6a7f` contains the submodule
  update and independently passing normal GX/functional test targets.
- Color8's two numeric packing expressions are active in `4a1267d7e` with a
  normal target and ASan/UBSan checks. This portion of general-native.patch
  must be skipped when applying its still-staged scheduler-routing change.
- Scheduler trace, snapshot and scratch-list host ownership is active in
  `34751a7c5`, independently of these patches. The actual renderer effect fixture
  passed again after the complete scheduler fix.
- SceneFunction active-binding routing, the original effect cohort, and the
  factory/initialization integration remain staged. Actor/layout/MultiScene
  EffectKeeper ownership and legacy provider retirement still gate activation.
- The execution-charset proposal is separately staged. Its coordinated host/
  Game text boundaries and preprocessing limitations must be resolved before
  enabling it. Original Game literals and byte-prefix methods are unchanged.

Jumping and the full Gateway bunny sequence are not validated or completed by
these checkpoints. The manager proof covers original construction, entry,
resource/capture-texture identity, scheduling and teardown; drawing is untested.
