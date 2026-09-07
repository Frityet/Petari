#include <aurora/exception.hpp>
#include "compat/GameDataHolderCompat.hpp"
#include "compat/PlayerStatusStorage.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/GameEventFlag.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/UserFile.hpp"
#include "compat/GameDataRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"

namespace {
struct HolderState {
    s32 power_star_num = 0;
    smgpc::compat::PlayerStatusStorage player_status;
    std::array<u16, 16> star_piece_alms{};
    std::map<std::string, bool> event_flags;
    std::map<std::string, u16> event_values;
    bool power_star_data_complete = true;
    mutable bool galaxies_initialized = false;
    mutable std::map<std::string, GameDataSomeGalaxyStorage> galaxies;
};

std::map<const GameDataHolder*, HolderState> sHolderStates;

[[noreturn]] void unavailable(std::string_view operation) {
    aurora::throw_host_exception<std::logic_error>("GameDataHolder operation is unavailable without retail backing data: " +
                           std::string(operation));
}

HolderState& require_state(GameDataHolder& holder) {
    const auto found = sHolderStates.find(&holder);
    if (found == sHolderStates.end()) {
        unavailable("holder state");
    }
    return found->second;
}

const HolderState& require_state(const GameDataHolder& holder) {
    const auto found = sHolderStates.find(&holder);
    if (found == sHolderStates.end()) {
        unavailable("holder state");
    }
    return found->second;
}

void initialize_galaxies(const HolderState& state) {
    if (!state.power_star_data_complete) {
        unavailable("per-galaxy Power Star ownership from aggregate-only save data");
    }
    if (state.galaxies_initialized) {
        return;
    }

    smgpc::compat::JkrHostAllocationScope host;
    auto* parser = ScenarioDataFunction::getScenarioDataParser();
    auto galaxies = std::map<std::string, GameDataSomeGalaxyStorage>{};
    for (s32 index = 0; index < parser->mScenarioData.size(); ++index) {
        const auto accessor = GalaxyStatusAccessor(parser->getScenarioData(index));
        const auto count = accessor.getPowerStarNum();
        if (count < 0 || count > 8) {
            aurora::throw_host_exception<std::out_of_range>("Galaxy Power Star count exceeds the original eight-bit storage");
        }
        if (count == 0) {
            continue;
        }
        auto [entry, inserted] = galaxies.try_emplace(accessor.getName(), accessor);
        if (!inserted) {
            aurora::throw_host_exception<std::logic_error>("The actual scenario catalog contains a duplicate galaxy name");
        }
        // The holder can outlive the process catalog and every scene heap.
        entry->second.mGalaxyName = entry->first.c_str();
    }
    state.galaxies.swap(galaxies);
    state.galaxies_initialized = true;
}

GameDataSomeGalaxyStorage& require_galaxy(const GameDataHolder& holder, const char* name) {
    if (name == nullptr || *name == '\0') {
        aurora::throw_host_exception<std::invalid_argument>("Power Star storage requires a galaxy name");
    }
    const auto& state = require_state(holder);
    initialize_galaxies(state);
    const auto entry = state.galaxies.find(name);
    if (entry == state.galaxies.end()) {
        aurora::throw_host_exception<std::invalid_argument>("Galaxy has no authored Power Star storage: " + std::string(name));
    }
    return entry->second;
}

void require_scenario_bit(s32 scenario_num) {
    if (scenario_num < 1 || scenario_num > 8) {
        aurora::throw_host_exception<std::out_of_range>("Power Star scenario is outside the original eight-bit storage");
    }
}

bool can_turn_on(const GameDataHolder& holder, const GameEventFlag& flag, unsigned depth) {
    if (depth > 32U) {
        aurora::throw_host_exception<std::logic_error>("Retail game event dependency graph exceeded its recursion bound");
    }
    switch (flag.mType) {
    case GameEventFlag::Type_0:
        return true;
    case GameEventFlag::Type_1:
        return holder.calcCurrentPowerStarNum() >= flag.mStarNum;
    case GameEventFlag::Type_SpecialStar:
        if (holder.calcCurrentPowerStarNum() == 0) {
            return false;
        }
        return holder.hasPowerStar(flag.mGalaxyName, flag.mStarID);
    case GameEventFlag::Type_4:
        return (flag.mRequirement1 == nullptr || holder.isOnGameEventFlag(flag.mRequirement1)) &&
               (flag.mRequirement2 == nullptr || holder.isOnGameEventFlag(flag.mRequirement2));
    case GameEventFlag::Type_5:
        if (flag.mEventValueName == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Retail story-dependent flag has no story event");
        }
        return holder.isPassedStoryEvent(flag.mEventValueName);
    case GameEventFlag::Type_EventValueIsZero:
        if (flag.mRequirement == nullptr || flag.mEventValueName == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Retail event-value flag has incomplete requirements");
        }
        return holder.isOnGameEventFlag(flag.mRequirement) && holder.getGameEventValue(flag.mEventValueName) == 0;
    case GameEventFlag::Type_10:
        return holder.isCompleteMarioAndLuigi();
    case GameEventFlag::Type_11:
        if (flag.mEventValueName == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Retail synchronized flag has no requirement");
        }
        return holder.isOnGameEventFlag(flag.mEventValueName);
    case GameEventFlag::Type_GalaxyOpenStar:
    case GameEventFlag::Type_Galaxy:
    case GameEventFlag::Type_Comet:
    case GameEventFlag::Type_StarPiece:
    default:
        unavailable("event predicate for " + std::string(flag.mName));
    }
}

}  // namespace

GameDataHolder::GameDataHolder(const UserFile* user_file)
    : mEventFlagChecker(nullptr), mEventValueChecker(nullptr), mPlayerStatus(nullptr), mAllGalaxyStorage(nullptr),
      mSpinDriverPathStorage(nullptr), mStarPieceAlmsStorage(nullptr), mMapInfo(nullptr), mScenarioProgressTestRun(nullptr),
      mChunkHolder(nullptr), mName{}, mUserFile(user_file) {
    smgpc::compat::JkrHostAllocationScope host;
    auto& state = sHolderStates[this];
    state = HolderState{};
    mPlayerStatus = &state.player_status;
    std::snprintf(mName, sizeof(mName), "mario1");
}

bool GameDataHolder::isDataMario() const {
    return std::strstr(mName, "mario") != nullptr;
}

bool GameDataHolder::canOnGameEventFlag(const char* name) const {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Game event flag query requires a name");
    }
    return can_turn_on(*this, smgpc::compat::game_data::require_retail_flag(name), 0U);
}

bool GameDataHolder::isOnGameEventFlag(const char* name) const {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Game event flag query requires a name");
    }
    const auto& flag = smgpc::compat::game_data::require_retail_flag(name);
    if ((flag.mSaveFlag & 0x1U) != 0U) {
        return can_turn_on(*this, flag, 0U);
    }
    const auto& flags = require_state(*this).event_flags;
    const auto found = flags.find(flag.mName);
    return found != flags.end() && found->second;
}

void GameDataHolder::tryOnGameEventFlag(const char* name) {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Game event flag write requires a name");
    }
    const auto& flag = smgpc::compat::game_data::require_retail_flag(name);
    if (!can_turn_on(*this, flag, 0U) || (flag.mSaveFlag & 0x1U) != 0U) {
        return;
    }
    switch (flag.mType) {
    case GameEventFlag::Type_0:
    case GameEventFlag::Type_1:
    case GameEventFlag::Type_GalaxyOpenStar:
    case GameEventFlag::Type_4:
    case GameEventFlag::Type_Galaxy:
    case GameEventFlag::Type_EventValueIsZero:
        require_state(*this).event_flags[flag.mName] = true;
        return;
    default:
        return;
    }
}

s32 GameDataHolder::getGameEventValue(const char* name) const {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Game event value query requires a name");
    }
    const auto& entry = smgpc::compat::game_data::require_retail_event_value(name);
    const auto& values = require_state(*this).event_values;
    const auto found = values.find(std::string(entry.name));
    return found != values.end() ? found->second : entry.default_value;
}

void GameDataHolder::setGameEventValue(const char* name, u16 value) {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Game event value write requires a name");
    }
    const auto& entry = smgpc::compat::game_data::require_retail_event_value(name);
    require_state(*this).event_values[std::string(entry.name)] = value;
}

bool GameDataHolder::isOnGameEventValueForBit(const char* name, int bit) const {
    if (bit < 0 || bit >= 16) {
        aurora::throw_host_exception<std::out_of_range>("Game event value bit is outside [0, 15]");
    }
    return (static_cast<u16>(getGameEventValue(name)) & static_cast<u16>(1U << bit)) != 0U;
}

void GameDataHolder::setGameEventValueForBit(const char* name, int bit, bool is_on) {
    if (bit < 0 || bit >= 16) {
        aurora::throw_host_exception<std::out_of_range>("Game event value bit is outside [0, 15]");
    }
    auto value = static_cast<u16>(getGameEventValue(name));
    const auto mask = static_cast<u16>(1U << bit);
    value = static_cast<u16>(mask & ~value);
    if (is_on) {
        value = static_cast<u16>(value | mask);
    }
    setGameEventValue(name, value);
}

s32 GameDataHolder::getPictureBookChapterCanRead() const {
    auto count = s32{};
    for (const auto suffix : std::string_view("ABCDEFGHI")) {
        const auto name = std::string("PictureBook") + suffix;
        if (!isOnGameEventFlag(name.c_str())) {
            break;
        }
        ++count;
    }
    return count;
}

s32 GameDataHolder::getPictureBookChapterAlreadyRead() const {
    return getGameEventValue("絵本既読章");
}

void GameDataHolder::setPictureBookChapterAlreadyRead(int value) {
    if (value < 0 || value > 9) {
        aurora::throw_host_exception<std::out_of_range>("Picture-book chapter is outside the retail chapter range");
    }
    setGameEventValue("絵本既読章", static_cast<u16>(value));
}

void GameDataHolder::addMissPoint(int points) {
    const auto value = static_cast<u32>(getGameEventValue("MissPointForLetter"));
    // Retail adds in u32, then clamps the wrapped result as signed PPC long.
    const auto next = std::clamp(static_cast<s32>(value + static_cast<u32>(points)), 0, 20);
    setGameEventValue("MissPointForLetter", static_cast<u16>(next));
}

void GameDataHolder::incPlayerMissNum() {
    setGameEventValue("MissNum", static_cast<u16>(std::min(getPlayerMissNum() + 1, 9999)));
}

s32 GameDataHolder::getPlayerMissNum() const {
    return std::clamp(getGameEventValue("MissNum"), 0, 9999);
}

bool GameDataHolder::hasPowerStar(const char* galaxy_name, s32 scenario_num) const {
    return makeGalaxyScenarioAccessor(galaxy_name, scenario_num).hasPowerStar();
}

void GameDataHolder::setPowerStar(const char* galaxy_name, s32 scenario_num, bool owned) {
    makeGalaxyScenarioAccessor(galaxy_name, scenario_num).setPowerStarFlag(owned);
}

s32 GameDataHolder::getPowerStarNumOwned(const char* galaxy_name) const {
    return require_galaxy(*this, galaxy_name).getPowerStarNumOwned();
}

GameDataSomeScenarioAccessor GameDataHolder::makeGalaxyScenarioAccessor(const char* galaxy_name, s32 scenario_num) {
    return static_cast<const GameDataHolder&>(*this).makeGalaxyScenarioAccessor(galaxy_name, scenario_num);
}

GameDataSomeScenarioAccessor GameDataHolder::makeGalaxyScenarioAccessor(const char* galaxy_name, s32 scenario_num) const {
    require_scenario_bit(scenario_num);
    return GameDataSomeScenarioAccessor(&require_galaxy(*this, galaxy_name), scenario_num);
}

bool GameDataHolder::hasGrandStar(int index) const {
    char name[32];
    std::snprintf(name, sizeof(name), "SpecialStarGrand%1d", index);
    return isOnGameEventFlag(name);
}

s32 GameDataHolder::calcCurrentPowerStarNum() const {
    const auto& state = require_state(*this);
    if (!state.power_star_data_complete) {
        return state.power_star_num;
    }
    s32 count = 0;
    for (const auto& [name, storage] : state.galaxies) {
        count += storage.getPowerStarNumOwned();
    }
    return count;
}

s32 GameDataHolder::getPlayerLeft() const {
    return mPlayerStatus->getPlayerLeft();
}

void GameDataHolder::addPlayerLeft(int value) {
    mPlayerStatus->addPlayerLeft(value);
}

bool GameDataHolder::isPlayerLeftSupply() const {
    return mPlayerStatus->isPlayerLeftSupply();
}

void GameDataHolder::offPlayerLeftSupply() {
    mPlayerStatus->offPlayerLeftSupply();
}

s32 GameDataHolder::getStockedStarPieceNum() const {
    return mPlayerStatus->mStockedStarPiece;
}

void GameDataHolder::addStockedStarPiece(int value) {
    mPlayerStatus->addStockedStarPiece(value);
}

bool GameDataHolder::isCompleteMarioAndLuigi() const {
    if (mUserFile == nullptr) {
        unavailable("owning user file");
    }
    return mUserFile->isOnCompleteEndingMarioAndLuigi();
}

bool GameDataHolder::isPassedStoryEvent(const char* name) const {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Story event query requires a name");
    }
    const auto& event = smgpc::compat::game_data::require_retail_story_event(name);
    return mPlayerStatus->mStoryProgress >= event.progress;
}

void GameDataHolder::followStoryEventByName(const char* name) {
    if (name == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("Story event write requires a name");
    }
    mPlayerStatus->mStoryProgress = smgpc::compat::game_data::require_retail_story_event(name).progress;
}

void GameDataHolder::resetAllData() {
    smgpc::compat::JkrHostAllocationScope host;
    auto& state = require_state(*this);
    auto reset = HolderState{};
    reset.galaxies = std::move(state.galaxies);
    reset.galaxies_initialized = state.galaxies_initialized;
    for (auto& [name, storage] : reset.galaxies) {
        storage.resetAllData();
    }
    state = std::move(reset);
}

u32 GameDataHolder::makeFileBinary(u8* buffer, u32 size) {
    static_cast<void>(buffer);
    static_cast<void>(size);
    unavailable("retail BinaryDataChunkHolder serialization");
}

bool GameDataHolder::loadFromFileBinary(const char* name, const u8* buffer, u32 size) {
    static_cast<void>(name);
    static_cast<void>(buffer);
    static_cast<void>(size);
    unavailable("retail BinaryDataChunkHolder deserialization");
}

namespace smgpc::compat::game_data {
std::size_t holder_state_count() noexcept {
    return sHolderStates.size();
}

void destroy_holder_state(GameDataHolder& holder) {
    JkrHostAllocationScope host;
    sHolderStates.erase(&holder);
    holder.mPlayerStatus = nullptr;
}

void copy_holder_state(GameDataHolder& destination, const GameDataHolder& source) {
    JkrHostAllocationScope host;
    auto copy = require_state(source);
    for (auto& [name, storage] : copy.galaxies) {
        storage.mGalaxyName = name.c_str();
    }
    require_state(destination) = std::move(copy);
    std::memcpy(destination.mName, source.mName, sizeof(destination.mName));
}

void set_holder_name(GameDataHolder& holder, const char* name) {
    if (name == nullptr || *name == '\0') {
        aurora::throw_host_exception<std::invalid_argument>("Game data name must not be empty");
    }
    std::snprintf(holder.mName, sizeof(holder.mName), "%s", name);
}

void set_holder_save_counts(GameDataHolder& holder, s32 power_star_num, s32 star_piece_num, s32 player_miss_num) {
    if (power_star_num < 0 || star_piece_num < 0 || player_miss_num < 0) {
        aurora::throw_host_exception<std::invalid_argument>("Game data counts must not be negative");
    }
    JkrHostAllocationScope host;
    auto& state = require_state(holder);
    state.power_star_num = power_star_num;
    // Aggregate-only imports carry no information about which stars were won.
    // A zero total does prove that every ownership bit is clear.
    state.power_star_data_complete = power_star_num == 0;
    state.galaxies.clear();
    state.galaxies_initialized = false;
    holder.mPlayerStatus->mStockedStarPiece = star_piece_num;
    holder.setGameEventValue("MissNum", static_cast<u16>(std::min(player_miss_num, 9999)));
}

void set_holder_ending_flags(GameDataHolder& holder, bool view_normal_ending, bool view_complete_ending,
                             bool final_challenge_star) {
    if (view_normal_ending || final_challenge_star) {
        unavailable("derived ending flags without per-galaxy Power Star storage");
    }
    const auto& stored_flag = require_retail_flag("ViewCompleteEnding");
    if ((stored_flag.mSaveFlag & 0x1U) != 0U) {
        aurora::throw_host_exception<std::logic_error>("Retail ViewCompleteEnding unexpectedly became a derived flag");
    }
    require_state(holder).event_flags[stored_flag.mName] = view_complete_ending;
}

void set_holder_event_state(GameDataHolder& holder, const std::map<std::string, bool>& flags,
                            const std::map<std::string, u16>& values) {
    auto checked_flags = std::map<std::string, bool>{};
    for (const auto& [name, value] : flags) {
        const auto& flag = require_retail_flag(name);
        if ((flag.mSaveFlag & 0x1U) != 0U) {
            aurora::throw_host_exception<std::invalid_argument>(name + " is derived and cannot be stored as a game event flag");
        }
        checked_flags[name] = value;
    }
    auto checked_values = std::map<std::string, u16>{};
    for (const auto& [name, value] : values) {
        static_cast<void>(require_retail_event_value(name));
        checked_values[name] = value;
    }
    auto& state = require_state(holder);
    state.event_flags = std::move(checked_flags);
    state.event_values = std::move(checked_values);
}

const std::map<std::string, bool>& holder_event_flags(const GameDataHolder& holder) {
    return require_state(holder).event_flags;
}

const std::map<std::string, u16>& holder_event_values(const GameDataHolder& holder) {
    return require_state(holder).event_values;
}

u8 holder_story_progress(const GameDataHolder& holder) {
    return holder.mPlayerStatus->mStoryProgress;
}

void set_holder_story_progress(GameDataHolder& holder, u8 progress) {
    if (progress > 60U) {
        aurora::throw_host_exception<std::invalid_argument>("Story progress is outside the retail StoryEvent BCSV range");
    }
    holder.mPlayerStatus->mStoryProgress = progress;
}
}  // namespace smgpc::compat::game_data
