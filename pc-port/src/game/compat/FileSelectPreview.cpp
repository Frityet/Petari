#include "FileSelectPreview.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwchar>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Game/Map/FileSelectFunc.hpp"
#include "Game/Map/FileSelectIconID.hpp"
#include "Game/Screen/BackButton.hpp"
#include "Game/Screen/FileSelectButton.hpp"
#include "Game/Screen/FileSelectInfo.hpp"
#include "Game/Screen/FileSelectNumber.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/Manual2P.hpp"
#include "Game/Screen/SysInfoWindow.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Logger.hpp"
#include "compat/FileSelectPreviewTextures.hpp"
#include "compat/LayoutTextureCompat.hpp"
#include "compat/RuntimeContext.hpp"
#include "compat/SharedSkyBackground.hpp"
#include "layout/LayoutDrawList.hpp"
#include "layout/Tpl.hpp"

namespace smgpc::game {
    namespace {

        [[nodiscard]] std::string trim_ascii_space(std::string text) {
            const auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }

            const auto last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1U);
        }

        [[nodiscard]] std::optional< std::uint32_t > get_positive_number_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr or value[0] == '\0') {
                return std::nullopt;
            }

            errno = 0;
            char* end_pointer = nullptr;
            const unsigned long parsed_value = std::strtoul(value, &end_pointer, 10);
            if (errno != 0 || end_pointer == value || *end_pointer != '\0' || parsed_value == 0UL ||
                parsed_value > std::numeric_limits< std::uint32_t >::max()) {
                return std::nullopt;
            }

            return static_cast< std::uint32_t >(parsed_value);
        }

        [[nodiscard]] bool get_bool_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return false;
            }

            const auto text = trim_ascii_space(value);
            return text != "0" && text != "false" && text != "False" && text != "FALSE";
        }

        [[nodiscard]] std::optional< bool > get_optional_bool_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            const auto text = trim_ascii_space(value);
            return text != "0" && text != "false" && text != "False" && text != "FALSE";
        }

        [[nodiscard]] std::optional< float > get_float_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            char* end_pointer = nullptr;
            const float parsed_value = std::strtof(value, &end_pointer);
            if (end_pointer == value || *end_pointer != '\0' || !std::isfinite(parsed_value)) {
                return std::nullopt;
            }

            return parsed_value;
        }

        [[nodiscard]] float environment_float_or(const char* name, float fallback) {
            if (const auto value = get_float_from_environment(name)) {
                return *value;
            }

            return fallback;
        }

        enum class FileSelectPreviewState {
            Preloading,
            Selecting,
            CreateConfirm,
            Creating,
            IconSelecting,
            IconConfirm,
            IconSaving,
            ExistingInfo,
            ExistingCopySelecting,
            ExistingCopyConfirm,
            ExistingCopying,
            ExistingDeleteConfirm,
            ExistingDeleting,
            Manual,
            Loading,
            Failed,
        };

        [[nodiscard]] constexpr float file_select_reference_x_to_layout(float referenceX) {
            return referenceX * (608.0F / 836.0F);
        }

        [[nodiscard]] constexpr float file_select_layout_x_to_reference(float layoutX) {
            return layoutX * (836.0F / 608.0F);
        }

        [[nodiscard]] TVec2f file_select_prompt_message_trans() {
            return TVec2f(315.0F, 545.0F);
        }

        void trace_icon_select_quad(const char* label, float x, float y, float radius, const assets::layout::tpl::DecodedImage* pTexture,
                                    bool usedPaneAnchor, std::uint64_t frame) {
            if (!get_bool_from_environment("SMGPC_DEBUG_ICON_SELECT")) {
                return;
            }
            if (frame % 60U != 0U) {
                return;
            }

            auto logger = compat::runtime_context().logger;
            if (!logger) {
                return;
            }

            logger->info(__FILE__, __LINE__, logging::Category::GAME,
                         "icon-select frame={} {} center=({:.2f},{:.2f}) radius={:.2f} pane_anchor={} texture={}x{} empty={}", frame, label, x, y,
                         radius, usedPaneAnchor, pTexture != nullptr ? pTexture->width : 0U, pTexture != nullptr ? pTexture->height : 0U,
                         pTexture == nullptr || pTexture->empty());
        }

        [[nodiscard]] bool is_file_select_reference_position_visible(const std::pair< float, float >& position) {
            return position.first >= -96.0F && position.first <= 932.0F && position.second >= -96.0F && position.second <= 552.0F;
        }

        [[nodiscard]] constexpr std::array< std::pair< float, float >, 4 > mii_select_fellow_icon_reference_positions() {
            return {
                std::pair< float, float >{188.7F, 139.0F},
                std::pair< float, float >{342.7F, 139.0F},
                std::pair< float, float >{496.7F, 139.0F},
                std::pair< float, float >{650.7F, 139.0F},
            };
        }

        [[nodiscard]] constexpr std::pair< float, float > mii_select_dummy_icon_reference_position() {
            return {188.7F, 253.0F};
        }

        [[nodiscard]] constexpr std::array< float, 5 > mii_icon_character_animation_frames() {
            return {0.0F, 4.0F, 3.0F, 2.0F, 1.0F};
        }

        struct FileSelectPlacement {
            float x;
            float y;
            float size;
        };

        [[nodiscard]] constexpr std::array< FileSelectPlacement, 6 > created_slot_icon_placements() {
            return {
                FileSelectPlacement{320.0F, 286.0F, 72.0F}, FileSelectPlacement{520.0F, 286.0F, 72.0F},
                FileSelectPlacement{204.0F, 208.0F, 52.0F}, FileSelectPlacement{632.0F, 208.0F, 52.0F},
                FileSelectPlacement{337.0F, 174.0F, 52.0F}, FileSelectPlacement{501.0F, 174.0F, 52.0F},
            };
        }

        [[nodiscard]] std::uint32_t load_auto_confirm_step_budget() {
            if (const auto steps = get_positive_number_from_environment("SMGPC_FILE_SELECT_AUTO_CONFIRM_STEPS")) {
                return *steps;
            }
            return get_bool_from_environment("SMGPC_FILE_SELECT_AUTO_CONFIRM") ? 32U : 0U;
        }

        [[nodiscard]] std::uint8_t clamp_u8(float value) {
            if (value <= 0.0F) {
                return 0U;
            }
            if (value >= 255.0F) {
                return 255U;
            }
            return static_cast< std::uint8_t >(std::lround(value));
        }

        void format_file_select_timestamp(OSTime timestamp, wchar_t* pDate, std::size_t dateCount, wchar_t* pTime, std::size_t timeCount) {
            if (pDate != nullptr && dateCount > 0U) {
                pDate[0] = L'\0';
            }
            if (pTime != nullptr && timeCount > 0U) {
                pTime[0] = L'\0';
            }
            if (timestamp <= 0) {
                return;
            }

            const auto seconds = static_cast< std::time_t >(timestamp / 1000000);
            std::tm calendar{};
#if defined(_WIN32)
            localtime_s(&calendar, &seconds);
#else
            localtime_r(&seconds, &calendar);
#endif
            if (pDate != nullptr && dateCount > 0U) {
                std::swprintf(pDate, dateCount, L"%04d/%02d/%02d", calendar.tm_year + 1900, calendar.tm_mon + 1, calendar.tm_mday);
            }
            if (pTime != nullptr && timeCount > 0U) {
                std::swprintf(pTime, timeCount, L"%02d:%02d", calendar.tm_hour, calendar.tm_min);
            }
        }

        [[nodiscard]] float smooth_step(float edge0, float edge1, float value) {
            const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0F, 1.0F);
            return t * t * (3.0F - 2.0F * t);
        }

        [[nodiscard]] assets::layout::tpl::DecodedImage make_copy_source_glow_texture() {
            constexpr std::uint16_t size = 128U;
            assets::layout::tpl::DecodedImage output{};
            output.width = size;
            output.height = size;
            output.rgba8.assign(static_cast< std::size_t >(size) * size * 4U, 0U);

            for (std::uint16_t y = 0U; y < size; ++y) {
                for (std::uint16_t x = 0U; x < size; ++x) {
                    const float dx = (static_cast< float >(x) + 0.5F) / static_cast< float >(size) * 2.0F - 1.0F;
                    const float dy = (static_cast< float >(y) + 0.5F) / static_cast< float >(size) * 2.0F - 1.0F;
                    const float radius = std::sqrt(dx * dx + dy * dy);
                    const float core = 1.0F - smooth_step(0.0F, 0.62F, radius);
                    const float rim = (1.0F - smooth_step(0.48F, 0.95F, radius)) * smooth_step(0.18F, 0.68F, radius);
                    const float edge = 1.0F - smooth_step(0.92F, 1.0F, radius);
                    const float alpha = std::clamp(core * 0.24F + rim * 0.62F, 0.0F, 1.0F) * edge;
                    const std::size_t index = (static_cast< std::size_t >(y) * size + x) * 4U;
                    output.rgba8[index + 0U] = clamp_u8(16.0F + rim * 44.0F);
                    output.rgba8[index + 1U] = clamp_u8(94.0F + core * 28.0F + rim * 88.0F);
                    output.rgba8[index + 2U] = 255U;
                    output.rgba8[index + 3U] = clamp_u8(alpha * 165.0F);
                }
            }

            return output;
        }

    }  // namespace

    class FileSelectPreview::Impl {
    public:
        Impl()
            : _buttons("ファイルセレクトボタン"), _fileInfo(11, "ファイル情報"), _manual("2Pマニュアル"),
              _backButton("ファイルコピー戻るボタン", true), _miiSelectLayout("MiiSelect PC layout", true),
              _miiConfirmIconLayout("MiiConfirmIcon PC layout", true), _pointerLayout("DPDPointer PC layout", true) {
            _buttons.initWithoutIter();
            _buttons.setCallbackFunctor(MR::Functor(this, &Impl::startSelectedFile), MR::Functor(this, &Impl::beginCopyConfirm),
                                        MR::Functor(this, &Impl::beginExistingIconChange), MR::Functor(this, &Impl::beginDeleteConfirm),
                                        MR::Functor(this, &Impl::beginManual));
            _fileInfo.initLayoutManager("FileInfo", 3);
            _manual.initWithoutIter();
            _backButton.initWithoutIter();
            _miiSelectLayout.initLayoutManager("MiiSelect", 1);
            _miiConfirmIconLayout.initLayoutManager("MiiConfirmIcon", 1);
            _pointerLayout.initLayoutManager("DPDPointer", 1);
            setupFileSelectPointerLayout();
            for (auto& iconLayout : _miiIconLayouts) {
                iconLayout = std::make_unique< LayoutActor >("MiiSelectIcon PC layout", true);
                iconLayout->initLayoutManager("MiiIcon", 1);
                iconLayout->kill();
            }
            _fileInfo.kill();
            _buttons.kill();
            _backButton.kill();
            _miiSelectLayout.kill();
            _miiConfirmIconLayout.kill();
            _saveSequence.initAfterResourceLoaded();
            GameSequenceFunction::setHostSaveDataHandleSequence(&_saveSequence);
            _infoMessage.initWithoutIter();
            _iconPromptMessage.initWithoutIter();
            _sysInfoWindow.initWithoutIter();
            _skyTextures = file_select_preview::load_sky_textures();
            _pointerTextures = file_select_preview::load_pointer_textures();
            _fellowIconTextures = file_select_preview::load_fellow_icon_textures();
            buildFellowIconTexMaps();
            _miiSelectTextures = file_select_preview::load_mii_select_textures();
            _planetTextures = file_select_preview::load_planet_textures();
            _badgeTexture = file_select_preview::make_file_number_badge_texture();
            _promptPillTexture = file_select_preview::make_prompt_pill_texture();
            _pageCounterTexture = file_select_preview::make_page_counter_pill_texture();
            _copySourceGlowTexture = make_copy_source_glow_texture();

            for (auto& userFile : _userFiles) {
                userFile = std::make_unique< UserFile >();
            }

            const std::array< TVec2f, 6 > positions{
                TVec2f{file_select_reference_x_to_layout(312.0F), 236.0F}, TVec2f{file_select_reference_x_to_layout(522.0F), 236.0F},
                TVec2f{file_select_reference_x_to_layout(200.0F), 179.0F}, TVec2f{file_select_reference_x_to_layout(633.0F), 179.0F},
                TVec2f{file_select_reference_x_to_layout(333.0F), 150.0F}, TVec2f{file_select_reference_x_to_layout(503.0F), 150.0F},
            };

            for (std::size_t i = 0U; i < _numbers.size(); ++i) {
                auto number = std::make_unique< FileSelectNumber >("ファイル番号");
                number->initWithoutIter();
                number->setNumber(static_cast< s32 >(i + 1U));
                number->setPaneVisible("PicBase", false);
                number->setPaneVisible("PicBaseFrame", false);
                number->setTrans(positions[i]);
                _numbers[i] = std::move(number);
            }
        }

        void appear() {
            _endRequested = false;
            _completion = FileSelectPreviewCompletion::None;
            _autoConfirmRemaining = load_auto_confirm_step_budget();
            _wasConfirmPressed = MR::testCorePadButtonA(0);
            (void)MR::testCorePadTriggerLeft(WPAD_CHAN0);
            (void)MR::testCorePadTriggerRight(WPAD_CHAN0);
            (void)MR::testCorePadTriggerUp(WPAD_CHAN0);
            (void)MR::testCorePadTriggerDown(WPAD_CHAN0);
            (void)MR::testSystemTriggerB();
            _selectedIconIndex = 0U;
            _copySourceIndex.reset();
            _copyDestinationIndex.reset();
            _planetAnimationFrame = 0U;
            _sysInfoWindow.forceKill();

            const auto selectedSlot = get_positive_number_from_environment("SMGPC_FILE_SELECT_SLOT");
            _selectedIndex = selectedSlot.has_value() ? std::min< std::size_t >(*selectedSlot, _numbers.size()) - 1U : 0U;

            _infoMessage.setCenter(false);
            _iconPromptMessage.kill();
            _infoMessage.setMessage("System_FileSelect008");
            _infoMessage.setTrans(file_select_prompt_message_trans());
            _infoMessage.appear();
            _infoMessage.setPaneVisible("InfoWinU", false);
            _infoMessage.setPaneVisible("InfoWinC", false);

            for (std::size_t i = 0U; i < _numbers.size(); ++i) {
                _numbers[i]->appear();
                if (i == _selectedIndex) {
                    _numbers[i]->onSelectIn();
                } else {
                    _numbers[i]->onSelectOut();
                }
            }

            GameSequenceFunction::startPreLoadSaveDataSequence();
            _state = FileSelectPreviewState::Preloading;
        }

        void movement() {
            _saveSequence.update();
            _infoMessage.movement();
            _iconPromptMessage.movement();
            _sysInfoWindow.movement();
            _fileInfo.movement();
            const auto stateBeforeButtonMovement = _state;
            _buttons.movement();
            const bool stateChangedByButtonMovement = _state != stateBeforeButtonMovement;
            _manual.movementWithChildren();
            _backButton.movement();
            _miiSelectLayout.movement();
            _miiConfirmIconLayout.movement();
            updatePointerLayout();
            _pointerLayout.movement();
            for (auto& iconLayout : _miiIconLayouts) {
                if (iconLayout != nullptr) {
                    iconLayout->movement();
                }
            }
            for (auto& number : _numbers) {
                number->movement();
            }
            ++_planetAnimationFrame;
            updateSaveSequenceState();
            if (_state == FileSelectPreviewState::Manual && _manual.isClosed()) {
                showExistingFileInfo();
            }

            if (_state == FileSelectPreviewState::Selecting || _state == FileSelectPreviewState::ExistingCopySelecting) {
                updateSelectionInput();
            } else if (_state == FileSelectPreviewState::IconSelecting) {
                updateIconSelectionInput();
            }

            const bool confirmPressed = MR::testCorePadButtonA(0);
            const bool confirmTriggered = confirmPressed && !_wasConfirmPressed;
            const bool cancelTriggered = MR::testSystemTriggerB();
            if (stateChangedByButtonMovement) {
                _wasConfirmPressed = confirmPressed;
                return;
            }
            if (_state == FileSelectPreviewState::ExistingCopySelecting && _backButton._24) {
                showExistingFileInfo();
                _wasConfirmPressed = confirmPressed;
                return;
            }

            if (_state == FileSelectPreviewState::Selecting && (confirmTriggered || consumeAutoConfirm())) {
                confirmSelectedFile();
            } else if (_state == FileSelectPreviewState::CreateConfirm && (confirmTriggered || consumeAutoConfirm())) {
                createSelectedFile();
            } else if (_state == FileSelectPreviewState::CreateConfirm && cancelTriggered) {
                returnToFileSelect();
            } else if (_state == FileSelectPreviewState::IconSelecting && (confirmTriggered || consumeAutoConfirm())) {
                beginIconConfirm();
            } else if (_state == FileSelectPreviewState::IconSelecting && cancelTriggered) {
                returnToFileSelect();
            } else if (_state == FileSelectPreviewState::IconConfirm && (confirmTriggered || consumeAutoConfirm())) {
                saveSelectedIcon();
            } else if (_state == FileSelectPreviewState::IconConfirm && cancelTriggered) {
                beginIconSelectFirst();
            } else if (_state == FileSelectPreviewState::ExistingInfo && confirmTriggered) {
                startSelectedFile();
            } else if (_state == FileSelectPreviewState::ExistingInfo && consumeAutoConfirm()) {
                runExistingAutoAction();
            } else if (_state == FileSelectPreviewState::ExistingInfo && cancelTriggered) {
                returnToFileSelect();
            } else if (_state == FileSelectPreviewState::ExistingCopySelecting && (confirmTriggered || consumeAutoConfirm())) {
                beginCopyTargetConfirm();
            } else if (_state == FileSelectPreviewState::ExistingCopySelecting && cancelTriggered) {
                showExistingFileInfo();
            } else if (_state == FileSelectPreviewState::ExistingCopyConfirm && (confirmTriggered || consumeAutoConfirm())) {
                copySelectedFileToSelectedSlot();
            } else if (_state == FileSelectPreviewState::ExistingCopyConfirm && cancelTriggered) {
                returnToCopySelect();
            } else if (_state == FileSelectPreviewState::ExistingDeleteConfirm && (confirmTriggered || consumeAutoConfirm())) {
                deleteSelectedFile();
            } else if (_state == FileSelectPreviewState::ExistingDeleteConfirm && cancelTriggered) {
                showExistingFileInfo();
            }
            updatePointerLayout();
            _wasConfirmPressed = confirmPressed;
        }

        void appendDrawCommands(render::layout::LayoutDrawList* pDrawList, std::uint64_t skyFrame) const {
            appendSkyDrawCommands(pDrawList, skyFrame);
            appendPlanetDrawCommands(pDrawList);
            appendCopySourceHighlightDrawCommands(pDrawList);
            appendCreatedSlotIconDrawCommands(pDrawList);
            appendBadgeDrawCommands(pDrawList);
            appendIconSelectionDrawCommands(pDrawList);
            if (shouldDrawFileSelectItems()) {
                for (const auto& number : _numbers) {
                    number->appendDrawCommands(pDrawList);
                }
            }
            appendPromptPillDrawCommands(pDrawList);
            _infoMessage.appendDrawCommands(pDrawList);
            _iconPromptMessage.appendDrawCommands(pDrawList);
            _fileInfo.appendDrawCommands(pDrawList);
            _buttons.appendDrawCommands(pDrawList);
            _manual.appendDrawCommandsWithChildren(pDrawList);
            _backButton.appendDrawCommands(pDrawList);
            _sysInfoWindow.appendDrawCommands(pDrawList);
            appendMiiConfirmIconDrawCommands(pDrawList);
            _saveSequence.appendDrawCommands(pDrawList);
            appendPointerDrawCommands(pDrawList);
        }

        [[nodiscard]] bool isEnd() const {
            return _endRequested;
        }

        [[nodiscard]] FileSelectPreviewCompletion completion() const {
            return _completion;
        }

        [[nodiscard]] const LayoutActor* layoutForSize() const {
            return nullptr;
        }

        [[nodiscard]] std::uint64_t skyFrame() const {
            return _planetAnimationFrame;
        }

        void setupFileSelectPointerLayout() {
            if (_pointerLayout.getResource() == nullptr) {
                return;
            }

            _pointerLayout.appear();
            MR::hidePane(&_pointerLayout, "PicNozzle");
            MR::hidePaneRecursive(&_pointerLayout, "HandArrow");
            MR::hidePaneRecursive(&_pointerLayout, "StarPointer");
            MR::showPaneRecursive(&_pointerLayout, "HandPointer");
            MR::showPaneRecursive(&_pointerLayout, "PicPlayer1");
            MR::hidePaneRecursive(&_pointerLayout, "PicPlayer2");
            MR::startAnim(&_pointerLayout, "Wait", 0);
            updatePointerLayout();
        }

    private:
        [[nodiscard]] bool shouldDrawFileSelectItems() const {
            return _state == FileSelectPreviewState::Preloading || _state == FileSelectPreviewState::Selecting ||
                   _state == FileSelectPreviewState::ExistingCopySelecting || _state == FileSelectPreviewState::Loading;
        }

        void updateSaveSequenceState() {
            if ((_state != FileSelectPreviewState::Preloading && _state != FileSelectPreviewState::Creating &&
                 _state != FileSelectPreviewState::IconSaving && _state != FileSelectPreviewState::ExistingCopying &&
                 _state != FileSelectPreviewState::ExistingDeleting && _state != FileSelectPreviewState::Loading) ||
                GameSequenceFunction::isActiveSaveDataHandleSequence()) {
                return;
            }

            if (!GameSequenceFunction::isSuccessSaveDataHandleSequence()) {
                _sysInfoWindow.forceKill();
                _state = FileSelectPreviewState::Failed;
                _completion = FileSelectPreviewCompletion::Failed;
                _endRequested = true;
                return;
            }

            if (_state == FileSelectPreviewState::Preloading) {
                refreshUserFiles();
                _state = FileSelectPreviewState::Selecting;
                _iconSelectionForExistingFile = false;
                _iconSavingExistingFile = false;
                updateSelectedFileInfoPreview();
                return;
            }

            if (_state == FileSelectPreviewState::Creating) {
                refreshUserFiles();
                beginIconSelectFirst();
                return;
            }

            if (_state == FileSelectPreviewState::IconSaving) {
                if (_iconSavingExistingFile) {
                    _iconSavingExistingFile = false;
                    refreshUserFiles();
                    showExistingFileInfo();
                    return;
                }
                _completion = FileSelectPreviewCompletion::CreatedNewFile;
            } else if (_state == FileSelectPreviewState::ExistingCopying || _state == FileSelectPreviewState::ExistingDeleting) {
                refreshUserFiles();
                showFileSelectPrompt();
                _state = FileSelectPreviewState::Selecting;
                _copySourceIndex.reset();
                _copyDestinationIndex.reset();
                updateSelectedFileInfoPreview();
                return;
            } else if (_state == FileSelectPreviewState::Loading) {
                _completion = FileSelectPreviewCompletion::LoadedExistingFile;
            }
            _endRequested = true;
        }

        void refreshUserFiles() {
            for (std::size_t i = 0U; i < _userFiles.size(); ++i) {
                GameSequenceFunction::restoreUserFile(_userFiles[i].get(), static_cast< int >(i + 1U));
                _slotCreated[i] = _userFiles[i]->isCreated();
            }
        }

        void setSelectedIndex(std::size_t index) {
            index = std::min(index, _numbers.size() - 1U);
            if (index == _selectedIndex) {
                return;
            }

            if (_numbers[_selectedIndex] != nullptr) {
                _numbers[_selectedIndex]->onSelectOut();
            }
            _selectedIndex = index;
            if (_numbers[_selectedIndex] != nullptr) {
                _numbers[_selectedIndex]->onSelectIn();
            }
            updateSelectedFileInfoPreview();
        }

        void updateSelectionInput() {
            if (MR::testCorePadTriggerLeft(WPAD_CHAN0)) {
                setSelectedIndex(_selectedIndex == 0U ? _numbers.size() - 1U : _selectedIndex - 1U);
            }
            if (MR::testCorePadTriggerRight(WPAD_CHAN0)) {
                setSelectedIndex((_selectedIndex + 1U) % _numbers.size());
            }
            if (MR::testCorePadTriggerUp(WPAD_CHAN0)) {
                if (_selectedIndex == 2U || _selectedIndex == 4U) {
                    setSelectedIndex(0U);
                } else if (_selectedIndex == 3U || _selectedIndex == 5U) {
                    setSelectedIndex(1U);
                }
            }
            if (MR::testCorePadTriggerDown(WPAD_CHAN0)) {
                if (_selectedIndex == 0U) {
                    setSelectedIndex(2U);
                } else if (_selectedIndex == 1U) {
                    setSelectedIndex(3U);
                } else if (_selectedIndex == 2U) {
                    setSelectedIndex(4U);
                } else if (_selectedIndex == 3U) {
                    setSelectedIndex(5U);
                }
            }
        }

        [[nodiscard]] bool consumeAutoConfirm() {
            if (_autoConfirmRemaining == 0U) {
                return false;
            }

            --_autoConfirmRemaining;
            return true;
        }

        [[nodiscard]] static std::string_view existingAutoAction() {
            const char* value = std::getenv("SMGPC_FILE_SELECT_EXISTING_ACTION");
            if (value == nullptr || value[0] == '\0') {
                return "start";
            }

            return value;
        }

        void runExistingAutoAction() {
            const std::string_view action = existingAutoAction();
            if (action == "copy") {
                beginCopyConfirm();
            } else if (action == "delete") {
                beginDeleteConfirm();
            } else if (action == "icon" || action == "mii") {
                beginExistingIconChange();
            } else if (action == "manual") {
                beginManual();
            } else if (action == "back") {
                returnToFileSelect();
            } else {
                startSelectedFile();
            }
        }

        [[nodiscard]] static constexpr std::array< FileSelectIconID::EFellowID, 4 > selectableFellowIds() {
            return {FileSelectIconID::Mario, FileSelectIconID::Yoshi, FileSelectIconID::Kinopio, FileSelectIconID::Peach};
        }

        [[nodiscard]] static const char* iconMessageId(FileSelectIconID::EFellowID fellowId) {
            static constexpr std::array< const char*, 5 > messageIds{
                "System_FileSelect_Icon000", "System_FileSelect_Icon001", "System_FileSelect_Icon002",
                "System_FileSelect_Icon003", "System_FileSelect_Icon004",
            };
            const auto index = static_cast< std::size_t >(fellowId);

            return index < messageIds.size() ? messageIds[index] : messageIds[0U];
        }

        [[nodiscard]] FileSelectIconID::EFellowID selectedFellowId() const {
            const auto fellowIds = selectableFellowIds();
            return fellowIds[std::min(_selectedIconIndex, fellowIds.size() - 1U)];
        }

        [[nodiscard]] const assets::layout::tpl::DecodedImage* selectedFellowTexture() const {
            const auto fellowIndex = static_cast< std::size_t >(selectedFellowId());
            if (fellowIndex >= _fellowIconTextures.fellows.size() || !_fellowIconTextures.fellows[fellowIndex].has_value()) {
                return nullptr;
            }
            return &*_fellowIconTextures.fellows[fellowIndex];
        }

        [[nodiscard]] const nw4r::lyt::TexMap* selectedFellowTexMap() const {
            const auto fellowIndex = static_cast< std::size_t >(selectedFellowId());
            if (fellowIndex >= _fellowIconTexMaps.size() || _fellowIconTexMaps[fellowIndex] == nullptr) {
                return nullptr;
            }
            return _fellowIconTexMaps[fellowIndex].get();
        }

        void buildFellowIconTexMaps() {
            for (std::size_t i = 0U; i < _fellowIconTextures.fellows.size(); ++i) {
                if (_fellowIconTextures.fellows[i].has_value()) {
                    _fellowIconTexMaps[i] = std::make_unique< nw4r::lyt::TexMap >(&*_fellowIconTextures.fellows[i]);
                }
            }
            if (_fellowIconTextures.mii_placeholder.has_value()) {
                _miiPlaceholderTexMap = std::make_unique< nw4r::lyt::TexMap >(&*_fellowIconTextures.mii_placeholder);
            }
        }

        void updateMiiSelectLayoutText() {
            const auto* selected_name = MR::getGameMessageDirect(iconMessageId(selectedFellowId()));
            if (selected_name != nullptr) {
                MR::setTextBoxMessageRecursive(&_miiSelectLayout, "TxtName", selected_name);
            }
        }

        void setupMiiSelectLayout() {
            _miiConfirmIconLayout.kill();
            _miiSelectLayout.appear();
            MR::startAnim(&_miiSelectLayout, "Wait", 0);
            MR::setTextBoxMessageRecursive(&_miiSelectLayout, "TxtMiiChange", L"좋아하는 아이콘을 선택해 주세요");
            MR::setTextBoxFormatRecursive(&_miiSelectLayout, "TxtPage", L"%d/%d", 1, 1);
            _miiSelectLayout.setPaneVisibleRecursive("Left", false);
            _miiSelectLayout.setPaneVisibleRecursive("Right", false);
            _miiSelectLayout.setPaneVisibleRecursive("MiiGroupB", false);
            updateMiiSelectLayoutText();
            setupMiiIconLayouts();
        }

        void setupMiiConfirmIconLayout() {
            _miiConfirmIconLayout.appear();
            MR::startAnim(&_miiConfirmIconLayout, "ButtonWait", 0);
            const auto* selected_name = MR::getGameMessageDirect(iconMessageId(selectedFellowId()));
            if (selected_name != nullptr) {
                MR::setTextBoxMessageRecursive(&_miiConfirmIconLayout, "MiiName", selected_name);
            }
            const auto* texMap = selectedFellowTexMap();
            MR::replacePaneTexture(&_miiConfirmIconLayout, "ShaMiiDummy", texMap, 0);
            MR::replacePaneTexture(&_miiConfirmIconLayout, "PicMiiDummy", texMap, 0);
        }

        void configureFellowIconLayout(LayoutActor* pIconLayout, FileSelectIconID::EFellowID fellowId, std::pair< float, float > position) const {
            if (pIconLayout == nullptr) {
                return;
            }

            const auto frame = mii_icon_character_animation_frames()[static_cast< std::size_t >(fellowId)];
            pIconLayout->appear();
            pIconLayout->setTrans(TVec2f(file_select_reference_x_to_layout(position.first), position.second));
            MR::showPane(pIconLayout, "MarioIcon");
            MR::hidePane(pIconLayout, "MiiIcon");
            MR::startAnim(pIconLayout, "Character", 0);
            MR::setAnimFrameAndStop(pIconLayout, frame, 0);
            MR::startPaneAnim(pIconLayout, "MarioIcon", "Character", 1);
            MR::setPaneAnimFrameAndStop(pIconLayout, "MarioIcon", frame, 1);
            MR::startPaneAnim(pIconLayout, "MarioIcon", "Hide", 2);
            MR::setPaneAnimFrameAndStop(pIconLayout, "MarioIcon", 0.0F, 2);
            MR::startPaneAnim(pIconLayout, "MiiIcon", "Hide", 2);
            MR::setPaneAnimFrameAndStop(pIconLayout, "MiiIcon", 0.0F, 2);
        }

        void configureDummyIconLayout(LayoutActor* pIconLayout, std::pair< float, float > position) const {
            if (pIconLayout == nullptr) {
                return;
            }

            constexpr float DUMMY_FRAME = 5.0F;
            pIconLayout->appear();
            pIconLayout->setTrans(TVec2f(file_select_reference_x_to_layout(position.first), position.second));
            MR::showPane(pIconLayout, "MarioIcon");
            MR::hidePane(pIconLayout, "MiiIcon");
            MR::startAnim(pIconLayout, "Character", 0);
            MR::setAnimFrameAndStop(pIconLayout, DUMMY_FRAME, 0);
            MR::startPaneAnim(pIconLayout, "MarioIcon", "Character", 1);
            MR::setPaneAnimFrameAndStop(pIconLayout, "MarioIcon", DUMMY_FRAME, 1);
            MR::startPaneAnim(pIconLayout, "MarioIcon", "Hide", 2);
            MR::setPaneAnimFrameAndStop(pIconLayout, "MarioIcon", 0.0F, 2);
            MR::startPaneAnim(pIconLayout, "MiiIcon", "Hide", 2);
            MR::setPaneAnimFrameAndStop(pIconLayout, "MiiIcon", 0.0F, 2);
        }

        void setupMiiIconLayouts() {
            constexpr auto fellowIds = selectableFellowIds();
            constexpr auto fallbackPositions = mii_select_fellow_icon_reference_positions();
            static constexpr std::array< const char*, 4 > iconPanes{"Mii01", "Mii02", "Mii03", "Mii04"};

            for (std::size_t i = 0U; i < fellowIds.size(); ++i) {
                bool usedPaneAnchor = false;
                const auto position = miiSelectPaneReferencePosition(iconPanes[i], fallbackPositions[i], &usedPaneAnchor);
                configureFellowIconLayout(_miiIconLayouts[i].get(), fellowIds[i], position);
                trace_icon_select_quad(iconPanes[i], position.first, position.second, 50.0F,
                                       _fellowIconTextures.fellows[static_cast< std::size_t >(fellowIds[i])].has_value() ?
                                           &*_fellowIconTextures.fellows[static_cast< std::size_t >(fellowIds[i])] :
                                           nullptr,
                                       usedPaneAnchor, _planetAnimationFrame);
            }

            bool usedMiiPaneAnchor = false;
            const auto miiPosition = miiSelectPaneReferencePosition("Mii05", mii_select_dummy_icon_reference_position(), &usedMiiPaneAnchor);
            configureDummyIconLayout(_miiIconLayouts[fellowIds.size()].get(), miiPosition);
            trace_icon_select_quad("Mii05", miiPosition.first, miiPosition.second, 50.0F,
                                   _fellowIconTextures.mii_placeholder.has_value() ? &*_fellowIconTextures.mii_placeholder : nullptr,
                                   usedMiiPaneAnchor, _planetAnimationFrame);
        }

        void hideIconLayouts() {
            _miiSelectLayout.kill();
            _miiConfirmIconLayout.kill();
            for (auto& iconLayout : _miiIconLayouts) {
                if (iconLayout != nullptr) {
                    iconLayout->kill();
                }
            }
        }

        [[nodiscard]] std::pair< float, float > miiSelectPaneReferencePosition(const char* pPaneName, std::pair< float, float > fallback,
                                                                               bool* pUsedPaneAnchor = nullptr) const {
            TVec2f position{};
            if (_miiSelectLayout.getPaneTrans(pPaneName, &position)) {
                const auto reference_position = std::pair< float, float >{file_select_layout_x_to_reference(position.x), position.y};
                if (is_file_select_reference_position_visible(reference_position)) {
                    if (pUsedPaneAnchor != nullptr) {
                        *pUsedPaneAnchor = true;
                    }
                    return reference_position;
                }
            }
            if (pUsedPaneAnchor != nullptr) {
                *pUsedPaneAnchor = false;
            }
            return fallback;
        }

        [[nodiscard]] std::pair< float, float > selectedIconReferencePosition() const {
            static constexpr std::array< const char*, 4 > iconPanes{"Mii01", "Mii02", "Mii03", "Mii04"};
            constexpr auto fallbackIconPositions = mii_select_fellow_icon_reference_positions();
            const auto index = std::min(_selectedIconIndex, iconPanes.size() - 1U);
            return miiSelectPaneReferencePosition(iconPanes[index], fallbackIconPositions[index]);
        }

        [[nodiscard]] std::pair< float, float > pointerHandTopLeftReferencePosition() const {
            float handX = 394.0F;
            float handY = 180.0F;
            if (_state == FileSelectPreviewState::IconConfirm) {
                const auto position = selectedIconReferencePosition();
                handX = position.first - 29.0F;
                handY = position.second - 24.0F;
            } else if (_state == FileSelectPreviewState::CreateConfirm || _state == FileSelectPreviewState::ExistingCopyConfirm ||
                       _state == FileSelectPreviewState::ExistingDeleteConfirm) {
                handX = 238.0F;
                handY = 205.0F;
            } else if (_state == FileSelectPreviewState::Creating || _state == FileSelectPreviewState::IconSaving ||
                       _state == FileSelectPreviewState::ExistingCopying || _state == FileSelectPreviewState::ExistingDeleting) {
                handX = 535.0F;
                handY = 360.0F;
            } else if (_state == FileSelectPreviewState::IconSelecting) {
                const auto position = selectedIconReferencePosition();
                handX = position.first - 29.0F;
                handY = position.second - 24.0F;
            } else if (_state == FileSelectPreviewState::Manual) {
                handX = 736.0F;
                handY = 296.0F;
            }

            return {handX, handY};
        }

        [[nodiscard]] std::pair< float, float > pointerLayoutCenterReferencePosition() const {
            constexpr float POINTER_LAYOUT_REFERENCE_X_ADJUST = -11.0F;
            constexpr float POINTER_LAYOUT_REFERENCE_Y_ADJUST = 5.0F;
            const auto handTopLeft = pointerHandTopLeftReferencePosition();
            return {
                handTopLeft.first + file_select_layout_x_to_reference(19.0F) + POINTER_LAYOUT_REFERENCE_X_ADJUST,
                handTopLeft.second + 7.0F + POINTER_LAYOUT_REFERENCE_Y_ADJUST,
            };
        }

        void updatePointerLayout() {
            if (_pointerLayout.getResource() == nullptr || _pointerLayout.isDead()) {
                return;
            }

            const auto position = pointerLayoutCenterReferencePosition();
            _pointerLayout.setTrans(TVec2f(file_select_reference_x_to_layout(position.first), position.second));
        }

        void updateIconSelectionInput() {
            constexpr auto fellowIds = selectableFellowIds();
            if (MR::testCorePadTriggerLeft(WPAD_CHAN0)) {
                _selectedIconIndex = _selectedIconIndex == 0U ? fellowIds.size() - 1U : _selectedIconIndex - 1U;
                updateMiiSelectLayoutText();
            }
            if (MR::testCorePadTriggerRight(WPAD_CHAN0)) {
                _selectedIconIndex = (_selectedIconIndex + 1U) % fellowIds.size();
                updateMiiSelectLayoutText();
            }
        }

        void showFileSelectPrompt() {
            _sysInfoWindow.forceKill();
            _fileInfo.kill();
            _buttons.kill();
            _backButton.kill();
            _manual.kill();
            _iconPromptMessage.kill();
            hideIconLayouts();
            _infoMessage.setCenter(false);
            _infoMessage.setMessage("System_FileSelect008");
            _infoMessage.setTrans(file_select_prompt_message_trans());
            _infoMessage.appear();
            _infoMessage.setPaneVisible("InfoWinU", false);
            _infoMessage.setPaneVisible("InfoWinC", false);
        }

        void returnToFileSelect() {
            showFileSelectPrompt();
            _state = FileSelectPreviewState::Selecting;
            updateSelectedFileInfoPreview();
        }

        void confirmSelectedFile() {
            if (_slotCreated[_selectedIndex]) {
                showExistingFileInfo();
                return;
            }

            _infoMessage.kill();
            _sysInfoWindow.appear("System_FileSelect001", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_Game);
            _sysInfoWindow.setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_NEW_FILE", "SE_SY_TALK_SELECT_NO");
            _state = FileSelectPreviewState::CreateConfirm;
        }

        void setFileInfoFromSelectedSlot() {
            auto& userFile = *_userFiles[_selectedIndex];
            std::array< u16, 16 > name{};
            u32 iconId = 0U;
            FileSelectIconID icon;
            if (userFile.getIconId(&iconId) && iconId >= 1U && iconId <= _fellowIconTextures.fellows.size()) {
                icon.setFellowID(static_cast< FileSelectIconID::EFellowID >(iconId - 1U));
            } else {
                icon.setMiiIndex(0U);
            }
            FileSelectFunc::copyMiiName(name.data(), icon);

            wchar_t dateMessage[32]{};
            wchar_t timeMessage[32]{};
            format_file_select_timestamp(userFile.getLastModified(), dateMessage, std::size(dateMessage), timeMessage, std::size(timeMessage));

            _fileInfo.setInfo(name.data(), static_cast< s32 >(_selectedIndex + 1U), userFile.getPowerStarNum(), userFile.getStarPieceNum(),
                              userFile.isLastLoadedMario(), userFile.isViewNormalEnding(), userFile.isViewCompleteEnding(), dateMessage, timeMessage,
                              userFile.getPlayerMissNum());
            _fileInfo.reflectInfo();
            _fileInfo.forceChange();
        }

        void updateSelectedFileInfoPreview() {
            if (_state != FileSelectPreviewState::Selecting && _state != FileSelectPreviewState::ExistingCopySelecting) {
                return;
            }

            if (!_slotCreated[_selectedIndex]) {
                _fileInfo.kill();
                return;
            }

            setFileInfoFromSelectedSlot();
            if (_fileInfo.isDead()) {
                _fileInfo.appear();
            }
        }

        void showExistingFileInfo() {
            _sysInfoWindow.forceKill();
            _manual.kill();
            _backButton.kill();
            _infoMessage.kill();
            _iconPromptMessage.kill();
            hideIconLayouts();
            setFileInfoFromSelectedSlot();
            if (_fileInfo.isDead()) {
                _fileInfo.appear();
            }
            _fileInfo.slide();
            _buttons.appear();
            _state = FileSelectPreviewState::ExistingInfo;
        }

        [[nodiscard]] std::optional< std::size_t > defaultCopyTargetIndex() const {
            const auto targetSlot = get_positive_number_from_environment("SMGPC_FILE_SELECT_COPY_TARGET");
            if (targetSlot.has_value() && *targetSlot >= 1U && *targetSlot <= _numbers.size()) {
                const auto targetIndex = *targetSlot - 1U;
                if (targetIndex != _selectedIndex) {
                    return targetIndex;
                }
            }

            for (std::size_t i = 0U; i < _slotCreated.size(); ++i) {
                if (i != _selectedIndex && !_slotCreated[i]) {
                    return i;
                }
            }

            for (std::size_t i = 0U; i < _slotCreated.size(); ++i) {
                if (i != _selectedIndex) {
                    return i;
                }
            }

            return std::nullopt;
        }

        void beginCopyConfirm() {
            _copySourceIndex = _selectedIndex;
            _copyDestinationIndex.reset();
            _buttons.kill();
            _fileInfo.kill();
            _backButton.appear();
            _sysInfoWindow.forceKill();
            _infoMessage.setCenter(false);
            _infoMessage.setMessage("System_FileSelect002");
            _infoMessage.setTrans(file_select_prompt_message_trans());
            _infoMessage.appear();
            _infoMessage.setPaneVisible("InfoWinU", false);
            _infoMessage.setPaneVisible("InfoWinC", false);

            _state = FileSelectPreviewState::ExistingCopySelecting;
            if (const auto target = defaultCopyTargetIndex(); target.has_value()) {
                setSelectedIndex(*target);
            } else {
                updateSelectedFileInfoPreview();
            }
        }

        void returnToCopySelect() {
            _sysInfoWindow.resetYesNoSelectorSE();
            _sysInfoWindow.forceKill();
            _backButton.appear();
            _infoMessage.setCenter(false);
            _infoMessage.setMessage("System_FileSelect002");
            _infoMessage.setTrans(file_select_prompt_message_trans());
            _infoMessage.appear();
            _infoMessage.setPaneVisible("InfoWinU", false);
            _infoMessage.setPaneVisible("InfoWinC", false);
            _state = FileSelectPreviewState::ExistingCopySelecting;
            updateSelectedFileInfoPreview();
        }

        void beginCopyTargetConfirm() {
            if (!_copySourceIndex.has_value() || *_copySourceIndex == _selectedIndex) {
                return;
            }

            _copyDestinationIndex = _selectedIndex;
            _backButton.kill();
            _infoMessage.kill();
            _fileInfo.kill();
            const bool destinationIsNew = !_slotCreated[_selectedIndex];
            _sysInfoWindow.appear(destinationIsNew ? "System_FileSelect016" : "System_FileSelect014", SysInfoWindow::Type_YesNo,
                                  SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_Game);
            _sysInfoWindow.setTextBoxArgNumber(static_cast< s32 >(*_copySourceIndex + 1U), 0);
            _sysInfoWindow.setTextBoxArgNumber(static_cast< s32 >(_selectedIndex + 1U), 1);
            _state = FileSelectPreviewState::ExistingCopyConfirm;
        }

        void copySelectedFileToSelectedSlot() {
            if (!_copySourceIndex.has_value() || !_copyDestinationIndex.has_value() || *_copySourceIndex == *_copyDestinationIndex) {
                _sysInfoWindow.forceKill();
                showExistingFileInfo();
                return;
            }

            const auto sourceIndex = *_copySourceIndex;
            const auto destinationIndex = *_copyDestinationIndex;
            u32 preservedIconId = 0U;
            const bool shouldPreserveDestinationIcon = _slotCreated[destinationIndex] && _userFiles[destinationIndex]->getIconId(&preservedIconId);
            _sysInfoWindow.resetYesNoSelectorSE();
            _sysInfoWindow.forceKill();
            GameSequenceFunction::storeCopyUserFileSequence(static_cast< int >(destinationIndex + 1U), static_cast< int >(sourceIndex + 1U));
            if (shouldPreserveDestinationIcon) {
                GameSequenceFunction::storeMiiOrIconIdUserFileSequence(static_cast< int >(destinationIndex + 1U), nullptr, &preservedIconId);
            }
            GameSequenceFunction::startSaveAllUserFileSequence();
            _state = FileSelectPreviewState::ExistingCopying;
        }

        void beginDeleteConfirm() {
            _buttons.kill();
            _fileInfo.kill();
            _backButton.kill();
            _sysInfoWindow.appear("System_FileSelect007", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_Game);
            _state = FileSelectPreviewState::ExistingDeleteConfirm;
        }

        void deleteSelectedFile() {
            _sysInfoWindow.resetYesNoSelectorSE();
            _sysInfoWindow.forceKill();
            _userFiles[_selectedIndex]->setLastLoadedMario(true);
            GameSequenceFunction::startDeleteUserFileSequence(static_cast< int >(_selectedIndex + 1U));
            _state = FileSelectPreviewState::ExistingDeleting;
        }

        void startSelectedFile() {
            const int fileIndex = static_cast< int >(_selectedIndex + 1U);
            const bool isMario = _userFiles[_selectedIndex]->isLastLoadedMario();
            _fileInfo.kill();
            _buttons.disappear();
            _backButton.kill();
            GameSequenceFunction::startGameDataLoadSequence(fileIndex, isMario);
            _state = FileSelectPreviewState::Loading;
        }

        void createSelectedFile() {
            const int fileIndex = static_cast< int >(_selectedIndex + 1U);
            _infoMessage.kill();
            _sysInfoWindow.resetYesNoSelectorSE();
            _sysInfoWindow.forceKill();
            GameSequenceFunction::startCreateUserFileSequence(fileIndex);
            _state = FileSelectPreviewState::Creating;
        }

        void beginIconSelectFirst() {
            _sysInfoWindow.forceKill();
            _fileInfo.kill();
            _buttons.kill();
            _backButton.kill();
            _selectedIconIndex = 0U;
            _iconPromptMessage.kill();
            _infoMessage.kill();
            setupMiiSelectLayout();
            _state = FileSelectPreviewState::IconSelecting;
        }

        void beginIconConfirm() {
            _infoMessage.kill();
            _iconPromptMessage.kill();
            setupMiiConfirmIconLayout();
            _miiSelectLayout.kill();
            for (auto& iconLayout : _miiIconLayouts) {
                if (iconLayout != nullptr) {
                    iconLayout->kill();
                }
            }
            _sysInfoWindow.appear(_iconSelectionForExistingFile ? "System_FileSelect005" : "System_FileSelect013", SysInfoWindow::Type_YesNo,
                                  SysInfoWindow::TextPos_Bottom, SysInfoWindow::MessageType_Game);
            _sysInfoWindow.setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_MII_CHANGE", "SE_SY_TALK_SELECT_NO");
            _state = FileSelectPreviewState::IconConfirm;
        }

        void saveSelectedIcon() {
            const int fileIndex = static_cast< int >(_selectedIndex + 1U);
            const u32 iconId = static_cast< u32 >(selectedFellowId()) + 1U;
            GameSequenceFunction::startSetMiiOrIconIdUserFileSequence(fileIndex, nullptr, &iconId);
            _iconSavingExistingFile = _iconSelectionForExistingFile;
            _iconSelectionForExistingFile = false;
            _infoMessage.kill();
            _iconPromptMessage.kill();
            _sysInfoWindow.resetYesNoSelectorSE();
            _sysInfoWindow.forceKill();
            _miiConfirmIconLayout.kill();
            _state = FileSelectPreviewState::IconSaving;
        }

        void beginExistingIconChange() {
            _iconSelectionForExistingFile = true;
            beginIconSelectFirst();
        }

        void beginManual() {
            _buttons.kill();
            _fileInfo.kill();
            _backButton.kill();
            _infoMessage.kill();
            _iconPromptMessage.kill();
            _sysInfoWindow.forceKill();
            hideIconLayouts();
            _manual.appear();
            _state = FileSelectPreviewState::Manual;
        }

        [[nodiscard]] bool shouldUseNearFileSelectSkyCamera() const {
            switch (_state) {
            case FileSelectPreviewState::CreateConfirm:
            case FileSelectPreviewState::Creating:
            case FileSelectPreviewState::IconSelecting:
            case FileSelectPreviewState::IconConfirm:
            case FileSelectPreviewState::IconSaving:
            case FileSelectPreviewState::ExistingInfo:
            case FileSelectPreviewState::ExistingCopySelecting:
            case FileSelectPreviewState::ExistingCopyConfirm:
            case FileSelectPreviewState::ExistingCopying:
            case FileSelectPreviewState::ExistingDeleteConfirm:
            case FileSelectPreviewState::ExistingDeleting:
            case FileSelectPreviewState::Loading:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool shouldUseLiveFileSelectSky() const {
            if (const auto forced = get_optional_bool_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY")) {
                return *forced;
            }

            return false;
        }

        void appendSkyDrawCommands(render::layout::LayoutDrawList* pDrawList, std::uint64_t skyFrame) const {
            if (pDrawList == nullptr) {
                return;
            }

            const bool show_bottom_haze = _state != FileSelectPreviewState::CreateConfirm && _state != FileSelectPreviewState::IconConfirm &&
                                          _state != FileSelectPreviewState::ExistingCopyConfirm &&
                                          _state != FileSelectPreviewState::ExistingDeleteConfirm;
            const bool show_left_comet_streak = _state == FileSelectPreviewState::IconSelecting || _state == FileSelectPreviewState::IconConfirm ||
                                                _state == FileSelectPreviewState::IconSaving;
            const float star_alpha_scale =
                environment_float_or("SMGPC_SHARED_SKY_STAR_ALPHA_SCALE", environment_float_or("SMGPC_FILE_SELECT_STAR_ALPHA_SCALE", 0.82F));
            const float bottom_haze_alpha_scale = environment_float_or("SMGPC_SHARED_SKY_BOTTOM_HAZE_ALPHA_SCALE",
                                                                       environment_float_or("SMGPC_FILE_SELECT_BOTTOM_HAZE_ALPHA_SCALE", 0.85F));
            const float left_comet_alpha_scale = environment_float_or("SMGPC_FILE_SELECT_LEFT_COMET_ALPHA_SCALE", 0.60F);
            const float left_nebula_alpha_scale = environment_float_or("SMGPC_FILE_SELECT_LEFT_NEBULA_ALPHA_SCALE", 1.0F);
            constexpr float ORIGINAL_SKY_YAW_RADIANS_PER_FRAME = 0.001F;
            constexpr float TWO_PI = 6.28318530717958647692F;
            const float star_scroll_u =
                std::fmod(static_cast< float >(skyFrame) * environment_float_or("SMGPC_SHARED_SKY_SCROLL_U", ORIGINAL_SKY_YAW_RADIANS_PER_FRAME / TWO_PI), 1.0F);
            const float star_scroll_v = std::fmod(static_cast< float >(skyFrame) * environment_float_or("SMGPC_SHARED_SKY_SCROLL_V", 0.0F), 1.0F);
            const float star_u_scale =
                environment_float_or("SMGPC_SHARED_SKY_STAR_U_SCALE", environment_float_or("SMGPC_FILE_SELECT_STAR_U_SCALE", 0.816F));
            const float star_v_scale =
                environment_float_or("SMGPC_SHARED_SKY_STAR_V_SCALE", environment_float_or("SMGPC_FILE_SELECT_STAR_V_SCALE", 0.891F));

            if (shouldUseLiveFileSelectSky() && _skyTextures.j3d_sky.has_value()) {
                pDrawList->push_triangle_batch(render::layout::TriangleBatchCommand{
                    .coordinate_width = 836.0F,
                    .coordinate_height = 456.0F,
                    .vertices =
                        {
                            render::layout::TriangleVertex{
                                .x = 0.0F, .y = 0.0F, .u = 0.0F, .v = 0.0F, .color = render::layout::pack_abgr(4U, 67U, 98U, 255U)},
                            render::layout::TriangleVertex{
                                .x = 836.0F, .y = 0.0F, .u = 1.0F, .v = 0.0F, .color = render::layout::pack_abgr(4U, 67U, 98U, 255U)},
                            render::layout::TriangleVertex{
                                .x = 0.0F, .y = 456.0F, .u = 0.0F, .v = 1.0F, .color = render::layout::pack_abgr(0U, 38U, 60U, 255U)},
                            render::layout::TriangleVertex{
                                .x = 836.0F, .y = 0.0F, .u = 1.0F, .v = 0.0F, .color = render::layout::pack_abgr(4U, 67U, 98U, 255U)},
                            render::layout::TriangleVertex{
                                .x = 836.0F, .y = 456.0F, .u = 1.0F, .v = 1.0F, .color = render::layout::pack_abgr(0U, 38U, 60U, 255U)},
                            render::layout::TriangleVertex{
                                .x = 0.0F, .y = 456.0F, .u = 0.0F, .v = 1.0F, .color = render::layout::pack_abgr(0U, 38U, 60U, 255U)},
                        },
                });
                if (_skyTextures.star_field.has_value() && !_skyTextures.star_field->empty()) {
                    const render::layout::TextureRef texture{
                        .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&*_skyTextures.star_field)),
                        .rgba8 = _skyTextures.star_field->rgba8.data(),
                        .width = _skyTextures.star_field->width,
                        .height = _skyTextures.star_field->height,
                        .wrap_s = 1U,
                        .wrap_t = 1U,
                    };
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = 0.0F,
                        .y0 = 0.0F,
                        .x1 = 836.0F,
                        .y1 = 456.0F,
                        .coordinate_width = 836.0F,
                        .coordinate_height = 456.0F,
                        .u0 = star_scroll_u,
                        .v0 = star_scroll_v,
                        .u1 = star_scroll_u + star_u_scale,
                        .v1 = star_scroll_v + star_v_scale,
                        .color_tl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                        .color_tr = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                        .color_bl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                        .color_br = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                        .texture = texture,
                    });
                }
                const bool use_near_sky_camera =
                    shouldUseNearFileSelectSkyCamera() && !get_bool_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_FAR_CAMERA");
                _skyTextures.j3d_sky->appendDrawCommands(pDrawList, static_cast< float >(skyFrame), _selectedIndex, use_near_sky_camera);
                if (show_bottom_haze && _skyTextures.bottom_haze.has_value() && !_skyTextures.bottom_haze->empty()) {
                    const render::layout::TextureRef texture{
                        .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&*_skyTextures.bottom_haze)),
                        .rgba8 = _skyTextures.bottom_haze->rgba8.data(),
                        .width = _skyTextures.bottom_haze->width,
                        .height = _skyTextures.bottom_haze->height,
                    };
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = 0.0F,
                        .y0 = 416.0F,
                        .x1 = 836.0F,
                        .y1 = 456.0F,
                        .coordinate_width = 836.0F,
                        .coordinate_height = 456.0F,
                        .u0 = 0.0F,
                        .v0 = 0.0F,
                        .u1 = 1.0F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 0U),
                        .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 0U),
                        .color_bl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * bottom_haze_alpha_scale)),
                        .color_br = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * bottom_haze_alpha_scale)),
                        .texture = texture,
                    });
                }
                return;
            }

            compat::append_shared_sky_background_draw_commands(pDrawList, _skyTextures, skyFrame,
                                                               compat::SharedSkyBackgroundOptions{.show_bottom_haze = show_bottom_haze});

            if (get_bool_from_environment("SMGPC_FILE_SELECT_J3D_SKY") && _skyTextures.model_snapshot.has_value() &&
                !_skyTextures.model_snapshot->empty()) {
                const render::layout::TextureRef texture{
                    .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&*_skyTextures.model_snapshot)),
                    .rgba8 = _skyTextures.model_snapshot->rgba8.data(),
                    .width = _skyTextures.model_snapshot->width,
                    .height = _skyTextures.model_snapshot->height,
                };
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = 0.0F,
                    .y0 = 0.0F,
                    .x1 = 836.0F,
                    .y1 = 456.0F,
                    .coordinate_width = 836.0F,
                    .coordinate_height = 456.0F,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .texture = texture,
                });
                return;
            }

            if (show_left_comet_streak && _skyTextures.comet_halo.has_value() && !_skyTextures.comet_halo->empty()) {
                const float appear = smooth_step(220.0F, 300.0F, static_cast< float >(_planetAnimationFrame));
                if (appear > 0.0F) {
                    const render::layout::TextureRef texture{
                        .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&*_skyTextures.comet_halo)),
                        .rgba8 = _skyTextures.comet_halo->rgba8.data(),
                        .width = _skyTextures.comet_halo->width,
                        .height = _skyTextures.comet_halo->height,
                    };
                    const std::uint8_t alpha = clamp_u8(appear * 205.0F * left_comet_alpha_scale);
                    const float drift = std::min(58.0F, std::max(0.0F, static_cast< float >(_planetAnimationFrame) - 240.0F) * 0.24F);
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = -96.0F + drift,
                        .y0 = -104.0F,
                        .x1 = 246.0F + drift,
                        .y1 = 336.0F,
                        .use_custom_vertices = true,
                        .x_tl = -58.0F + drift,
                        .y_tl = 42.0F,
                        .x_tr = 208.0F + drift,
                        .y_tr = -104.0F,
                        .x_bl = -96.0F + drift,
                        .y_bl = 336.0F,
                        .x_br = 246.0F + drift,
                        .y_br = 124.0F,
                        .u0 = 0.08F,
                        .v0 = 0.0F,
                        .u1 = 0.86F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_tr = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_bl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_br = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .blend_mode = render::layout::BlendMode::Additive,
                        .texture = texture,
                    });
                }
            }

            if (_state == FileSelectPreviewState::IconConfirm) {
                const float appear = smooth_step(220.0F, 300.0F, static_cast< float >(_planetAnimationFrame));
                if (appear > 0.0F) {
                    const auto alpha = [&](float value) { return clamp_u8(appear * value * left_comet_alpha_scale); };
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = -88.0F,
                        .y0 = -36.0F,
                        .x1 = 96.0F,
                        .y1 = 176.0F,
                        .use_custom_vertices = true,
                        .x_tl = -52.0F,
                        .y_tl = -36.0F,
                        .x_tr = 96.0F,
                        .y_tr = -12.0F,
                        .x_bl = -88.0F,
                        .y_bl = 126.0F,
                        .x_br = 55.0F,
                        .y_br = 176.0F,
                        .u0 = 0.0F,
                        .v0 = 0.0F,
                        .u1 = 1.0F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(200U, 240U, 104U, alpha(190.0F)),
                        .color_tr = render::layout::pack_abgr(248U, 214U, 88U, alpha(136.0F)),
                        .color_bl = render::layout::pack_abgr(92U, 60U, 110U, alpha(8.0F)),
                        .color_br = render::layout::pack_abgr(222U, 42U, 96U, alpha(28.0F)),
                        .blend_mode = render::layout::BlendMode::Additive,
                    });
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = -30.0F,
                        .y0 = 48.0F,
                        .x1 = 96.0F,
                        .y1 = 264.0F,
                        .use_custom_vertices = true,
                        .x_tl = 62.0F,
                        .y_tl = 48.0F,
                        .x_tr = 96.0F,
                        .y_tr = 64.0F,
                        .x_bl = -30.0F,
                        .y_bl = 264.0F,
                        .x_br = 12.0F,
                        .y_br = 250.0F,
                        .u0 = 0.0F,
                        .v0 = 0.0F,
                        .u1 = 1.0F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(240U, 36U, 78U, alpha(34.0F)),
                        .color_tr = render::layout::pack_abgr(230U, 28U, 62U, alpha(20.0F)),
                        .color_bl = render::layout::pack_abgr(210U, 44U, 104U, alpha(26.0F)),
                        .color_br = render::layout::pack_abgr(235U, 34U, 86U, alpha(16.0F)),
                        .blend_mode = render::layout::BlendMode::Additive,
                    });
                }
            }

            if (show_left_comet_streak && _skyTextures.nebula.has_value() && !_skyTextures.nebula->empty()) {
                const float appear = smooth_step(220.0F, 300.0F, static_cast< float >(_planetAnimationFrame));
                if (appear > 0.0F) {
                    const render::layout::TextureRef texture{
                        .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&*_skyTextures.nebula)),
                        .rgba8 = _skyTextures.nebula->rgba8.data(),
                        .width = _skyTextures.nebula->width,
                        .height = _skyTextures.nebula->height,
                    };
                    const std::uint8_t alpha = clamp_u8(appear * 86.0F * left_nebula_alpha_scale);
                    const float drift = std::min(42.0F, std::max(0.0F, static_cast< float >(_planetAnimationFrame) - 240.0F) * 0.22F);
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = -112.0F + drift,
                        .y0 = -82.0F,
                        .x1 = 166.0F + drift,
                        .y1 = 312.0F,
                        .use_custom_vertices = true,
                        .x_tl = -18.0F + drift,
                        .y_tl = -82.0F,
                        .x_tr = 166.0F + drift,
                        .y_tr = -16.0F,
                        .x_bl = -112.0F + drift,
                        .y_bl = 246.0F,
                        .x_br = 104.0F + drift,
                        .y_br = 312.0F,
                        .u0 = 0.0F,
                        .v0 = 0.0F,
                        .u1 = 1.0F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_tr = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_bl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_br = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .blend_mode = render::layout::BlendMode::Additive,
                        .texture = texture,
                    });
                }
            }
        }

        void appendPromptPillDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr || _promptPillTexture.empty()) {
                return;
            }
            if (_state != FileSelectPreviewState::Preloading && _state != FileSelectPreviewState::Selecting &&
                _state != FileSelectPreviewState::ExistingCopySelecting && _state != FileSelectPreviewState::Loading) {
                return;
            }

            const render::layout::TextureRef texture{
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&_promptPillTexture)),
                .rgba8 = _promptPillTexture.rgba8.data(),
                .width = _promptPillTexture.width,
                .height = _promptPillTexture.height,
            };

            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = 151.0F,
                .y0 = 377.0F,
                .x1 = 685.0F,
                .y1 = 419.0F,
                .u0 = 0.0F,
                .v0 = 0.0F,
                .u1 = 1.0F,
                .v1 = 1.0F,
                .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .texture = texture,
            });
        }

        void appendBadgeDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr || _badgeTexture.empty()) {
                return;
            }
            if (!shouldDrawFileSelectItems()) {
                return;
            }

            struct Placement {
                float x;
                float y;
                float size;
            };
            const std::array< Placement, 6 > placements{
                Placement{312.0F, 236.0F, 42.0F}, Placement{522.0F, 236.0F, 42.0F}, Placement{200.0F, 179.0F, 42.0F},
                Placement{633.0F, 179.0F, 42.0F}, Placement{333.0F, 150.0F, 42.0F}, Placement{503.0F, 150.0F, 42.0F},
            };

            const render::layout::TextureRef texture{
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&_badgeTexture)),
                .rgba8 = _badgeTexture.rgba8.data(),
                .width = _badgeTexture.width,
                .height = _badgeTexture.height,
            };

            for (const auto& placement : placements) {
                const float radius = placement.size * 0.5F;
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = placement.x - radius,
                    .y0 = placement.y - radius,
                    .x1 = placement.x + radius,
                    .y1 = placement.y + radius,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .texture = texture,
                });
            }
        }

        void appendIconSelectionDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr || _badgeTexture.empty()) {
                return;
            }
            if (_state != FileSelectPreviewState::IconSelecting) {
                return;
            }

            _miiSelectLayout.appendDrawCommands(pDrawList);
            const std::uint8_t panel_light_alpha = clamp_u8(255.0F * environment_float_or("SMGPC_FILE_SELECT_MII_PANEL_LIGHT_ALPHA_SCALE", 0.35F));
            if (panel_light_alpha > 0U) {
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = 0.0F,
                    .y0 = 76.0F,
                    .x1 = 836.0F,
                    .y1 = 316.0F,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(190U, 225U, 245U, panel_light_alpha),
                    .color_tr = render::layout::pack_abgr(190U, 225U, 245U, panel_light_alpha),
                    .color_bl = render::layout::pack_abgr(190U, 225U, 245U, panel_light_alpha),
                    .color_br = render::layout::pack_abgr(190U, 225U, 245U, panel_light_alpha),
                });
            }
            bool drewMiiIconLayouts = false;
            for (const auto& iconLayout : _miiIconLayouts) {
                if (iconLayout != nullptr && !iconLayout->isDead()) {
                    iconLayout->appendDrawCommands(pDrawList);
                    drewMiiIconLayouts = true;
                }
            }

            if (drewMiiIconLayouts) {
                return;
            }

            const auto fellowIds = selectableFellowIds();
            const render::layout::TextureRef texture{
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&_badgeTexture)),
                .rgba8 = _badgeTexture.rgba8.data(),
                .width = _badgeTexture.width,
                .height = _badgeTexture.height,
            };

            constexpr auto fallbackIconPositions = mii_select_fellow_icon_reference_positions();
            const std::array< const char*, 4 > iconPanes{"Mii01", "Mii02", "Mii03", "Mii04"};
            for (std::size_t i = 0U; i < fellowIds.size(); ++i) {
                const bool selected = i == _selectedIconIndex;
                const float size = selected ? 80.0F : 70.0F;
                const float radius = size * 0.5F;
                bool usedPaneAnchor = false;
                const auto position = miiSelectPaneReferencePosition(iconPanes[i], fallbackIconPositions[i], &usedPaneAnchor);
                const float x = position.first;
                const float y = position.second;
                const std::uint32_t alpha = _state == FileSelectPreviewState::IconSaving ? 140U : 255U;
                const auto fellowIndex = static_cast< std::size_t >(fellowIds[i]);
                const auto* icon = fellowIndex < _fellowIconTextures.fellows.size() && _fellowIconTextures.fellows[fellowIndex].has_value() ?
                                       &*_fellowIconTextures.fellows[fellowIndex] :
                                       nullptr;
                trace_icon_select_quad(iconPanes[i], x, y, selected ? 46.0F : 41.0F, icon, usedPaneAnchor, _planetAnimationFrame);

                if (icon != nullptr && !icon->empty()) {
                    const render::layout::TextureRef icon_texture{
                        .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(icon)),
                        .rgba8 = icon->rgba8.data(),
                        .width = icon->width,
                        .height = icon->height,
                    };
                    const float iconRadius = (selected ? 92.0F : 82.0F) * 0.5F;
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = x - iconRadius,
                        .y0 = y - iconRadius,
                        .x1 = x + iconRadius,
                        .y1 = y + iconRadius,
                        .u0 = 0.0F,
                        .v0 = 0.0F,
                        .u1 = 1.0F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_tr = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_bl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_br = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .texture = icon_texture,
                    });
                } else {
                    pDrawList->push_quad(render::layout::QuadCommand{
                        .x0 = x - radius,
                        .y0 = y - radius,
                        .x1 = x + radius,
                        .y1 = y + radius,
                        .u0 = 0.0F,
                        .v0 = 0.0F,
                        .u1 = 1.0F,
                        .v1 = 1.0F,
                        .color_tl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_tr = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_bl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .color_br = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                        .texture = texture,
                    });
                }
            }

            const float miiSize = 54.0F;
            const float miiRadius = miiSize * 0.5F;
            bool usedMiiPaneAnchor = false;
            const auto miiPosition = miiSelectPaneReferencePosition("Mii05", mii_select_dummy_icon_reference_position(), &usedMiiPaneAnchor);
            const float miiX = miiPosition.first;
            const float miiY = miiPosition.second;
            const std::uint32_t miiAlpha = _state == FileSelectPreviewState::IconSaving ? 120U : 180U;
            trace_icon_select_quad("Mii05", miiX, miiY, 42.0F,
                                   _fellowIconTextures.mii_placeholder.has_value() ? &*_fellowIconTextures.mii_placeholder : nullptr,
                                   usedMiiPaneAnchor, _planetAnimationFrame);
            if (_fellowIconTextures.mii_placeholder.has_value() && !_fellowIconTextures.mii_placeholder->empty()) {
                const render::layout::TextureRef mii_texture{
                    .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&*_fellowIconTextures.mii_placeholder)),
                    .rgba8 = _fellowIconTextures.mii_placeholder->rgba8.data(),
                    .width = _fellowIconTextures.mii_placeholder->width,
                    .height = _fellowIconTextures.mii_placeholder->height,
                };
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = miiX - 42.0F,
                    .y0 = miiY - 42.0F,
                    .x1 = miiX + 42.0F,
                    .y1 = miiY + 42.0F,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(255U, 255U, 255U, miiAlpha),
                    .color_tr = render::layout::pack_abgr(255U, 255U, 255U, miiAlpha),
                    .color_bl = render::layout::pack_abgr(255U, 255U, 255U, miiAlpha),
                    .color_br = render::layout::pack_abgr(255U, 255U, 255U, miiAlpha),
                    .texture = mii_texture,
                });
            } else {
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = miiX - miiRadius,
                    .y0 = miiY - miiRadius,
                    .x1 = miiX + miiRadius,
                    .y1 = miiY + miiRadius,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(180U, 190U, 200U, miiAlpha),
                    .color_tr = render::layout::pack_abgr(180U, 190U, 200U, miiAlpha),
                    .color_bl = render::layout::pack_abgr(116U, 124U, 132U, miiAlpha),
                    .color_br = render::layout::pack_abgr(116U, 124U, 132U, miiAlpha),
                    .texture = texture,
                });
            }
        }

        void appendMiiConfirmIconDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr) {
                return;
            }
            if (_state == FileSelectPreviewState::IconConfirm || _state == FileSelectPreviewState::IconSaving) {
                _miiConfirmIconLayout.appendDrawCommands(pDrawList);
            }
        }

        void appendPointerDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr) {
                return;
            }

            if (_pointerLayout.getResource() != nullptr && !_pointerLayout.isDead()) {
                _pointerLayout.appendDrawCommands(pDrawList);
                return;
            }

            const auto push_texture = [pDrawList](const assets::layout::tpl::DecodedImage& image, float x, float y, float size, std::uint32_t color) {
                if (image.empty()) {
                    return;
                }

                const render::layout::TextureRef texture{
                    .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&image)),
                    .rgba8 = image.rgba8.data(),
                    .width = image.width,
                    .height = image.height,
                };
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = x,
                    .y0 = y,
                    .x1 = x + size,
                    .y1 = y + size,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = color,
                    .color_tr = color,
                    .color_bl = color,
                    .color_br = color,
                    .texture = texture,
                });
            };

            const auto handTopLeft = pointerHandTopLeftReferencePosition();
            const float handX = handTopLeft.first;
            const float handY = handTopLeft.second;

            if (_pointerTextures.hand_shadow.has_value()) {
                push_texture(*_pointerTextures.hand_shadow, handX + 4.0F, handY + 4.0F, 58.0F, render::layout::pack_abgr(0U, 0U, 0U, 120U));
            }
            if (_pointerTextures.hand.has_value()) {
                push_texture(*_pointerTextures.hand, handX, handY, 58.0F, render::layout::pack_abgr(255U, 255U, 255U, 255U));
            }
            if (_pointerTextures.star.has_value()) {
                push_texture(*_pointerTextures.star, handX + 18.0F, handY + 36.0F, 27.0F, render::layout::pack_abgr(70U, 210U, 255U, 255U));
            }
        }

        void appendPlanetDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr || _planetTextures.empty()) {
                return;
            }
            if (!shouldDrawFileSelectItems()) {
                return;
            }

            struct Placement {
                float x;
                float y;
                float size;
            };
            const std::array< Placement, 6 > placements{
                Placement{320.0F, 303.0F, 134.0F}, Placement{520.0F, 303.0F, 134.0F}, Placement{204.0F, 220.0F, 82.0F},
                Placement{632.0F, 220.0F, 82.0F},  Placement{337.0F, 184.0F, 82.0F},  Placement{501.0F, 184.0F, 82.0F},
            };
            const float large_x_offset = environment_float_or("SMGPC_FILE_SELECT_PLANET_LARGE_X_OFFSET", 0.0F);
            const float large_y_offset = environment_float_or("SMGPC_FILE_SELECT_PLANET_LARGE_Y_OFFSET", 0.0F);
            const float large_scale = environment_float_or("SMGPC_FILE_SELECT_PLANET_LARGE_SCALE", 1.0F);
            const float large_x_spread = environment_float_or("SMGPC_FILE_SELECT_PLANET_LARGE_X_SPREAD", 1.0F);
            const float small_x_offset = environment_float_or("SMGPC_FILE_SELECT_PLANET_SMALL_X_OFFSET", 0.0F);
            const float small_y_offset = environment_float_or("SMGPC_FILE_SELECT_PLANET_SMALL_Y_OFFSET", 0.0F);
            const float small_scale = environment_float_or("SMGPC_FILE_SELECT_PLANET_SMALL_SCALE", 1.0F);
            constexpr float center_x = 420.0F;

            for (std::size_t i = 0U; i < placements.size(); ++i) {
                const auto& placement = placements[i];
                const auto& planetTexture = _planetTextures[(_planetAnimationFrame / 3U + 1U + i * 5U) % _planetTextures.size()];
                const render::layout::TextureRef texture{
                    .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&planetTexture)),
                    .rgba8 = planetTexture.rgba8.data(),
                    .width = planetTexture.width,
                    .height = planetTexture.height,
                };
                const bool large = i < 2U;
                const float x_offset = large ? large_x_offset : small_x_offset;
                const float y_offset = large ? large_y_offset : small_y_offset;
                const float scale = large ? large_scale : small_scale;
                const float radius = placement.size * scale * 0.5F;
                const float placement_x = large ? center_x + (placement.x - center_x) * large_x_spread : placement.x;
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = placement_x + x_offset - radius,
                    .y0 = placement.y + y_offset - radius,
                    .x1 = placement_x + x_offset + radius,
                    .y1 = placement.y + y_offset + radius,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .texture = texture,
                });
            }
        }

        void appendCreatedSlotIconDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr || !shouldDrawFileSelectItems()) {
                return;
            }

            constexpr auto placements = created_slot_icon_placements();

            for (std::size_t slotIndex = 0U; slotIndex < _userFiles.size(); ++slotIndex) {
                if (_state == FileSelectPreviewState::ExistingCopySelecting && _copySourceIndex.has_value() && *_copySourceIndex == slotIndex) {
                    continue;
                }
                if (!_slotCreated[slotIndex] || _userFiles[slotIndex] == nullptr) {
                    continue;
                }

                const auto* icon = iconTextureForSlot(slotIndex);
                if (icon == nullptr || icon->empty()) {
                    continue;
                }

                const auto& placement = placements[slotIndex];
                const float radius = placement.size * 0.5F;
                const render::layout::TextureRef texture{
                    .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(icon)),
                    .rgba8 = icon->rgba8.data(),
                    .width = icon->width,
                    .height = icon->height,
                };
                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = placement.x - radius,
                    .y0 = placement.y - radius,
                    .x1 = placement.x + radius,
                    .y1 = placement.y + radius,
                    .u0 = 0.0F,
                    .v0 = 0.0F,
                    .u1 = 1.0F,
                    .v1 = 1.0F,
                    .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                    .texture = texture,
                });
            }
        }

        void appendCopySourceHighlightDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
            if (pDrawList == nullptr || _state != FileSelectPreviewState::ExistingCopySelecting || !_copySourceIndex.has_value()) {
                return;
            }

            const auto slotIndex = *_copySourceIndex;
            if (slotIndex >= _slotCreated.size() || !_slotCreated[slotIndex] || _copySourceGlowTexture.empty()) {
                return;
            }

            const auto* icon = iconTextureForSlot(slotIndex);
            if (icon == nullptr || icon->empty()) {
                return;
            }

            constexpr auto placements = created_slot_icon_placements();
            const auto& placement = placements[slotIndex];
            const float iconSize = std::max(76.0F, placement.size * 1.15F);
            const float iconRadius = iconSize * 0.5F;
            const float glowPulse = 1.0F + std::sin(static_cast< float >(_planetAnimationFrame) * 0.075F) * 0.045F;
            const float glowRadius = iconSize * 0.82F * glowPulse;

            const render::layout::TextureRef glowTexture{
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&_copySourceGlowTexture)),
                .rgba8 = _copySourceGlowTexture.rgba8.data(),
                .width = _copySourceGlowTexture.width,
                .height = _copySourceGlowTexture.height,
            };
            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = placement.x - glowRadius,
                .y0 = placement.y - glowRadius,
                .x1 = placement.x + glowRadius,
                .y1 = placement.y + glowRadius,
                .u0 = 0.0F,
                .v0 = 0.0F,
                .u1 = 1.0F,
                .v1 = 1.0F,
                .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .blend_mode = render::layout::BlendMode::Additive,
                .texture = glowTexture,
            });

            const render::layout::TextureRef iconTexture{
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(icon)),
                .rgba8 = icon->rgba8.data(),
                .width = icon->width,
                .height = icon->height,
            };
            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = placement.x - iconRadius,
                .y0 = placement.y - iconRadius,
                .x1 = placement.x + iconRadius,
                .y1 = placement.y + iconRadius,
                .u0 = 0.0F,
                .v0 = 0.0F,
                .u1 = 1.0F,
                .v1 = 1.0F,
                .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_bl = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .color_br = render::layout::pack_abgr(255U, 255U, 255U, 255U),
                .texture = iconTexture,
            });
        }

        [[nodiscard]] const assets::layout::tpl::DecodedImage* iconTextureForSlot(std::size_t slotIndex) const {
            if (slotIndex >= _userFiles.size() || _userFiles[slotIndex] == nullptr) {
                return nullptr;
            }

            u32 iconId = 0U;
            if (_userFiles[slotIndex]->getIconId(&iconId) && iconId >= 1U && iconId <= _fellowIconTextures.fellows.size()) {
                const auto fellowIndex = static_cast< std::size_t >(iconId - 1U);
                if (_fellowIconTextures.fellow_models[fellowIndex].has_value()) {
                    return &*_fellowIconTextures.fellow_models[fellowIndex];
                }
                if (_fellowIconTextures.fellows[fellowIndex].has_value()) {
                    return &*_fellowIconTextures.fellows[fellowIndex];
                }
            } else if (_fellowIconTextures.mii_placeholder.has_value()) {
                return &*_fellowIconTextures.mii_placeholder;
            }

            return nullptr;
        }

        FileSelectButton _buttons;
        FileSelectInfo _fileInfo;
        Manual2P _manual;
        BackButton _backButton;
        LayoutActor _miiSelectLayout;
        LayoutActor _miiConfirmIconLayout;
        LayoutActor _pointerLayout;
        SaveDataHandleSequence _saveSequence{};
        InformationMessage _infoMessage;
        InformationMessage _iconPromptMessage;
        SysInfoWindow _sysInfoWindow{SysInfoWindow::WindowType_Normal, SysInfoWindow::ExecuteType_Normal};
        file_select_preview::SkyTextures _skyTextures{};
        file_select_preview::PointerTextures _pointerTextures{};
        file_select_preview::FellowIconTextures _fellowIconTextures{};
        std::array< std::unique_ptr< nw4r::lyt::TexMap >, 5 > _fellowIconTexMaps{};
        std::unique_ptr< nw4r::lyt::TexMap > _miiPlaceholderTexMap{};
        std::array< std::unique_ptr< LayoutActor >, 5 > _miiIconLayouts{};
        file_select_preview::MiiSelectTextures _miiSelectTextures{};
        std::vector< assets::layout::tpl::DecodedImage > _planetTextures{};
        assets::layout::tpl::DecodedImage _badgeTexture{};
        assets::layout::tpl::DecodedImage _promptPillTexture{};
        assets::layout::tpl::DecodedImage _pageCounterTexture{};
        assets::layout::tpl::DecodedImage _copySourceGlowTexture{};
        std::array< std::unique_ptr< UserFile >, 6 > _userFiles{};
        std::array< bool, 6 > _slotCreated{};
        std::array< std::unique_ptr< FileSelectNumber >, 6 > _numbers{};
        FileSelectPreviewState _state{FileSelectPreviewState::Preloading};
        std::size_t _selectedIndex{};
        std::size_t _selectedIconIndex{};
        bool _wasConfirmPressed{};
        std::uint32_t _autoConfirmRemaining{};
        std::uint64_t _planetAnimationFrame{};
        bool _endRequested{};
        bool _iconSelectionForExistingFile{};
        bool _iconSavingExistingFile{};
        std::optional< std::size_t > _copySourceIndex{};
        std::optional< std::size_t > _copyDestinationIndex{};
        FileSelectPreviewCompletion _completion{FileSelectPreviewCompletion::None};
    };

    FileSelectPreview::FileSelectPreview() : mImpl(std::make_unique< Impl >()) {
    }

    FileSelectPreview::~FileSelectPreview() = default;
    FileSelectPreview::FileSelectPreview(FileSelectPreview&&) noexcept = default;
    FileSelectPreview& FileSelectPreview::operator=(FileSelectPreview&&) noexcept = default;

    void FileSelectPreview::appear() {
        mImpl->appear();
    }

    void FileSelectPreview::movement() {
        mImpl->movement();
    }

    void FileSelectPreview::appendDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
        mImpl->appendDrawCommands(pDrawList, mImpl->skyFrame());
    }

    void FileSelectPreview::appendDrawCommands(render::layout::LayoutDrawList* pDrawList, std::uint64_t skyFrame) const {
        mImpl->appendDrawCommands(pDrawList, skyFrame);
    }

    bool FileSelectPreview::isEnd() const {
        return mImpl->isEnd();
    }

    FileSelectPreviewCompletion FileSelectPreview::completion() const {
        return mImpl->completion();
    }

    const LayoutActor* FileSelectPreview::layoutForSize() const {
        return mImpl->layoutForSize();
    }

}  // namespace smgpc::game
