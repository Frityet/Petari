# Message lookup: real text or absent

## Outcome

Missing BMG identifiers are no longer converted into visible identifier text.

- `Game/Util/MessageUtil.{cpp,hpp}` are byte-for-byte identical to the root
  decompilation sources.
- The retail source is excluded until `MessageHolder`, `TalkMessageInfo`, and
  the remaining message-system dependency closure are available.
- `compat/MessageUtilCompat.cpp` implements the currently linked retail lookup
  surface. It returns `nullptr` when no runtime or real message entry exists.
- The host `LayoutUtil` path now supplies an empty string for an absent entry,
  never the BMG identifier.
- `MessageService` no longer exposes `message_or`, `message_utf16_or`,
  `message_raw_utf16_or`, or `format_message_utf16_or` APIs that let callers
  invent fallback text.

The bridge contains no route, stage, language, or message-ID exceptions.

## Verification

`MessageRealOrAbsentTests.cpp` verifies that all text and metadata lookups for
a missing ID return absence, formatting returns empty text, and the retail
direct lookup functions return null without a real runtime archive. It also
checks that a present message retains its supplied UTF-16 text.

See `verification.log` for the build and test results.
