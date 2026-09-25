# CSV ownership closure

Scanned current `src/Game` creation sites for `createCsvParser`, `tryCreateCsvParser`, `createSheetParser`, and heap-created `JMapInfo`. The following original owners discarded parser pointers while retaining parsed record strings, or left child parsers until a later heap-disposal phase:

| Actual owner | Retirement |
| --- | --- |
| ScenarioData / ScenarioDataParser | Delete both scenario/zone parsers and each scenario child before FileLoader |
| ParticleResourceHolder | Delete automatic/simple effect parsers, particle records and JPA manager |
| StageDataHolder | Delete parent-owned local stages and object-name table; array members retire table parsers |
| ActorAnimKeeper | Parser member after info array; constructor cleans partial array |
| ActorPadAndCameraCtrl | Parser member after info array; constructor cleans partial array |
| LightDataHolder | Parser member retained through record lifetime |
| LightZoneInfo | Per-zone parser member retained through record lifetime |
| DemoTimeKeeper / DemoSubPartKeeper / DemoPlayerKeeper | Parser members and canonical keeper array cleanup |
| DemoCameraKeeper / DemoActionKeeper | Parser members and canonical keeper array cleanup |
| DemoWipeKeeper / DemoSoundKeeper | Parser members; original AssignableArray members retain record cleanup |
| DemoPadRumbler / MoviePlayingSequence | Actual movie parent deletes rumbler; rumbler deletes entries and parser |
| FixedPosition | Local unique parser; constructor copies all needed values |

LiveActor's camera-helper failure catch deletes/nulls its animation keeper before registry adoption, avoiding a lease leak during partial setup. Existing actor registry ownership is unchanged on success. Demo child ownership now follows actual DemoExecutor destructors, and the obsolete DemoDirectorOwnership sidecar is deleted.

BckCtrl uses a stack parser whose shared decoded strings remain owned by actual ArchiveHolder cache registration. Existing parser-member owners outside this list were left in place. The manifest and scoped delta list every source/header changed in this lane; no blanket release of archive borrowers is used.
