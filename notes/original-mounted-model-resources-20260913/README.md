# Original mounted model-resource handoff

The twenty-second original application run reached normal actor placement after Mario initialization and stopped while creating Air's model. `ModelManager::init` requested the already mounted `SphereAir.arc`, but `ResourceHolderService` bypassed that mount and synchronously reread the DVD archive. The original command-generation scope had scheduling disabled; a blocking DVD read is invalid there. The scheduling rejection is preserved.

`ResourceArchiveOwner` now retains the published `MountedArchive`, uses that exact `JKRMemArchive`, and allocates the original holder and typed model backing on the archive's original heap. It no longer creates another archive object or rereads a separate DVD cache. If a mount is absent, the service follows original `createAndAddInner`'s receive/mount path. The original manager's unused explicit heap parameter remains unchanged.

The mount already owns bounded JMap registrations, so the resource holder's duplicate registrations were removed. Converted BTI/CANM cache entries are restored before their native backing retires, including failed construction. All owner metadata remains host allocated. This is a bounded handoff correction; full original ResourceHolderManager main-thread dispatch remains a separate existing frontier.

Evidence: `../gateway-wakeup-demo-20260912/original-app-twenty-second-{run,backtrace}.log`; the thirty-second production build passed, and the twenty-third real run passed the archive/scheduler boundary before reaching a separate CollisionParts scene-owner prerequisite. No component test campaign was run.
