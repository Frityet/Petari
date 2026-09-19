# Util and library reconciliation

The merge brings upstream `7663bd3be278c7dfaaccd9df729b3b9cf61acf27` into local `pcp-decomp` at `3c8ca69ba1d0c8367f983e30ce6182799a9694a3`. Native source remains separately selected; this is the Wii reference merge.

- FurCtrl/Drawer/Multi/Param/Shader and J3DModel adopt the complete incoming recovery together. FurBank moves to FurMulti, J3DModel2 becomes private to FurCtrl, and J3DVtxShader replaces the old unknown type. All local method names have incoming definitions; the former J3DModel2 destructor is inline.
- AreaObjUtil, DirectDraw, MapPartsUtil, JointController, SceneUtil, StringUtil, TalkUtil and LayoutUtil adopt incoming complete implementations. Incoming LayoutUtil moves the recovered TextBoxRecursive operation methods into its header. SceneUtil follows the typed, const StageDataHolder placement matrix.
- ObjUtil retains five recovered bodies where upstream still provides only declarations: getCsvDataBool, getCsvDataVec, getCsvDataColor, findNamePosOnGround and the matrix overload of tryFindLinkNamePos. The locally recovered void findNamePosOnGround signature remains; the incoming bool forward declaration is removed. Incoming completed rumble/creator code remains.
- MapUtil adopts incoming original ground and query functions while retaining the local isSoundCodeSand helper.
- LiveActorUtil retains the local scope needed around the isBindRoof goto target, with other incoming changes auto-merged.
- JointController.hpp contains both index and name factory overloads. BothDirPtrList contains the incoming default constructor and the recovered bool constructor. Functor retains its const/inline overloads and zero alignment; the const one-argument overload return type is corrected to match its returned member-pointer type.
- Array retains its generic member callback overload. Duplicate Vector::insert definitions were removed. TPartition3 adopts incoming complete out-of-class definitions after identifying duplicates against local inline bodies. The two implementations perform the same cross product and plane calculation.
- NW4R uses incoming assertion macros and the complete incoming LinkList, CharWriter and PrintContext definitions. Incoming CharWriter already provides all locally added accessors. An auto-merged duplicate Rect::SetHeight was removed. MSL functional keeps incoming additions plus the auto-merged local const member-reference functions.

`resolve-root.py` records the first explicit resolution pass; it is historical and must not be rerun over the final audited tree. Compiler checks and review subsequently remove auto-merged duplicates and repair contracts. `initial-root` records the initial compilation failures, including conflicts in not-yet-frozen dependent headers. Final compile receipts are recorded separately.


## Final compiler reconciliation

Broad compilation found shared-header changes in local units outside the upstream diff. Removed redundant Player INIT_NERVE instance blocks now supplied by NEW_NERVE, expanded the four empty nerve declarations to ordinary Nerve classes, and updated TalkTextFormer byte fields to their upstream names at offsets 0x31/0x32. Explicit declarations were retained for recovered out-of-line destructors/assignment members where MWCC otherwise inferred conflicting definitions. Auto-merged duplicates in ShadowVolumeDrawer, Pane and NPCUtil were reconciled against both parents. NPCUtil's incoming complete methods replace the earlier equivalent private reaction helpers; no native NPCUtil import was performed. The final validation records successful current-source hashes for all configured Game units and changed library units.
