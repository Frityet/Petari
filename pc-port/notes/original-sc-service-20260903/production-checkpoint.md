# Regular native SC checkpoint

Applied the root enum-name compilation correction, full original SDK accessor
and product code, typed runtime service, and Aurora indexed SysConf operations.
The regular smg-pc-original-system-config-tests target builds with LLVM23 on
macOS ARM64 and passes all seven CPU groups in production-runtime.log.

This verifies the same real NAND/OS/SDK owner graph as the isolated fixture,
through the regular game and Aurora archives. No console settings are invented.
Original defaults, type checks, duplicate record indexing, failure semantics,
product-memory restoration and owner lifetime are covered.

The service is now callable but not yet installed by RuntimeContext. A separate
console NAND import/startup change will install it before screen/camera queries.
The complete original async SC/NAND state machine remains outside this checkpoint.

Aurora indexed mutation commit26e5972 is pushed on codex/macos-compat.
