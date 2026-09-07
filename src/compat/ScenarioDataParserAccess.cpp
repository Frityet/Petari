#include <aurora/exception.hpp>
#include "runtime/ScenarioCatalogOwnership.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include <stdexcept>

ScenarioDataParser* ScenarioDataFunction::getScenarioDataParser() {
    auto* catalog = smgpc::runtime::ScenarioCatalogOwnership::active();
    if (!catalog)
        aurora::throw_host_exception<std::logic_error>("The actual process scenario catalog is not published");
    return &catalog->parser();
}
