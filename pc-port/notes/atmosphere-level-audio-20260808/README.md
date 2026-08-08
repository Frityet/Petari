# Retail atmosphere level audio foundation

Date: 2026-08-08

## Result

The PC runtime now has a real, generalized playback path for JAudio atmosphere
level sounds. It resolves the sound name through retail `SMR.szs` metadata,
loads the referenced wave archive by its retail WSYS name, decodes JAudio AFC
HQ into PCM, and mixes persistent looping voices through an opened SDL3 output
stream. Returned `JAISoundHandle` objects carry a concrete backend voice token;
there is no event-only or silent-success substitute in this path.

The exact `SphereSelectorHandle` remains absent from the production factory.
Its remaining runtime-reachable audio calls include system SE/ME and stage-BGM
operations, whose older compatibility paths are still logical/event-only. The
factory therefore reports
`system_se_me_and_stage_bgm_playback_runtime_unavailable` instead of treating
atmosphere playback alone as a complete route.

## Retail evidence

Both Korean disc revisions used for comparison contain byte-identical audio
inputs for this slice:

- `KrKorean/AudioRes/SMR.szs`: 196,608 bytes, SHA-256
  `a9f7d2f7828052098a1828b1cd172f301f2b2091526dd207dde154b59f9f49aa`;
- `KrKorean/AudioRes/Waves/B64kawa_0.aw`: 4,770,848 bytes, SHA-256
  `088f96c7ac62ace546a5f3234cfcdd4d2a2b478b8a2629e3195a1f608efcbbc8`.

The BAA/BST/BSTN/BSC/IBNK/WSYS chain resolves the two retail wind sounds to
looped waves `0x008c + 0x002b` and `0x008c + 0x0029`. The decoder follows the
cloned Dolphin Zelda/JAudio oracle: JAudio AFC HQ uses 9-byte blocks for 16
samples, the high header nibble is the delta exponent, and the low nibble
selects one of all 16 coefficient pairs in the global JAudio table.

Decoded PCM SHA-256 oracles:

- wave `0x008c`: `fcea2c737cc5d2a9d6bd5180ce0276df967718b66bc07111ad7a1f87eacaa2f1`;
- wave `0x002b`: `97c1ca349eea02ff337b6edc203add1f9fbde183b04d6dad53cec171974cfbad`;
- wave `0x0029`: `d8bb87c35e7d6b6a5a0960c525b583dd8ab12c84ab1ca6ec037d22a3ea5a9a1d`.

## Runtime contract

- A sound request fails if its name or parameter semantics are unproven, its
  retail resource chain is malformed/missing, SDL cannot open a real output,
  or its backend handle is inconsistent.
- Calls in consecutive frames refresh the same retail voice/token and update
  gain/pitch atomically with respect to the callback.
- A missed frame starts retail-style release. Refreshing a voice that is
  already stopping does not revive it; a new token is created only after the
  old handle detaches, matching JAudio lifetime behavior.
- Callback-owned retired voices are reclaimed outside the real-time callback.
- Scene teardown detaches every handle, stops voices, and supports clean scene
  re-entry, including teardown during an open runtime frame.
- SDL dummy/disk drivers are accepted only by explicit focused-test injection;
  production default-output construction rejects them.

## Verification

See `verification.log`. The focused test proves retail metadata and PCM hashes,
malformed-resource bounds, real SDL callback activity with nonzero samples,
stable token/update semantics, missed-frame release, post-release restart,
device removal, scene teardown/re-entry, and production driver rejection.

No `Game/` source was edited for this work.
