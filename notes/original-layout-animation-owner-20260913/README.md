# Original layout animation ownership

Recover the original LayoutManager animation lookup, pane binding/unbinding, and preorder pane metadata from the retail LayoutManager object. Binding an ancestor skips subtrees that have their own pane controller. Existing actual LayoutGroupCtrl/LayoutAnmPlayer can then use real NW4R transforms instead of reaching an unavailable-object exception.

Reference compile passed. First object comparison: getAnimTransform 99.74%; bind/unbind top-level 100%; bind/unbind subtree 99.48%/99.36%; initPaneInfo and recursive metadata 100%; countPanes 94.74%. No further matching campaign.

Native integration is complete and frozen for the shared production build. Ownership: the existing typed pane graph belongs to Nw4rLayoutRecords; an actual NW4R Layout owns its transform list, borrows the graph through SDK user-allocation flags, and unbinds before graph retirement. Converted BRLAN bytes and texture backing remain leased by actual AnimTransform instances. Root/pane animation frame-sidecar migration is outside this bounded change.


The recovered LayoutManager source/header are copied byte-identically to native Game. The compat provider now supplies the retained container, pointer arrays, and original controller registration; eight obsolete throws were removed. The existing reference LayoutPaneCtrl::recalcChildGlobalMtx body also replaces its compat throw. An actual NW4R Layout owns the full animation list and GroupContainer; user-allocated pane/group records remain singly owned by Nw4rLayoutRecords. Teardown unbinds animations, destroys the container/transforms, then retires group and pane records. Layout dimensions and origin come from the same authored BRLYT.

All LayoutHolder animation entries now create actual NW4R transforms in original resource-table order. Native BRLAN conversion leases remain attached to the transforms. Animation-only textures resolve from the same retained archive through the existing bounded TPL converter and transfer their backing lease into AnimTransformBasic. Missing/invalid resources remain errors.

Necessary verification only: original compiler pass and first object match above, plus successful isolated native compilation of LayoutManager.cpp, LayoutManagerCompat.cpp and Nw4rLayoutRecords.cpp. No component tests or shared build/run performed by this agent. Root owns integration execution.

Shared LayoutManagerCompat.cpp hunks owned here: LayoutHolder include; ManagerState pane_infos/animation_transforms ownership; bind_actor_manager actual layout/metadata/transform setup; renamed constructor fields; add/create pane-controller metadata publication; removed eight obsolete throw providers; original recursive child matrix body. Root's pointing-target and Peirce's effect/base-name changes are preserved and included in the file hash; they are not attributed to this animation recovery.

Remaining scope: the compatibility root/pane frame evaluators and follow-position renderer still exist; this batch does not claim complete LayoutPaneCtrl or LayoutRuntime removal. Actual group animation controls can now resolve and bind real SDK animation objects.
