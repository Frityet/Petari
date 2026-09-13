# Native ResFont backing allocation

The seventh real original-app run reaches GameSystemFontHolder::createFontFromFile after GameSystem::init, then aborts on a 0x8000-byte JKRExpHeap allocation inside BRFNT texture decoding. Stack evidence: notes/gateway-wakeup-demo-20260912/original-app-seventh-run.log. The decoded RGBA sheet vector is host-owned renderer backing, not an original Wii resource allocation.

Nw4rFontCompat now creates HostFontResourceState and its shared control block under Aurora HostAllocationScope in the Font constructor body. This leaves the Font object's original allocation itself unchanged. ResFont::SetResource also enters host allocation for the parsed font's shared owner/control block and complete parse transaction. The directly callable parse_brfnt_font boundary independently routes every decoded sheet, map, width table and diagnostic string allocation to host memory, covering all parser callers. Each scope restores the caller's allocation routing on success and exceptions. Original input bytes stay borrowed; no new resource copy, altered parser acceptance, heap growth or Game edits.

Both affected native TUs compile; no tests or full app build were run by this agent. Root owns the next real production run. Exact two-file source manifest and compile receipts are included. Sources frozen for integration; no commit yet.

Production integration: thirteenth smg-pc build passes. Eighth real-disc Metal run gets through font creation and next exits on the old LayoutActor pointing-target rejection. Thus the observed 0x8000 font texture allocation failure is cleared; no gameplay or complete startup claim. Build/run receipts are in notes/gateway-wakeup-demo-20260912/original-app-thirteenth-build.json and original-app-eighth-run.json.
