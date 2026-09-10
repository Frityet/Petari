# Original process messaging children

Imported complete original NWC24Messenger, NWC24System, NWC24SendThread, NWC24Function, UTF16Util and LuigiMailDirector sources and headers. These are required children of the original GameSequenceDirector/FindingLuigiEventScheduler, even while the selected scene sends no messages. Imports preserve the original background queue, retry and save-callback behavior.

Recovered MR::calcWiiMailSize in decomp first from RMGK01 0x804085F0–0x804086E4, then copied the translation unit into native Game. It preserves the four original UTF-16 length calls, 364-byte fixed overhead, unsigned base64-size arithmetic, 57-byte line breaks, and three four-byte separators. A single Wii compile and bounded object comparison completed: 244-byte retail function, 92.78689% match. No tuning loop or unrelated decomp tests were run. retail-recovery.json records the exact comparison command and result.

The native OS worker support is supplied by the shared Aurora thread implementation, not a synchronous send substitute. The NWC24/VF SDK storage and message API implementation remains a separate unimplemented native boundary; importing its types or Game code does not make network or message-board delivery work. No messages were sent. Native wchar_t-to-UTF16 conversion must also be addressed at the appropriate width boundary before enabling the real send path.

The integrated compiler exposed two original SendState comparisons between its bool-valued selectTask() and nullptr. Fixed both in decomp first as boolean conditions and mirrored; no blanket nullptr macro. Added the general Metrowerks size_t.h forwarding header outside Game.
