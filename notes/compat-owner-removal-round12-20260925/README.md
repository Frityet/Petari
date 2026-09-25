# Message, talk and GPU callback owner cleanup

Removed 10 compat files, reducing the inventory from 76 to 66. Deleted the unused information-message binding; moved exact UTF-16 access into MessageData/MessageUtil; put debug owner checks in their actual camera/player callers; and gave the actual TalkDirector and DrawSyncManager their native borrower retirement operations. No renamed alternate owner or new fixture framework.

DrawSyncManager now deletes its own raw child storage after joining its worker. Original callback ranges, queue acknowledgement ordering and rollback remain intact. Talk selection/message/demo algorithms are unchanged; its actual owner scrubs borrowed actors/controllers and marks retirement. See lane notes for scope and limitations.

Validation was one app build and one fresh-save 120-frame Gateway opening run. Both passed, and the game exited cleanly. No focused suites or longer gameplay run. This does not prove rabbit catches or Rosalina spawning. Existing useful tests were mechanically adapted without running them.

Unrelated dirty source/tests and the six preexisting staged route-note changes are preserved. Owned manifests and before snapshots support isolated commit staging; validation describes the integrated working tree.
