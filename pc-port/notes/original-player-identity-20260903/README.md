# Original player identity storage

MarioTask previously referenced the unnamed retail symbol at 0x806B6288. That byte belongs to MarioActor.cpp in the original split and is the existing `gIsLuigi` global initialized by MarioActor. The task now references that actual owner, as root MarioWalk already does. Native MarioTask and the remaining walking source use the same identity; no duplicate variable or fixed player-mode value is supplied.

`python3 pc-port/notes/original-player-identity-20260903/verify.py` compiles the previous and corrected MarioTask with the configured original compiler and compares all 240 bytes and all references of startHipDropBlur, canonicalizing only the old symbol name. They are identical. An empty extern-C block was removed. This is a symbol/compilation correction, not a change to jump or movement behavior.
