# Publication and refreshed movement demo

The code checkpoint is **60df3871d0a6a70e160863ccfdbd3c6853460cdf** on pcp-aurora. Both author and committer are codex <codex@openai.com>. The push completed and origin/pcp-aurora was independently verified at that exact SHA.

Aurora was published first at **7f93df56a1053deb9b2572a1af8fd4044c963078** on codex/macos-compat, also authored/committed by codex, with its remote SHA verified. The unchanged decomp reference is **ef44fad5dbc7def9d25cf149b823e1cf512bd3e2** on pcp-decomp; its published SHA was checked again. The root code checkpoint includes the new Aurora gitlink.

The local app at `build/playable-demo/Super Mario Galaxy Movement Demo.app` now contains the exact already-replayed executable **89b80b085aa6d0f9949e3db3e26b7c817e9bb182e12402ea1fceef2a586a2488**. Packaging did not rebuild or launch it. The packager verifies the copied executable hash, local signature, launch script and system-library dependencies. Its embedded provenance identifies the code checkpoint above and includes the passing 960-tick movement validation. `package-result.json` records the exact app, backup path and command.

The preceding app remains available in `build/demo-history/original-player-camera-20260910-150323/`. The refreshed app still uses the external local disc, normal 1280x720 launch and no frame limit. No game assets were copied into the bundle.

The user's staged DISCREPENCY_REPORT.md/MACOS.md deletions, unrelated sequence note and untracked package_walking_demo.py were excluded. After the code commit, these were the only remaining non-ignored working-tree changes. This evidence-only follow-up does not alter the code used to build or validate the app.
