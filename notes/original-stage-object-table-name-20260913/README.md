# Original stage object-table path correction

The fifteenth actual Gateway run passed stage archive loading and then crashed in StageDataHolder::initTableData because its old decompilation requested `/StageData/ObjTableTable.arc`.

Retail initTableData at 0x803483E0 passes label 0x805D26AA to MR::receiveArchive. That label contains `/StageData/ObjNameTable.arc`, matching the actual stationed-file table. Corrected the typo in decomp first, compiled the complete source with the original Metrowerks toolchain, then copied exactly that literal correction into native. No file alias or missing-archive fallback was added.

The compile receipt and exact extracted assembly/label are adjacent. The production crash stack is ../gateway-wakeup-demo-20260912/original-app-fifteenth-backtrace.log.

Twenty-third production compilation passed; activation of the actual unique planet creator exposed further original MapParts helper link dependencies. Restored the already decompiled MR::sign and MR::makeMtxFrontNoSupport into the existing shared native math providers without changing the recovered bodies. Other agents own the remaining original lifecycle/model/mirror links.

The twenty-fifth production build links all current original planet/map-part/mirror dependencies. Its sixteenth actual run passes the corrected object-table load and reaches original PauseMenu initialization; the next error is native pane-animation layer ownership. This is stage-loading progress, not a completed stage or bunny-demo claim.
