#include "resource/TextEncoding.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/UserFile.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/GameDataOwnership.hpp"
#include "compat/GameDataSession.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/ScenarioCatalogOwnership.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }
namespace {
void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string(message));
}

template <typename Exception = std::exception, typename F>
void require_throws(F&& operation, std::string_view message) {
    bool threw = false;
    try { operation(); } catch (const Exception&) { threw = true; }
    require(threw, message);
}

void require_absent_bindings() {
    require_throws<std::logic_error>([] { GameDataFunction::getCurrentGameDataHolder(); },
                                    "current holder query requires actual selected-file ownership");
    require_throws<std::logic_error>([] { GameDataFunction::getSceneStartGameDataHolder(); },
                                    "scene-start holder query requires actual backup ownership");
    require_throws<std::logic_error>([] { GameDataFunction::getPictureBookChapterCanRead(); },
                                    "global picture-book state requires actual holder backing");
    require_throws<std::logic_error>([] { GameDataFunction::onGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str()); },
                                    "global event writes must not create a synthetic file");
}
}

int main() {
    try {
        require_absent_bindings();
        require_throws<std::logic_error>([] { GameDataFunction::getUserName(); },
                                        "user-name routing requires the actual save-sequence owner");
        require_throws<std::logic_error>([] { GameDataFunction::getSysConfigFileTimeAnnounced(); },
                                        "system configuration requires its actual save-sequence owner");

        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "actual UserFile construction requires the authored scenario catalog");
        struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
        DVDInit();
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime resources;
        smgpc::runtime::DvdFileSystemService dvd({});
        smgpc::runtime::ArchiveMountService mounts(dvd);
        auto catalog = std::make_shared<smgpc::runtime::ScenarioCatalogOwnership>(
            resources.host_heaps(), resources.budget().scenario_catalog_bytes, mounts);
        smgpc::compat::game_data::initialize_event_table();
        const auto free_before = resources.host_heaps()->root_heap().getTotalFreeSize();

        for (u16 selected_file = 1; selected_file <= 6; ++selected_file) {
            {
                smgpc::compat::GameDataSession session(selected_file, resources, catalog);
                auto& file = session.user_file();
                auto& current = session.holder();
                const auto& backup = session.scene_start_holder();
                const auto expected_name = std::string("mario") + std::to_string(selected_file);
                require(file.mGameDataHolder == &current && file.mConfigDataHolder != nullptr &&
                            current.mUserFile == &file && backup.mUserFile != &file && backup.mUserFile != nullptr,
                        "session must own two real UserFiles and their associated original holders");
                require(session.selected_file() == selected_file &&
                            std::string_view(file.getGameDataName()) == expected_name &&
                            std::string_view(backup.mName) == expected_name,
                        "each selected slot must retain its exact current and backup marioN identity");
                require(GameDataFunction::getCurrentGameDataHolder() == &current &&
                            GameDataFunction::getSceneStartGameDataHolder() == &backup && &current != &backup,
                        "current and scene-start bindings must address separate original holders");
                require(smgpc::compat::game_data::holder_story_progress(current) == 0 &&
                            smgpc::compat::game_data::holder_story_progress(backup) == 0 &&
                            current.isPassedStoryEvent(smgpc::resource::encode_cp932("ゲーム開始直後").c_str()) &&
                            !current.isPassedStoryEvent(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str()) && !current.isPassedStoryEvent(smgpc::resource::encode_cp932("スピン権利").c_str()),
                        "fresh files must preserve original progress zero, without a seeded demo checkpoint");

                // Original setUserName copies the entire eleven-element buffer.
                constexpr wchar_t name[11] = L"Rosalina";
                file.setUserName(name);
                require(std::wstring_view(file.mUserName) == L"Rosalina", "actual UserFile must retain its user name");
                require_throws<std::logic_error>([] { GameDataFunction::getUserName(); },
                                                "a holder binding must not pretend a SaveDataHandleSequence exists");
                require(!current.isOnGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str()), "fresh original flag storage must be clear");
                current.tryOnGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str());
                require(current.isOnGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str()) && !backup.isOnGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str()),
                        "actual Type_0 flag writes must leave the independent backup unchanged");
                current.setPictureBookChapterAlreadyRead(3);
                require(current.getPictureBookChapterAlreadyRead() == 3 && backup.getPictureBookChapterAlreadyRead() == 0,
                        "actual VLE1 picture-book values must be isolated between holders");

                // The bounded demo checkpoint is an explicit fixture action.
                GameDataFunction::followStoryEventByName(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str());
                require(smgpc::compat::game_data::holder_story_progress(current) == 5 &&
                            !backup.isPassedStoryEvent(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str()),
                        "original event lookup must explicitly advance only the current holder to progress five");
                current.addPlayerLeft(20);
                current.addStockedStarPiece(456);
                session.store_scene_start();
                require(smgpc::compat::game_data::holder_story_progress(backup) == 5 &&
                            backup.getPictureBookChapterAlreadyRead() == 3 && backup.isOnGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str()),
                        "store_scene_start must serialize actual story, value and flag chunks into the backup");
                require(current.getPlayerLeft() == 24 && backup.getPlayerLeft() == 4 && backup.getStockedStarPieceNum() == 456,
                        "original PLAY deserialization resets backup lives to four and preserves stored star bits");
                file.resetAllData();
                require(smgpc::compat::game_data::holder_story_progress(current) == 0 &&
                            !current.isOnGameEventFlag(smgpc::resource::encode_cp932("ハチマリオ初変身").c_str()) && current.getPictureBookChapterAlreadyRead() == 0 &&
                            smgpc::compat::game_data::holder_story_progress(backup) == 5,
                        "reset acts on the actual current chunks and leaves the scene-start snapshot intact");
            }
            require_absent_bindings();
            require(resources.host_heaps()->root_heap().getTotalFreeSize() == free_before,
                    "each selected-file owner must return its complete profile heap on teardown");
        }

        require_throws<std::out_of_range>([&] { smgpc::compat::GameDataSession invalid(0, resources, catalog); },
                                          "native selected-file owner must reject slot zero");
        require_throws<std::out_of_range>([&] { smgpc::compat::GameDataSession invalid(7, resources, catalog); },
                                          "native selected-file owner must reject slots beyond six");
        require_absent_bindings();
        require(resources.host_heaps()->root_heap().getTotalFreeSize() == free_before,
                "rejected native owner construction must not allocate a profile heap");

        {
            smgpc::compat::GameDataSession outer(2, resources, catalog);
            GameDataFunction::followStoryEventByName(smgpc::resource::encode_cp932("チコガイドデモ終了").c_str());
            outer.store_scene_start();
            GameDataFunction::followStoryEventByName(smgpc::resource::encode_cp932("スピン権利").c_str());
            require(smgpc::compat::game_data::holder_story_progress(outer.holder()) == 15 &&
                        smgpc::compat::game_data::holder_story_progress(outer.scene_start_holder()) == 10,
                    "current story may advance beyond its separate authored scene-start snapshot");
            {
                smgpc::compat::GameDataSession inner(6, resources, catalog);
                require(GameDataFunction::getCurrentGameDataHolder() == &inner.holder() &&
                            GameDataFunction::getSceneStartGameDataHolder() == &inner.scene_start_holder(),
                        "nested actual session must replace both current and backup bindings");
                GameDataFunction::followStoryEventByName(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str());
                inner.store_scene_start();
                GameDataFunction::followStoryEventByName(smgpc::resource::encode_cp932("チコガイドデモ終了").c_str());
                require(smgpc::compat::game_data::holder_story_progress(inner.holder()) == 10 &&
                            smgpc::compat::game_data::holder_story_progress(inner.scene_start_holder()) == 5 &&
                            smgpc::compat::game_data::holder_story_progress(outer.holder()) == 15,
                        "nested session current/backup changes must not mutate the outer actual holder");
            }
            require(GameDataFunction::getCurrentGameDataHolder() == &outer.holder() &&
                        GameDataFunction::getSceneStartGameDataHolder() == &outer.scene_start_holder() &&
                        smgpc::compat::game_data::holder_story_progress(outer.holder()) == 15 &&
                        smgpc::compat::game_data::holder_story_progress(outer.scene_start_holder()) == 10,
                    "nested session destruction must restore both unchanged outer owners");
        }
        require_absent_bindings();
        require(resources.host_heaps()->root_heap().getTotalFreeSize() == free_before,
                "nested sessions must reclaim both original profile heaps");
        std::cout << "Game-data real-or-absent tests passed: original UserFiles, distinct snapshots, six slots and nested ownership\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Game-data real-or-absent tests failed: " << error.what() << '\n';
        return 1;
    }
}
