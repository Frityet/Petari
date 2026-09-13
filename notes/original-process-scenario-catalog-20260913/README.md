# Original process scenario accessor, 2026-09-13

The tenth production run passed pane-effect creation and failed at the native `ScenarioDataFunction::getScenarioDataParser` accessor. The original `GameSystemSceneController::initAfterStationedResourceLoaded` already constructs and initializes its actual `mScenarioParser` before initializing PlayTimerScene and ScenarioSelectScene. This was an accessor ownership mismatch, not missing authored data or incorrect startup order.

`src/compat/ScenarioDataParserAccess.cpp` now follows the original `GameSystem -> mSceneController -> mScenarioParser` expression when an actual GameSystem exists. It does not fall back to another catalog when that original field is null. Executions without GameSystem retain their existing explicitly owned standalone catalog route. No new parser, catalog publication, row, or scene state is created.

Reference: `decomp/src/Game/System/ScenarioDataParser.cpp`, `ScenarioDataFunction::getScenarioDataParser`; retail 0x803A9708–0x803A9714 loads exactly these fields. The native accessor compiles successfully (`native-compile.json` includes its source hash). No tests or production runs were performed by this agent; root owns the next build/run.
