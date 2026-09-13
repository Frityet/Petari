# Original NPC, screen, shadow and archive cleanup

Removed the separate screen-dimension and unused player-override compatibility files. Original JUTVideo getters now supply screen/EFB dimensions. Original NPC resource/goods, pose, floating motion and cutscene-fade wrappers replace host substitutes, including a reversed Tico float direction and forced 60-frame fade. Two complete original elliptical shadow drawers use the common owner; original axis helpers and live drawer offset/cut setters replace approximations and stale metadata updates.

Archive mount/receive/retention and layout/standalone-texture creation now resolve the original language path before going through the existing retained archive owner. This fixes the strap-screen resource handoff under the original process without any resource-name substitutions.

The eleventh full production build passes. Sixth real-disc Metal run completes GameSystem::init and enters the frame loop; it passes the prior sound and strap archive failures, then exits on ResourceHolder lookup of ObjectData/SaveIconBanner.arc. That next path-resolution boundary is still being implemented. No Logo completion, Gateway wakeup, bunny capture or Rosalina appearance is claimed.

No new tests were added and no broad suite was run. Two screen accessors match their four retail instructions 100%; per-cohort reference/compiler evidence is retained in the linked task note directories. Existing tests only lost constructors that wrote unread override state; assertions are unchanged. Build receipt includes the user-owned compile/header changes preserved in the working tree.

Reference commits bce4e6089 and 9ce434dfe were published before updating this gitlink. The user's staged documentation deletions and other unrelated files remain untouched.
