# Original submodel file queries

Restored complete existing ModelUtil::isExistModel/isExistSubModel bodies in OriginalModelAccess.cpp. They format the original ObjectData archive path and call the ordinary DVD file service, preserving the original nonlocalized existence rule. Removed the PlanetMapRuntimeCompat substitute that required a development RuntimeContext and searched its custom object archive catalog. Every PlanetMap model initialization can now perform its ordinary optional-model probes under the actual original process.

This change does not pretend optional water, indirect, bloom or LOD creators are all complete; the remaining explicit unsupported creation paths stay visible. No new reference recovery, Game edit, archive substitution or component test. Included in the next production build.
