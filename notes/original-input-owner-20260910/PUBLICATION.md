# Published original controller checkpoint

All commits use codex <codex@openai.com> as author and committer. Submodules were pushed before the root gitlinks.

- Root code/evidence: `794ce6cf74c3973208328f5519647a017d6c3d2f` on origin/pcp-aurora.
- Decomp recovery: `f68c8a0231e8406ca897f649e90b714209c5a6ec`; latest upstream merge: `ec3406dfcca2dc829d2261483a77692d86067a20` on origin/pcp-decomp.
- Latest merged SMGCommunity upstream: `d1ae0a05cc023d52ecdcbc7731c8c79f0cb84dc6`.
- Aurora BRLAN: `760e7f195a4484b166df7050e9871f00ad06ff36`; SDK/keyboard input: `a941314ff303c73f34e9a919b7b08349df625561` on origin/codex/macos-compat.

Remote branch SHAs were checked against the published source heads. Source/header comparison is18/18 exact to decomp. The five focused linked input/player fixtures pass after final cleanup; the showcase links. Aurora WPAD passes31/31 sanitizer checks; BRLAN passes10/10 plus28,672 original-result comparisons.

The final packaged executable `019f7be6e2f99ef7db96cb573f9334eba41f63739271f327d7266a68eb8f0f30` passed13/13 movement/jump/camera replay checks over960 ticks at1280x720,59.939 FPS and exit0. Packaging verified the identical executable hash and local executable signature without rebuilding.

- Updated app: `/Users/frityet/Projects/petari/build/playable-demo/Super Mario Galaxy Movement Demo.app`.
- Previous working app retained: `/Users/frityet/Projects/petari/build/demo-history/before-original-input-owner-20260910-180308/Super Mario Galaxy Movement Demo.app`.
- Packaging provenance: `packaging.json`; runtime identity/results: `demo-runtime-final.json`; assertions: `demo-validation-final.json`.

The two user-staged document deletions, unrelated sequence note edit and untracked walking packager were preserved. The complete Gateway bunny/Rosalina sequence is still an active goal; this publication establishes the bounded movement demo and original input owner integration.
