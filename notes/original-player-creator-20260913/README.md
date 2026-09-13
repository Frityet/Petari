# Original player creation from stage placement

The seventeenth actual original Gateway run passes pause-menu initialization and reaches StageDataHolder::initPlacementMario. LLDB confirms a valid authored StartObj row (index 0), object name Mario, and null NameObjFactory creator. The native creator table omitted both original Mario and MarioActor rows.

Restored both exact original rows to the shared native factory: each constructs the actual MarioActor and has no per-row archive override. The original Game stage loader still chooses the authored start row, sets placement zone, invokes the creator and initializes the actor. No replacement player spawn, start-ID override or Game edit was added.

The existing full MarioActor source supplies construction/initialization. Build/run receipts live in ../gateway-wakeup-demo-20260912; this registration alone does not prove player initialization or rendering is complete.

The twenty-seventh production build passes. The eighteenth actual run now reaches MarioActor construction and initialization; its next failure is a ModelManager allocation in the selected native heap domain. This confirms the authored player-creation handoff, not completed Mario initialization or gameplay.
