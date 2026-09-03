# Mario::updateBinderInfo retail review

The preceding compile-foundation comparison exposed a 65.60863% match for
the historical root function. Native activation is still disabled. This
bounded follow-up checks RMGK01 `0x802D2FE8`, size `0x9C8`, against the supplied
DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

Retail findings used in the completed reconstruction:

- The retail tail (`0x802D392C..0x802D3934`) only normalizes `_3A4` and returns
  true. The old source's following Binder ground/wall/roof assignment block is
  absent from retail. In particular this routine does not replace the game's
  own grounded flag or shadow/ground position with Binder summaries.
- Retail obtains each contact through `Binder::getPlane`. The current original
  getter directly returns `&mPlane[index]`; this is an API/call correspondence
  correction, with no separate contact-storage behavior claimed.
- Ground pushing requires `mVerticalSpeed > 30`, not `> 0`. A contact that
  needs no corner/edge push does not consume the first-push allowance.
- The direction selected from contact geometry and the actor-to-contact vector
  are distinct retained temporaries. The old source normalizes the former
  where retail normalizes the latter.
- For dot >= 0.707 on an edge, retail strips normal velocity and selects push
  scale 1; for dot <= -0.707 it selects 0. The old source reverses these values.
- The inner correction uses movement bit `_14`, not hip-drop `_B`, and clears
  that same `_14` bit afterward. Its angle/normal constants are the exact
  loaded values 2.3561945, 1.4959966, 0 and 0.17; the old source substitutes
  150, 120, 70 and -0.8.
- Opposing-normal correction scales the removed normal component by positive
  0.5 after constructing that vector, not by -2.
- The stalled-wall branch reads actor `_27C`, removes its air-gravity
  component and tests the returned scalar. Its correction uses wall*2 and
  negative gravity*5, and its no-ceiling translation is gravity*-30. The old
  source uses a virtual last-move query, wall*3 and gravity*-1. The later
  hip-drop branch does retain its own original virtual last-move query.
- The hip-drop steering blend uses `_1A8` and starts its dot weight at 1;
  the old source substitutes front direction and weight 0. Front direction
  remains the actual final near-zero fallback.
- A ceiling correction preserves the original host-name exception stored at
  `0x805C497B`, `マンホールのふた(クッパ船)`. This is a retail branch recovered
  from its actual HA/LO address and host pointer loads, not a new host rule.

All findings above come from the actual instruction/relocation graph, not the
fuzzy percentage alone. Raw disassembly, baseline source/object, and baseline
objdiff stay under `build/original-mario-binder-info-20260903/`. The completed
reconstruction passes the full 626-instruction canonical comparison and native
syntax check described in [README.md](README.md).
