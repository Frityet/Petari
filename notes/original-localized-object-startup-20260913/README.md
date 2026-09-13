# Localized object archive ownership

The sixth production run passed the strap layout path and stopped on ResourceHolderService's lookup of `ObjectData/SaveIconBanner.arc`. Its object factory still used direct unlocalized VFS candidates even though original ResourceHolderManager uses MR::makeObjectArchiveFileName followed by the original language-aware file handoff.

The factory now uses those existing original helpers for object/map-parts/qualified-path selection and localization, then retains and caches the resolved archive through its existing owner. There is no resource-specific mapping or alternate archive. Only ResourceHolderCompat.cpp::create_and_add changed; previous staged localized-layout changes remain untouched.

Required native compilation passes. No additional tests or GPU runs; root owns the next production run. This fixes resource filename handoff without claiming completion of the separate original archive-owner migration.
