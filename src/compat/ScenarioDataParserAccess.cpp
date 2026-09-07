#include "runtime/ScenarioCatalogOwnership.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include <stdexcept>

ScenarioDataParser* ScenarioDataFunction::getScenarioDataParser() {
    auto* catalog = smgpc::runtime::ScenarioCatalogOwnership::active();
    if (!catalog)
        throw std::logic_error("The actual process scenario catalog is not published");
    return &catalog->parser();
}
