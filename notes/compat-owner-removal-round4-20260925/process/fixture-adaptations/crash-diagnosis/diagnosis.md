# Confirmed standalone RuntimeContext owner failure

Built binary SHA-256 and exact real-disc/Metal environment are in launch JSON files. `lldb-backtrace.log` contains the complete all-thread stack; the debugger's exit code 0 indicates successful debugger operation, not test success. The target stopped with EXC_BAD_ACCESS address 0x40.

Main-thread chain: `FileLoader::getRequestFileInfoConst(this=null)` → `FileLoader::isNeedToLoad` → `requestMountArchive` → `MR::mountAsyncArchive` → `MR::mountArchive` → `MessageHolder::initGameData` → `MessageHolderOwnership` → `RuntimeContext` → the old successful-construction loop in the test. Requested file: `KrKorean/MessageData/Message.arc`.

The log first prints both retained failure checks as complete. Full standalone reconstruction is not valid: original message loading needs its original FileLoader owner. Do not add fallback loading or fake GameSystem ownership to repair this fixture.

Reviewed HEAD: 3b37917cdcc04ac7e2ce190cab9f9835fe2b95fa. Both HEAD and the dirty RuntimeContext constructor have this MessageHolder bootstrap. Preexisting working changes remove native scene-service wrappers; HEAD wrapper constructors only retain their runtime reference. The final renamed failure-only fixture does not depend on those removals and makes no scheduler-size assertion. Production RuntimeContext files were left untouched.
