# Remove parallel stage-session and scenario metadata owners

Baseline: e02746442. Deleted compat/StageSessionState.cpp/.hpp and StageScenarioMetadataResolver.cpp/.hpp. Before deletion, all production references were internal to those four files: no other production code called the session binding, metadata resolver, or its begin/end audio helpers. A complete post-edit src cpp/hpp search for all retired type/function names returns no matches. The temporary-data allocation, thread-local scene identity, mutable metadata override, and parallel parser have no replacement provider.

RestartStageSessionTests is replaced by OriginalStageSessionTests. One original Gateway process checks actual scene-controller selection, sequence-owned restart identity/mutation/reset, separate scene-entry/default IDs, authored zone names, Gateway empty Comet/FileSelect missing Comet, and actual Purple rows through GalaxyStatusAccessor. It restores temporary restart state and checks owner retirement. It does not request or claim a full gameplay restart. Synthetic unresolved/Purple metadata replacement and standalone nested binding contracts are retired.

Independent assertions are retained in existing fixtures: JAudioPlaybackTests contains JAISoundID and AudioEventService identity/reset checks; PlayerActorBridgeTests contains explicit nerve-change capability checks. The unused session include is removed from the Shadow owner test. Other consumers belong to root/other lanes.

Nine paths were clean before edits; snapshots and scoped patch are included. Root owns wiring, builds, staging and commits. Source search and whitespace checks passed. No build/runtime was run; latest user steering requests fewer tests and root will validate the integrated app.
