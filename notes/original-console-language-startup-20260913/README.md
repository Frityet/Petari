# Native console language at boot

The fourth production run submitted a real FileLoader request but stopped at FileRipper's original existence assertion for `/AudioRes/SMR.szs`. Root's debugger captured that valid, non-dangling filename and the actual loader worker/stationed heap. The authenticated Korean audio corpus stores its SMR resource under `KrKorean/AudioRes/SMR.szs`. Original SCGetLanguage falls back to English without IPL.LNG; original filename localization then attempts UsEnglish and falls back to the unavailable unprefixed path.

The native app now initializes a missing IPL.LNG before constructing GameSystem, using the mounted disc ID's regional country code. Japanese-region codes choose Japanese, Korean-region codes choose Korean, and North American/PAL codes choose English. Existing settings, including imported SYSCONF records, remain untouched. An unknown region or failed SC insertion throws rather than reporting an initialized setting. This sets the current native console configuration only; it does not overwrite NAND files.

The ownership separation follows Dolphin's boot policy in `dolphin/Source/Core/Core/BootManager.cpp` and regional language defaults in `ConfigManager.cpp::GetLanguageAdjustedForRegion`; country-code grouping is from `DiscIO/Enums.cpp::CountryCodeToRegion`. DVD path lookup and original SC accessors remain unchanged. This is a native boot policy, not recovered Game behavior. No Game source, SDK provider, resource path substitution or default audio names changed.

Only `src/app/OriginalGameApplication.cpp` changed. Required direct compilation passes (native-compile.json/log). No extra tests or GPU runs. Root owns the next production run, which will establish actual progress through the SMR load.
