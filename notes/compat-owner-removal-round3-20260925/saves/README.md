# Save serializers replace SaveChunkEncoding

The original chunk classes now handle their packed scalar fields directly. `JSUInputStream::readBig(T&)` preserves unread bytes from the initialized caller value, matching a partial raw read on Wii. `writeBig(T)` writes only the stream's available prefix. Neither changes the existing raw stream API or native typed helpers.

`BinaryDataChunkBase::validateData` is a native preflight hook. Each real chunk owns its checks. `BinaryDataContentAccessor::validate` checks the shared descriptor format, first matching attribute semantics, field extents/overlap, record counts and required/optional attributes. GALA and SYSC provide their own schemas. The holder validates all chunks before deserializing any, then invokes original virtual serializers directly; there is no signature-specific adapter, transformed copy buffer or replacement game state.

MISC's legacy one-byte payload remains supported. PLAY retains initialized missing bytes and signed stream-size behavior. Other fixed/structured chunk outputs reject null or oversized destinations; invalid input cannot enter an unbounded original parser. SPN1's format checks belong to SpinDriverPathStorage, with bounded writes at its actual scenario/galaxy serializers.

New tests exercise actual save ownership in the original process. SaveConfig's exact-source check remains for unaffected ConfigDataHolder/ConfigDataMii/UserFile owners. The pre-existing SaveDataHandleSequence native functor include and nerve-macro substitutions are no longer incorrectly asserted byte-identical; schema and binary behavior checks remain. No changes were made to SaveDataHandleSequence.
