# Explicit NAND storage ownership — 2026-09-12

NAND SDK operations now use an explicitly retained SaveDataService binding instead of reaching through RuntimeContext and its unrelated Game children. RuntimeContext installs that same binding beside its system configuration service. Original process startup can therefore supply storage before constructing GameSystem, without a fabricated RuntimeContext or alternate storage implementation.

The existing file descriptors identify their actual SaveDataService. Binding retirement invalidates its descriptors before releasing the borrowed service, overlapping owners are rejected, and a new owner cannot inherit a previous owner's handles or files. Real storage still requires a configured persistence directory; missing ownership and missing persistence do not report successful writes.

The complete original system-config executable builds and runs successfully. Added standalone NAND coverage verifies exact binary create/write/read, ownership overlap rejection without disturbing the active descriptor, descriptor retirement, isolated owner generations, and preservation of caller-owned NAND data after binding destruction. Existing configuration, region/language, array/byte-order, malformed-source and mapped-memory restoration checks also pass. Exact build/run results and binary SHA are in the evidence bundle.

The two RuntimeContext changes publish and retain this binding; they do not establish successful original GameSystem startup, which remains a separate diagnostic frontier.
