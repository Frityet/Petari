# Remove dead registry contracts

Deleted five unused APIs: generated NameObj name retention, blanket registration-suffix deletion, actor resource lookup, and two alternate Binder setup/release helpers. Their only definitions had no live callers. Also removed four fixture-only introspection APIs (raw ownership tag, Binder presence/config, LodCtrl counter), the duplicate Binder configuration record and generated-name storage.

Production continues to own the actual Binder. Existing retained fixtures use the live claimed-ownership query and actual object retirement; they no longer require unused counters or mirrored Binder parameters. The obsolete ActorRuntimeRegistryTests target/file was deleted: it asserted copying NameObj names and immediate suspend flags, and referenced several already-removed model/clipping shims, all contrary to the restored original implementations.

No fixture tests were run or added. This is preparatory deletion, not completion of the remaining actor/NameObj owner migration.
