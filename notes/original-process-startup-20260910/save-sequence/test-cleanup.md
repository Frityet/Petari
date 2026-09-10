# Save test cleanup after original sequence activation

Removed the deleted SaveDataHandleSequenceCompat header from both debug fixtures. AuroraNativeTests only contained unused sequence includes, so its actual Aurora NAND storage and no-owner story-event assertions remain unchanged.

SaveDataCoreRealOrAbsentTests previously constructed the facade and asserted that its members stayed null and all state-machine methods threw. These assertions contradict the complete original constructor, which allocates its temporary save buffer and creates the real NoOperation nerve; original getters, idle draw, and state changes are valid methods rather than unsupported facades. Removed those obsolete calls instead of introducing a partial GameSystem or forcing original methods to fail.

The replacement fixture starts with no RuntimeContext and no original GameSystem singleton. It exercises the public GameDataFunction current-holder, backup-holder, user-name and system-configuration paths. Each must report the actual missing process save sequence at the ownership boundary, and the queries must leave both process and runtime absent. These are four dependency-boundary checks, not save/load completion coverage.

Remaining coverage requires actual process startup: SaveDataHandleSequence resource initialization, current/backup UserFile creation within that owner, NAND queue completion, banner persistence, confirmation/error UI nerves, and reset preparation. This fixture does not call or claim to validate those operations. Existing focused actual UserFile/chunk/PlayerStatus fixtures are separate proofs and do not establish full process save execution.

Only these two test source files changed. No build or test was run, as requested; parent owns integrated validation.
