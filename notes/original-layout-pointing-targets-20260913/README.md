# Original layout pointing targets

The eighth original-process launch reaches a stale LayoutActor::initPointingTarget rejection that predates the real NW4R pane graph. Restored the exact already-decompiled LayoutActor body: it constructs the original StarPointerLayoutTargetKeeper. Existing original circle/rectangle targets and StarPointerUtil use their ordinary pane-to-screen projection and hit rules. No substitute hit boxes or layout-specific pointer rules are introduced.

Native layout retirement now releases the keeper, its pointer array and registered target objects before destroying the pane graph they borrow. Original Game initialization and target methods are unchanged. No component test added; production build/run is the integration check.
