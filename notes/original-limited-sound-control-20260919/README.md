# Retain the original limited-sound state

The source-confirmed next audio boundary before the tower demo is
RunawayTico::exeWhiteOut / exeWhiteIn calling `MR::limitedSound` after the third
rabbit conversation. The native utility previously resolved the real name and
then requested an absent AudSystem. The live run has not yet reached this
specific call; the authored source establishes the dependency.

The original AudLimitedSound.cpp is copied byte-for-byte from the existing
decompilation. Its native header was already present and is identical to the
reference. No new decompilation or Game behavior change was made.

JAudioLimitedSoundOwnership retains exactly two actual AudLimitedSoundInfo
records, matching AudSystem's constructor. Registering an existing ID does not
refresh its countdown; a full table silently declines another registration;
zero or negative delay still occupies a slot until the next original update.
The original record implementation decrements and clears itself. The owner
also retains the original explicit clear operation. It creates no AudSystem,
voice, output device or name-table replacement.

The native utility preserves original name-to-ID resolution, then dispatches to
the actual disabled-object-audio owner. Missing-owner AudSystem access continues
to reject unsupported use. The original process advances limiter records once
at the existing audio wrapper movement. A service that already owns concrete
PCM delegates to that backend; its frame edge advances the same record type.

Before registering, the PCM backend stops every matching level sound, one-shot
and retained stream ID, even for duplicate registration or a full table. Actual
PCM voices enter their original stop/release path; release tails are preserved.
New matching object sound-effect/level requests are suppressed until expiry.
There is no system/home category exemption for this limiter in the original
AudSoundObject, and direct BGM starts do not acquire an invented limiter gate.

AudSystem::initSceneVolume and resumeReset do not clear limited-sound records.
Accordingly, neither native scene-control reset nor retained playback scene
reset clears this state; construction and explicit record clear initialize it.
The stop-by-ID adapter is restricted to voices actually owned by its native
service. No sequence-track support is claimed or fabricated.

The existing OriginalJaiSoundOwnershipTests fixture now checks:

- scene-only: two-slot capacity, duplicate expiry, zero/negative delay,
  explicit record clear and persistence across original scene volume reset;
- backend-only: original sound-name conversion and MR::limitedSound on the
  worker-created owner, with wrapper-driven expiry;
- full retail mode: actual level/one-shot release, system-SE suppression,
  stream stop even when no limiter slot is free, reset persistence and renewed
  actual playback after expiry.

`source-equivalence.json` records exact donor hashes and the native utility
comparison. Eight native output-control helper bodies now differ from the
reference; the other 78 definitions are unchanged. The prior seven-function
comparison in original-audio-output-controls-20260919 applies to that earlier
committed checkpoint.

Source comparison and targeted `git diff --check` pass.

## Validation

After the live debugger session ended and the concurrent Aurora FIFO sources
were frozen, `python3 notes/original-limited-sound-control-20260919/validate.py`
completed the following serialized commands successfully:

- `xmake build smg-pc-original-jai-sound-ownership-tests`: exit 0 in 11.54 s;
  its log includes the exact donor and new native limiter owner compilation.
- `--scene-only`: exit 0, including limiter capacity, countdown, duplicate,
  zero/negative delay and original scene reset checks.
- `--backend-only`: exit 0 against the retained retail fixture, including
  original sound-name conversion and wrapper-driven limiter expiry after real
  OS-worker construction, across three heap lifetimes.
- Full retail fixture: exit 0 in 6.375 s, with no skipped coverage; actual
  level/one-shot/stream stopping, SE suppression, original reset persistence,
  duplicate expiry and restored playback assertions all passed.

The existing retained fixture was selected explicitly through
`SMGPC_RETAIL_FILES_ROOT`. No dummy audio driver was selected. Logs are
`build.log`, `scene-only.log`, `backend-only.log` and `full-retail.log`.
`validation.json` records commands, results, elapsed times and the test binary
SHA256 `4a54239708b05748e69f2607dea5ad6c955df46398bb77b93d7e99f591d45061`.
It also proves the concurrently inspected PointLightRuntimeTests.cpp.o retained
the same hash; that test target was not rebuilt. No correction was needed after
these test runs, and no parent commit was made by this task.

This validates the audio ownership and control paths. Runtime passage through
the third catch, tower transition and Rosalina appearance remains unverified.
