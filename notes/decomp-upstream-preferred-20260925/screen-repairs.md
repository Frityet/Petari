# Screen merge repairs

The first full configured Wii source build failed because both the upstream message-tag header and the retained fork-only header declared MessageEditorMessageTag. Restore upstream MessageTagSkipTagProcessor.hpp and MessageUtil.cpp, including the upstream inline isGroupTagId ownership and signed getParamLength. Remove the now-unused fork-only MessageEditorMessageTag.hpp rather than retaining an alias.

Restore upstream BloomEffect.cpp: the automatic merge retained switch(drawType) but upstream renamed the parameter param5. All methods exist in upstream; the only other retained differences were redundant includes.

These repairs select existing upstream code rather than reconstructing gameplay behavior.

JSystem: restore upstream J2DOrthoGraph.cpp to remove the fork getGrafType out-of-line definition duplicated by the upstream inline header definition.

Second build exposed two remaining overlaps: WarpPod initPair/drawCylinder contained mixed local/upstream names; LayoutManager retained mSubtreeSize while the upstream header uses mChildCount. Both files now exactly follow upstream, which contains all of their methods.

All configured sources then compiled; link exposed RunawayRabbitCollect::__vt missing because the fork-only explicit destructor declaration survived while upstream removed its definition and uses the implicit destructor. Restore the upstream header so MWCC emits the canonical vtable/destructor.

The following link likewise identified the stale explicit RunawayRabbit destructor declaration. Restore its upstream header for the same ownership reason. Other retained explicit destructors have source bodies and were not changed.
