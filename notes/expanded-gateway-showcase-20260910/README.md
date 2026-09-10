# Expanded Gateway showcase — 2026-09-10

Created the requested fresh local app at:

`/Users/frityet/Projects/petari/build/playable-demo-20260910-fc91f414/Super Mario Galaxy Gateway Showcase.app`

The ordinary Gateway route automatically creates every supported active authored placement. No production feature flags or actor filters needed changing: the fresh package exposes the latest original Lumas, stage/collision/camera work and shared compatibility improvements. It keeps the regular story checkpoint and does not enable the separate stand-in spin route. Coins, star pieces and the complete bunny/Rosalina sequence still have actual provider gaps; see `feature-audit.md`.

One 240-frame run with scripted walking/jump inputs exited 0 at 59.59 displayed FPS. It constructed all 78 ready stage objects plus Mario. No further gameplay replay was run. `smoke.json` and compressed `smoke.log.gz` retain that evidence, and `runtime-validation.txt` is included in the app. This short run is not a full gameplay-completion claim.

The local packager copied exactly binary SHA256 `fc91f414cd3fc44501eae46e69fc16fbeb6529b1056545e6ab8fa9ef790faa83`, uses 1280×720 with unlimited duration, and keeps the existing external disc. The older movement app remains unchanged. Added a display-name option to the existing packager so the new app is distinguishable in Finder/Launch Services. User `script/package_walking_demo.py` was preserved. The app's own provenance records code commit `b4bf0c7cc62ff7ea90c45584caa8a6fe102b8ba4`; the follow-up commit adds only this completion receipt.

Changes were committed and pushed as codex, submodules before root. `package-receipt.json` records the publication hashes and local output.
