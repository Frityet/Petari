# Round20 integration

This batch deletes the standalone JKernel allocation provider/routing header, absorbs constexpr CP932 encoding into the existing text encoding module, moves formatted-string runtime implementation into Aurora MSL C, dismantles the forced Metrowerks prefix header, and prunes unused actor registry APIs.

Allocation no longer reaches through a compat routing interface: actual JKRHeap operators use Aurora routing state directly and authoritative root/MEM2 ranges. The full existing overload bodies remain unchanged. Formatter implementation uses concrete MSL entry points while its own numeric conversions keep host stdio; only original-client compilation receives the MSL string-format declaration boundary.

CP932 encoding retains the frozen 9,280-entry Windows-31J mapping and static array-lvalue literals; existing runtime text codecs are unchanged. Most Game edits are mechanical includes. The removed forced header's compiler/math/SDK declarations belong to canonical headers; missing Game type dependencies get explicit includes, and mixed-long integer clamp stays next to the actual Game overload.

Nine registry functions are deleted, including five unused production APIs and four fixture-only observers. An obsolete registry fixture asserted already-replaced mirror state and borrowed-name behavior contrary to the donor implementation, so its target/source is deleted. Retained fixtures use actual ownership and object retirement. No replacement fixture suite is introduced.

Validation is limited to the integrated app build and one fresh-save 120-frame Gateway opening smoke after all lanes freeze. No claim about full route completion or audio output is made.
