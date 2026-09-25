# Layout API test cleanup

Retired unbound LayoutHost service exception contracts and synthetic preview-to-Game manager attachment tests; canonical LayoutManager now loads an actual resource graph, so those old construction contracts no longer exist. Kept standalone preview/resource checks, original tag/matrix/group cases and actual-process FlyMeter checks. Existing wipe checks now inspect actual manager resources and animation transforms rather than parallel LayoutHost telemetry. Removed sidecar-count assertions.

No tests built or run. Wipe tests still use their preexisting standalone SceneExecutionFixture; migration to actual process lifecycle remains outstanding following restoration of original GameSystem executor lookup. The build/run evidence for this batch is the app and short actual-process opening smoke only.
