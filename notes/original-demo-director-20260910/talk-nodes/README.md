# Original talk nodes and NW4R message processing — 2026-09-10

The port now compiles the complete original `TalkNodeCtrl` and `TalkMessageHistory` implementation. `TalkRuntime` no longer reconstructs a second flow graph or copies message text into controller buffers: Game nodes and text point into the actual `MessageHolder` resource owner. Controller retirement releases the original copied flow name, node controller and camera allocation. The remaining native TalkMessageCtrl/TalkFunction request/presentation services are still a frontier; this is not full TalkDirector activation.

The parent owns the actual scene alias binding. `SceneObjHolderBinding` exposes the real process holder before scene Talk controllers are created and restores/clears it only after their retirement. Missing process ownership stays explicit; this cohort creates no GameSystem or scene stand-in.

## Original source and native seams

- Complete original TalkNodeCtrl source has 23/23 measured functions at **100%** against the RMGK01 original object (`nodes-reference.proof.json`). No traversal branch was rewritten for native behavior.
- Missing `MessageEditorMessageTag::getParam32` and `MessageTagSkipTagProcessor::skipTag` were recovered in decomp first after reading `decomp/AGENT_DECOMP_GUIDE.md`. The full Wii TU compiles. getParam32 is **100%**; skipTag is **87.77778%** with the same byte-length arithmetic and one redundant retail context-pointer load optimized away. Existing Process, CalcRect, constructor and measured helpers stay **100%**. `NO_INLINE` retains the original caller boundary. See baseline/final proof JSONs and retained objdiff reports.
- Native wchar_t stores one original UTF-16 code unit per 32-bit native unit. The narrow architecture seams use native-unit pointer advancement in getSubMessage, obtain packed tag byte length from the widened word, and combine original big-endian u32 parameter halves. Original reference branches remain intact.
- The five complete original NW4R source TUs (`ut_TagProcessorBase`, `ut_TextWriterBase`, `ut_CharWriter`, `ut_CharStrmReader`, `ut_Font`) and their full declarations are imported. Font reader selection now runs after real ResFont resource initialization. A trailing reader field distinguishes an explicitly supplied wchar_t stream from raw UTF-16 scalar input; the original encoding-selected reader functions otherwise remain unchanged.
- Native Rect preserves original constructors/operations using scalar conditional selection instead of the PPC-only FSelect header. Existing native Color packing and actual Font resource ownership are preserved. JMapUtil gains the previously missing exact original getMessageID inline helper.
- Direct non-flow messages intentionally retain the original node index of -1. Native display, trace and completion metadata resolve their catalog index separately through actual MessageData and never mutate that Game field.
- The two former duplicate tag definitions and all copied TalkNodeCtrl/History/RecursiveHelper providers were removed from compat.

## Bounded validation and build hook

`native-proof.json` records syntax success for all 10 affected production TUs; `fixture-syntax.json` records the new helper syntax success. The parent registers the five NW4R TUs with contraction disabled and adds `tests/OriginalTalkNodeTests.cpp` to the existing `smg-pc-original-message-holder-tests` target. No shared Xmake build was run by this agent.

The focused helper extends the existing complete real-disc MessageHolder fixture. It checks original current/next pointers and sentinel behavior across authored FLW nodes, both available branch choices, local event-read history without persistent-save dependencies, copied-name/resource-pointer ownership, reset cursors, and group-8 submessage lookup returning actual retained text. It also exercises original tag skip/default newline/tab dispatch through the complete original TextWriter, widened u32 tag parameters, and actual font readers. It runs in both existing owner generations. The live fixture and production smoke are **pending parent execution**; syntax and Wii matching are not runtime claims.

`source-manifest.json` lists only this agent's source/reference/test cohort. Parent-owned scene alias and build hooks are separate. Existing user deletions and unrelated pending work were preserved.
