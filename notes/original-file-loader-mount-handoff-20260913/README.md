# Original async archive request handoff

The fourteenth actual Gateway run crashed in FileHolder::getContext from StageFileLoader::waitLoadedStageFile: mountAsyncArchive had mounted bytes synchronously in a separate native cache, while the original receiveFile waited on FileLoader, which had never received the request.

Restored existing original mountArchive, mountAsyncArchive and object/layout request helper bodies. The real FileLoaderThread now reads each requested archive into its selected Game heap, publishes bounded native archive backing through its existing createAndAddArchive callback, and then signals its real FileHolder entry. receiveArchive waits that request before borrowing the published native archive; isMountedArchive remains a nonblocking query. No StageFileLoader/Game behavior was altered or missing entry fabricated.

Native archive conversion/backing is still supplied by ArchiveMountService; this checkpoint does not claim complete ResourceHolderManager/ArchiveHolder migration. No component tests were added. Production receipts live in ../gateway-wakeup-demo-20260912/.

Twenty-second production build passed. Fifteenth real-disc run completed StageFileLoader receive and reached StageDataHolder initialization; its next failure is an existing erroneous decompiled object-table filename.
