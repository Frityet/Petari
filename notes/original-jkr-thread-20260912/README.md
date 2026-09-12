# Original JKRThread on native SDK threads — 2026-09-12

The full emitted JKRThread reference source now supplies thread construction, queued/jammed messages, entry dispatch, intrusive thread-list lookup and destruction. The header follows the reference; its source-emitted TColor assignment declaration is restored. The sole native source correction sizes message storage with `sizeof(OSMessage)` instead of four bytes, preserving complete native pointers.

The reference compiles with the original compiler and all emitted sections match the retail object at 100%. It is unchanged in decomp. The existing-OSThread constructor has no emitted retail symbol or current caller, so this change does not invent that declared-only method.

The original-thread executable builds and runs successfully. It verifies three complete heap lifetimes, actual worker allocations in the requested JKR heap, native pointer delivery and queue order, completed-thread retirement, cancellation of a blocked worker, destructor unwinding before storage release, cancellation before first resume, and removal from the original thread list. Every child allocation returns to the root heap. The initial run and the rerun with Aurora priority scheduling both pass; exact binary hashes are in the receipts.

This proves the original emitted JKR thread cohort. Complete original GameSystem initialization and Gateway progression remain under development.
