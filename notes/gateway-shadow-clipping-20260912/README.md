# Original shadow-aware clipping — 2026-09-12

The old native `setClippingRangeIncludeShadow` unconditionally threw even after
the port acquired actual ShadowController lists and projection state. Restored
the original projection queries, controller lookups and clipping functions from
ActorShadowUtil and ActorShadowLocalUtil in a shared provider. All eleven bodies
are byte-identical to the current decomp reference; `source-proof.json` records
the comparison. The original missing local-utility declaration header was copied
unchanged. No existing Game implementation was edited.

The clipping sphere uses the original midpoint and radius covering actor and
projected shadow, and retains the caller-owned center pointer. When projection
is absent it uses the original actor-centered sphere. It reads actual original
controller state, including pointer-backed projection positions, rather than
cached shadow definitions or a stage/actor-specific result.

The existing original shadow owner test now checks unprojected/projected state,
midpoint/radius, pointer-backed projection changes and clearing the borrowed
center when projection ends. Its old standalone holder setup was migrated to
SceneExecutionFixture after the first run exposed its missing original executor
owner. Build/run evidence is recorded in the parent integration note.

This removes one shared Coin/Chip/StarPiece-related prerequisite. Mirror actor
creation, item result handling and full original GameScene activation remain
separate boundaries. No unsupported actor placement was enabled, and this is
not a claim that the Gateway bunny chase or Rosalina is working.

Coordinator final integration: the affected focused tests build and pass; see ../gateway-integration-20260912/validation-summary.json for selected final receipts and explicit remaining limits.
