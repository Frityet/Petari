# Canonical owners: execution, layouts, input and pointer

Removed 15 files from `src/compat/`, reducing the working inventory from 91 to 76. Also deleted the alternate LayoutManager/LayoutHost graph and restored the original Game layout actors, pane/group controllers, NameObj execution holder, SceneFunction category dispatch, and StarPointerDirector/StarPointerUtil. WPad now resolves its actual GameSystem owner. Unused J3dSystemCompat disappeared without replacement.

Required native ownership and architecture fixes live with actual Game/NW4R owners. The remaining SceneScheduler preserves native resource retention and permanent retirement bookkeeping; movement/calc execution uses the actual category lists. Independent layout previews remain separate from Game managers. No new decompilation was needed: the restored algorithms are existing donor sources. See `nameobj/implementation.md`, `layout/README.md`, `wpad/README.md` and `scene/README.md` for exact retained boundaries.

## Validation scope

Following the requested faster workflow, validation is one application build and a fresh-save 120-frame Gateway opening smoke, with retries only for observed compile/startup failures. No focused test suites or broader gameplay campaign. Existing tests were adapted or retired where they depended on removed APIs; `layout-tests/README.md` records a remaining wipe-fixture lifecycle migration. A successful opening smoke does not establish rabbit catches or Rosalina spawning.

The first build exposed missing donor SDK VEC3TransformNormal/DrawInfo members and one stale LayoutHost scheduler call; those were repaired in their actual owners. The first smoke exited with SIGSEGV before completing frames. LLDB identified ordinary LogoScene retirement: GameSystemSceneController unpublishes mScene before deletion, then native scheduler retirement tried to resolve the executor through that cleared publication. Native permanent retirement now takes the retained executor explicitly; ordinary Game calls keep the donor lookup. The controller publication order is preserved.

Final integrated result is recorded in `validation.json` after retry. `gateway-120.json` and `startup-backtrace2.log` preserve the original failure rather than replacing it.

## Scope and publication

Per-lane manifests and before snapshots support isolated staging. Initially dirty RuntimeContext and test build changes are staged only by this batch's delta. HEAD-only legacy methods lose obsolete removed-API references without adopting unrelated working deletions. The six preexisting staged route-note changes are preserved byte-for-byte. Notes exclude local NAND saves and snapshot trees from publication.
