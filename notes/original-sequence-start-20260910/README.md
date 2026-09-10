# Original input processing and sequence prerequisite audit — 2026-09-10

Previous goal turn is **progress**: published root code60df3871d and evidence9b6a17412 plus Aurora7f93df56; ten focused executables and a 960-tick real-disc movement replay passed. Full Gateway chase/Rosalina remains incomplete.

Starting root is `9b6a1741219bf4b195fa18cad3af438a31613e0b`. The only starting dirty paths are the user's staged documentation deletions, unrelated original-sequence-galaxy-move note and untracked package_walking_demo.py; preserve them.

The player cleanup exposed the next missing owner: original GameSequenceProgress::startScene sets pointer mode, controller/dimming policy, story entry, timers/wipes and conditional spin permission before GameSequenceFunction::startScene invokes the actual scene start. Native SceneLifecycleService currently starts GameScene immediately during creation, while SceneTransitionRequestService owns a separate StorySequenceExecutor and bypasses GameSequenceProgress::requestGalaxyMove. Native GameSequenceFunction.cpp is a save-only replacement, and GameSystem/GameSequenceDirector/GameSequenceProgress are absent from native source.

The intended direction is complete original owner/source activation, with platform scheduling, heap, resource and input support outside Game. The sequence probes establish prerequisites, not a running original sequence. A copied spin-lock call, another fake progress owner or unimplemented no-op dependencies would hide that integration gap.

## Working source restoration

The native port now compiles the complete original `WPadStick.cpp`, byte-identical to the corrected decomp reference. Two missing current-sample stores were recovered from retail Wii assembly before copying the source. Original strict thresholds, directional hold/trigger/release, speed, vertical navigation and unsupported-device early return remain intact. The duplicated compatibility constructor is removed.

Aurora now exposes the exact 0x84-byte SDK KPADStatus and FreeStyle/Classic extension union. Its generalized native device selection explicitly reports the keyboard/mouse controller as FreeStyle, including a neutral stick. It publishes actual axes and acceleration samples through KPADRead, with matching WPADProbe metadata. Core devices cannot publish extension-only C/Z buttons. There is no game-specific condition in Aurora.

The retained original WPad records run original button, pointer and stick processing before camera/player updates. Six stick utility bodies now read that processed original object rather than the parallel Aurora stick cache. Other GamePadUtil methods and the full WPad/WPadHolder update still require further original-owner recovery; this cohort does not claim they have been replaced. The host method is renamed to `update_samples` with no obsolete alias.

## Validation

- Aurora's standalone linked WPAD fixture: **18/18 passed**, including complete record offsets/layout, explicit device capabilities, input transitions, independent acceleration deltas and bounded output writes. See `input-sdk/`.
- Linked original pointer/stick, WPad pause and camera director executables: **3/3 passed**. Pointer/stick runs actual input heaps repeatedly, all four directional edges, strict thresholds, processed-sample timing, stable neutral input and nonzero last-sample retention on disconnect. See `native-input-results.json`.
- The edited broad AuroraNative fixture compiled and linked. Its previously documented unrelated scene/scheduler/model failures were not fixed; the full suite is not claimed passing. Analog checks formerly bypassing the original owner moved into the actual original-input fixture, and its absent-owner contract replaces those invalid assumptions.
- The exact showcase binary `7b14a42624902be493d423a97f20361aa222fea0614bf53334443789eac25f3f` passed **13/13 checks** during a 960-tick real-disc, synthetic SDL replay at 1280x720. About **59.93 FPS**, exit 0, no timeout. Idle drift 0; W/A/S/D and releases accepted; jumps at 600/780 landed at 635/815; all observed Mario/camera values finite. See `demo-measurement.json` and `demo-validation.json`. Raw trace remains local at `notes/preview-fps-crash-20260910/original-nunchuk-20260910.log`.
- The complete original stick TU compiles with the Wii compiler and native LLVM 23. Restoring the required stores changes update's fuzzy score from 99.22% to 96.08%; the higher incomplete score is not behavioral proof. See `stick/` for instruction evidence.

Independent review found no material input integration regression. Its suggested nonzero-disconnect coverage was added and passed. All changed production source hashes and the exact decomp/native equality check are in `native-source-manifest.json`.

## Sequence findings and remaining work

Whole original GameSequenceProgress compiles unchanged with seven exact header overlays. Its direct imports expose 36 missing project definitions plus six present fail-loud dependencies; link presence alone would not create the actual GameSystem/Director/Progress owner graph. `progress-dependency-audit.md` records the measured boundary and `save-sequence/sequence-owner-audit.md` maps duplicated native ownership and startup ordering to the originals.

Whole original SaveDataHandleSequence and NANDErrorSequence also compile in a scratch overlay. Their initialization reaches native GameDataHolder's unimplemented serialization. The next coherent prerequisite is its actual six-chunk save owner, preserving Wii byte order and encoded event IDs at the format boundary. The data audit independently found and recovered two incorrect reference methods against Wii assembly: event progress comparison and event-bit clearing. These reference corrections do not activate native GameDataHolder. See `game-data-owner/` for evidence and further layout/stream prerequisites.

The native input adapter still publishes one non-consuming snapshot per frame; original 200 Hz Wii filtering/calibration is not implemented. Full sequence startup, persistent saves, bunny chase and Rosalina remain incomplete. This checkpoint is concrete input restoration and a refreshed movement demo, not completion of the overall goal.

## Published checkpoint and local demo

Published as author and committer `codex <codex@openai.com>`, with each remote branch hash verified after pushing:

- Aurora `30b06fd6f93229106a6a63220cd3f2062cd08ba1` on `codex/macos-compat`.
- Decomp `11157e7c391da10ad2713dd3738e98c84d47f7ca` on `pcp-decomp`.
- Root source/evidence `a987f3c05deef716b2f9499c56bfbf565db6c71f` on `pcp-aurora`, after both submodule publications.

The existing local `build/playable-demo/Super Mario Galaxy Movement Demo.app` was refreshed without rebuilding. Its copied executable is exactly the validated `7b14a42624902be493d423a97f20361aa222fea0614bf53334443789eac25f3f`; packaging checked its signature, architecture and system-library dependencies. The prior `89b80b...` app remains at `build/demo-history/original-nunchuk-20260910-163501/Super Mario Galaxy Movement Demo.app`. `demo-package.json` records the copied hash, validation description, backup and packaging-time source snapshot. The snapshot is provenance, not a claim of compiler attestation.

The user's staged documentation deletions, unrelated prior note edit and untracked walking packager were preserved. The original SMGCommunity merge remains in the branch history; this cohort adds no Game gameplay rewrite beyond importing the complete recovered original stick TU.
