# Localized archive handoff

The fifth production run selected console language 9 for the Korean disc, completed GameSystem::init and then failed opening `/LayoutData/WiiRemoteStrapReplace.arc`. Original makeLayoutArchiveFileName deliberately returns that logical path after isFileExist(path, true) verifies its localized form. The native ResourceHolderService bypassed the following original FileUtil localization step and opened the logical path directly.

Existing native archive mount, receive and heap lookup now apply original makeFileNameConsideringLanguage before reaching ArchiveMountService. The layout and texture resource factories do the same after layout/object filename selection, and mounted-layout retention resolves the same identity. Embedded raw mounts remain explicitly named and keep their existing owner. A null mount heap selects the original current heap, and real mount failures propagate. No filename-specific mapping, alternate resource, placeholder or successful failure state was added.

Only src/compat/FileUtilCompat.cpp and src/compat/ResourceHolderCompat.cpp changed. Both required native translation-unit compiles pass (native-compile.json). No tests or GPU runs. Root owns the next actual production run. The native archive service remains the existing owner; this is not a claim that its pending whole FileLoader/archive ownership migration is complete.
