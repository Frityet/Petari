# Title screen and opening sequence restoration

Focus: normal boot through the title screen, new-file flow, prologue and cutscenes. Use the original sequence/scene/actor owners and general host/SDK fixes. No stage injection as evidence of title-to-intro success.

Starting root: fecb0ea1f. Aurora: 76573611d76a7bb6bfce8ae60e03813d8db9c566. Existing editor changes, historical notes and staged rabbit-route work are unrelated and preserved (see initial-status.txt). Another task is finishing tester packaging in a separate Linux container.

The current README reports FileSelect as incomplete; verify that claim against this checkout. Source inspection finds normal GameSystem -> Logo -> original story sequence -> FileSelect; FileSelector and TitleSequenceProduct are linked. Existing Logo strap bypass is unrelated to the title layout itself. First step is an unmodified, fresh-save boot with actor/scene traces.

## Reproduction and implementation

The fresh baseline ran 900 frames to FileSelect/Title, displaying only the sky. TitleSequenceProduct waits for isPreparedStageBgm, but the native AudBgmMgr start rejected every request while output was disabled. The baseline also aborts on shutdown because scene retirement individually deletes unclaimed FileSelectEffect array elements. Trace shows the older FileSelector control writes only Z when interpolating and adds the selection effect offset to the actual planet every frame; all planets start at the first slot. Current decomp already fixes all three behaviors and additional selection/save UI branches.

Restored FileSelector.cpp from the pinned decomp with only native compilation/text-width adaptations. Its actual owner now claims/deletes its effect array to keep scene retirement from deleting interior pointers. This is an original-source restoration plus native array lifetime handling, not a placement workaround.

The user requested audible output during the work. The preliminary silent-voice experiment was discarded before publication. The current work binds real JAIStreamMgr/JAIStream objects to Aurora's PCM decoder and SDL mixer. Resource metadata and stream files come from the mounted disc; readiness follows full decoding and actual voice playback follows the original prepare/lock/unlock states. Each stream owns its host PCM data/token. There is no pointer-keyed voice registry. DSP sequences and effects remain a separate incomplete engine; first validate streamed title music and the original title flow before expanding further.

## First live checkpoint

`core-flow-1` reached title and new-file Mii selection from a fresh save and shut down cleanly after SIGTERM requested SDL exit (4,372 frames). SDL delivered 3,421 audio callbacks, mixing 3,503,104 frames and 712,654 nonzero samples. The music is decoded from the mounted retail title AST, with no synthetic ready signal. `title-with-stream.png` records the title logo and A+B prompt. Focused original-JAI tests cover preparation locking, PCM output, pause, EOF, fade and owner retirement.

Manual2P now claims its owned BackButton so general scene retirement leaves it to its existing destructor. The earlier live run exposed that double delete after the effect-array ownership correction.

The new-file blocker is the port-specific MiiSelect replacement: it never created MiiSelectIcon actors, leaving an empty icon chooser. Restoring the pinned decomp selector and its existing createLytTexMap(ResTIMG*) overload; only CP932, missing direct includes and native wchar_t conversion differ. User steering prioritizes title -> picturebook -> intro gameplay -> Gateway over smaller bugs.

## Original title to opening gameplay reached

`core-flow-2` uses normal boot and a fresh sandboxed NAND. Original FileSelector selected/created a file, the restored decomp MiiSelect completed icon confirmation, and GameSequence transitioned into PeachCastleGardenGalaxy. PrologueDirector progressed through PictureBook, PeachLetter, Arrive and GameStart. `core-opening-gameplay.png` records Mario in the opening festival area. `core-flow-2-transitions.json` extracts scene/nerve transitions from the live trace. No stage injection or game-state mutation was used. These traces prove progression; they do not yet prove all picturebook visuals or the later Gateway handoff. The debug controller script was empty throughout this run, with input supplied through the live window.

Aurora 3767682 publishes an initial-paused PCM voice state before the device callback can see the voice. The original JAI lifetime test also verifies this through the stream manager. DSP sequences and effects remain incomplete; title and prologue retail AST music are active.
