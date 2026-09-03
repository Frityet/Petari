# Original heap and animation integration checkpoint

This checkpoint supplies actual JKR base, expanding and solid heaps, disposer
lists, retained native allocation domains, and the original twelve J3D animation
loader families. It also supplies the general MSL string-format boundary needed
by original resource naming and Aurora's serial diagnostic/fatal providers.
The original model/ResourceHolder owner and original jump integration remain in
progress; no jumping or completed demo claim follows from this checkpoint.

Original allocation scopes route ordinary/aligned/JKR new through the selected
actual heap. Native parser/cache/registry ownership remains outside those heaps.
Allocation provenance supports external-buffer children and original bulk/tail
release, without relying on the caller's current heap during deletion. Typed
owners retain the heap until their actual objects have been destroyed.

The MSL provider is compiled in the common module without the forced declaration
header, so its native numeric-format fallback calls libc. Game/showcase/test
translation units receive the declaration boundary before stdio and disable the
four corresponding compiler builtins. This preserves original null-string,
string precision/width and C-locale wide-string behavior. Numeric and host
extension formatting continues to use native-width host varargs.

Validation used the existing native macOS arm64 LLVM 23 debug configuration:

- The showcase and all 20 focused regression targets built successfully. The
  target lists and exit codes are in build-gates.json and extra-gates.json.
- All 20 binaries passed, including original heap/allocation, J3D animation,
  packets/matrices/materials/joints/geometry/textures, archive/JMap/BckCtrl,
  Xanime core/player, KCL resources and the actual camera runtime. Tests needing
  retail fixtures received SMGPC_REAL_DISC. Results are in test-gates.json and
  extra-gates.json.
- The final Base/Solid/Disposer/list source passed five groups normally and with
  ASan/UBSan after the provenance and formatting integration. Its source/archive
  hashes are in the companion original-jkr-base-20260903/native-evidence.json.
- Companion isolated checks cover seven allocation-domain groups normally,
  with ASan/UBSan and TSan; fourteen animation groups with the same modes;
  all 531 MarioAnime animations; and nine MSL groups with the same modes.
  Their source proofs and exact scope are in the respective notes folders.
- Fresh title and Gateway smokes both exited successfully. Each used a 600-frame
  cap and stopped at its existing success condition: two rendered title frames
  and five rendered Gateway frames. Gateway verified animated Mario submission,
  an on-screen actor center, GPU draw submission, probe gravity, and exact planet
  KCL contact. These bounded checks do not exercise a jump or a bunny chase.

Aurora c595d62 supplies OSReport/OSVReport/OSPanic/OSFatal and passes its five
diagnostic tests. It was pushed before recording the parent submodule pointer.
No Game gameplay bodies are changed by this integration.
