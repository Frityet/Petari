#include <aurora/exception.hpp>
#include "runtime/ScenarioCatalogOwnership.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <stdexcept>

ScenarioDataParser* ScenarioDataFunction::getScenarioDataParser() {
    if (auto* system = SingletonHolder<GameSystem>::get()) {
        return system->mSceneController->mScenarioParser;
    }
    auto* catalog = smgpc::runtime::ScenarioCatalogOwnership::active();
    if (!catalog)
        aurora::throw_host_exception<std::logic_error>("The actual process scenario catalog is not published");
    return &catalog->parser();
}
