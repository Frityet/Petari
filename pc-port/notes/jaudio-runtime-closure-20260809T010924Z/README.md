# Generalized retail JAudio runtime closure

## Scope and boundary

- No `src/Game` file was edited by this lane. The pre-existing SaveIcon and TriggerChecker changes were left untouched.
- Generic PCM mixing, STRM/BLCK decoding, and BAA/BST/BSTN/BSC/IBNK/WSYS/AW parsing live in Aurora.
- SMG compat resolves retail disc overlays, exact Game policy, RuntimeContext ownership, and AudWrap/SoundUtil state synchronization.
- `JAISoundHandle` attachment now always names a concrete backend owner and mixer token. Event recording happens only after backend allocation succeeds.
- Controller-speaker playback is explicitly absent. Retail's optional ordinary-SE substitute is used only when the caller supplies it.
- ME remains exactly unavailable (`hasME() == false`), so the retail guarded `startSystemME` path is a no-op.
- Multi-BGM/sequence scheduling remains explicitly unavailable. FileSelector and SphereSelector therefore remain absent rather than entering with fake audio.

## Retail asset evidence

- `STM_TITLE` = `0x02000001`, `/AudioRes/Stream/SMG_title_strm.ast`.
- `STM_PROLOGUE_01` = `0x0200001f`, `/AudioRes/Stream/SMG_ev_prolo01_strm.ast`.
- `STM_PROLOGUE_01_B` = `0x02000046`, `/AudioRes/Stream/SMG_ev_prolo01_b_strm.ast`.
- `STM_PROLOGUE_02` = `0x02000020`, `/AudioRes/Stream/SMG_ev_prolo02_strm.ast`.
- `SE_SY_GAME_START` = `0x20`, 21 finite layers spanning bank 64/program 74 and bank 65/program 4.
- `SE_SY_TALK_FOCUS_ITEM` = `0x36`, four finite bank-64/program-13 layers.
- `SE_SV_PEACH_OPENING_LETTER` uses duration-zero sample-completion dependencies; its second note begins at 2.075233333 s, not at the static wait-only time.
- `SE_DM_ARRIVE_CASTLE_STAR` = `0x00070016`, one finite layer.
- `SE_DM_ASTRO_HANDLE_GRAB` = `0x00070005`, two bank-88 finite layers.
- `SE_SY_GALAXY_SELECTED` = `0x5e`, 17 finite layers.
- Localized SMR wave references are resolved through a generalized localized-then-base `AudioRes/Waves` overlay. This is required for GAME_START bank 65 and Prologue/Sphere banks 115/88.
- Exact AudBgmSetting tables are consumed from the ordinary mirrored Game source recovered in parent commit `efc0452d9`; no title/FileSelect/Sphere table is embedded in compat.

## Runtime invariants covered

- Prepared stream callbacks advance global SDL statistics while the paused voice token stays at rendered frame zero; unlock advances that same token.
- Preparation lock and ordinary host pause are independent: host unpause cannot bypass preparation, and unlock cannot bypass a retained host pause.
- Prepared-before-unlock and explicitly paused streams both release the host mixer pause when stopped, remain `stopping` through a nonzero fade, then retire and detach.
- Nonzero stage fade remains `stopping` with its handle attached, then retires the token, detaches the service handle, and releases the facade keeper during the normal RuntimeContext audio-frame lifecycle.
- Submit suppresses a level allocation and does not record a fake start; permit restores concrete allocation.
- Name queries resolve name-to-ID through the retail archive and compare the backend ID, including raw-ID starts.
- Logical active stage state without a matching backend throws. A concrete non-stopping backend without logical state also throws.
- Active track mutation and next-BGM queueing throw until their real schedulers/backend layer controls exist. Pre-existing queue metadata is rejected during facade synchronization.
- Malformed BSC register shifts and overflowing duration/gate-rate products are rejected with checked arithmetic. Impossible STRM sample counts are rejected against payload capacity before vector allocation.
- Retained Prologue streams `STM_PROLOGUE_01` and `STM_PROLOGUE_02` decode, attach concrete tokens, render, and detach in the playback proof. Its reachable finite SEs likewise pass through concrete mixer start/stop.

## Known strict absences

- JAudio sequence/multi-BGM scheduler: blocks MBGM_FILE_SELECT, MBGM_LIBRARY, and stage multi-BGM changes.
- JAudio BME/BMT/CITS ME scheduler: `hasME()` is false.
- Positional actor sound-object backend: actor `startSound`/`startLevelSound` throw instead of routing globally.
- Controller speaker backend: exact speaker-unavailable branch only.

See `focused-test-results.txt` for the build/test commands and results from this freeze.
