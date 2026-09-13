# Reached PauseMenu pane-layer ownership failure

PauseMenu::init initializes a whole-layout controller with one layer, then creates named pane controllers with their own counts; Stars explicitly gets two. Later MR::startPaneAnim(this, "Stars", "Star", 1) and setPaneAnimFrameAndStop target that pane's second local controller. This is original authored behavior.

Canonical LayoutPaneCtrl.cpp allocates mAnmPlayerArray(animLayerNum) and a LayoutAnmPlayer per slot. start, stop, isAnimStopped and getFrameCtrl index only this local array. Canonical LayoutUtil.cpp routes named-pane calls through getPaneCtrl(pPaneName), while whole-layout calls use getPaneCtrl(nullptr). Frame writes and queries directly access the selected player's J3DFrameCtrl. LayoutAnmPlayer::start sets BRLAN duration/loop mode, frame0 and rate1; stop sets rate0; isStop checks null transform, state1 or rate0.

The reached native error came from named-pane LayoutRuntime methods additionally calling animation(animLayer), which validates against the unrelated whole-layout count. All named-pane start/stop/frame/rate/query paths, including debugPaneAnimEndFrame, need local registered-controller capacity. No increase of whole-layout count is warranted.

Binding has a second ownership boundary: when LayoutManager::_61 is false, bindPaneCtrlAnimSub and unbindPaneCtrlAnimSub skip the entire subtree at another independently controlled descendant. With _61 true, original LayoutPaneCtrl binds recursively without that exclusion. Each pane/player has an independent frame controller even when getAnimTransform resolves the same shared BRLAN transform; reflectFrame publishes that player's frame before its pane evaluation.

This was bounded reference guidance to Peirce's active production fix; no production edits or tests were made by this lane.
