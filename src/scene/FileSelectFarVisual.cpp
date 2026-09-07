#include "scene/FileSelectFarVisual.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/FileSelectCameraController.hpp"
#include "Game/Screen/FileSelectNumber.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/StageLightSceneBinding.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace smgpc::scene {
    namespace {

        constexpr auto cFileNumbers = std::array<s32, 6U>{1, 2, 4, 6, 5, 3};
        constexpr auto cThetaOffsetsDegrees =
            std::array<f32, 6U>{10.0F, -10.0F, 0.0F, 0.0F, 0.0F, 0.0F};
        constexpr auto cPi = 3.1415927410125732F;
        constexpr auto cThetaStep = 1.0471975803375244F;
        constexpr auto cSlope = 0.3490658700466156F;
        constexpr auto cItemRadius = 5000.0F;
        constexpr auto cPlanetOffset = 900.0F;
        constexpr auto cNumberOffset = 2150.0F;
        constexpr auto cFarObservationsBeforeNumbers = 3U;
        constexpr auto cPlanetScale = 30.0F;
        constexpr auto cHighlightedScaleFactor = 1.2F;
        constexpr auto cNormalScaleFactor = 1.0F;
        constexpr auto cScaleTransitionSteps = 30;

        [[nodiscard]] TVec3f authored_slot_position(std::size_t index) {
            const auto theta =
                -static_cast<f32>(index + 4U) * cThetaStep -
                cThetaOffsetsDegrees[index] * cPi / 180.0F;
            const auto ring_x = cItemRadius * JMACosRadian(theta);
            const auto ring_z = cItemRadius * JMASinRadian(theta);
            return TVec3f{
                ring_x,
                -JMASinRadian(cSlope) * ring_z,
                JMACosRadian(cSlope) * ring_z,
            };
        }

        class FileSelectFarSlotHost final : public LiveActor {
        public:
            explicit FileSelectFarSlotHost(const TVec3f &base_position)
                : LiveActor("ファイルセレクトアイテム"),
                  number_anchor(base_position.x,
                                base_position.y + cNumberOffset,
                                base_position.z) {
                mPosition.set(base_position);
                planet_matrix.makeTrans(base_position.x,
                                        base_position.y + cPlanetOffset,
                                        base_position.z);
                makeActorAppeared();
            }

            TPos3f planet_matrix{};
            TVec3f number_anchor{};
        };

    }  // namespace

    struct FileSelectFarVisual::Impl final {
        enum class ScaleState {
            Small,
            ToBig,
            Big,
            ToSmall,
        };

        struct SlotRuntime final {
            FileSelectFarSlotHost *host = nullptr;
            FileSelectFarSlotView view{};
            f32 scale_factor = cNormalScaleFactor;
            s32 scale_step = 0;
            ScaleState scale_state = ScaleState::Small;
        };

        class MovementDriver final : public NameObj {
        public:
            explicit MovementDriver(Impl &owner)
                : NameObj("ファイルセレクト遠景表示制御"), _owner(&owner) {
            }

            void init(const JMapInfoIter &) override {
                MR::connectToSceneMapObjMovement(this);
            }

            void movement() override {
                _owner->update_after_camera();
            }

        private:
            Impl *_owner;
        };

        explicit Impl(smgpc::runtime::RuntimeContext &runtime)
            : _runtime(&runtime),
              _placement_tables(resolve_stage_placement_tables(
                  runtime.dvd(), "FileSelect", 1)),
              _stage_light(runtime.dvd(), "FileSelect", _placement_tables) {
            try {
                create_camera();
                create_slots();
                auto driver = std::make_unique<MovementDriver>(*this);
                _driver = &_children.adopt(std::move(driver));
                _driver->initWithoutIter();
                update_number_positions();
            } catch (...) {
                end_camera_if_started();
                throw;
            }
        }

        ~Impl() {
            end_camera_if_started();
        }

        void create_camera() {
            auto camera = std::make_unique<FileSelectCameraController>(
                "ファイルセレクトカメラ制御");
            _camera = &_children.adopt(std::move(camera));
            _camera->initWithoutIter();
            _camera->appear();
            _camera_started = true;

            // FileSelector creates this controller during init. Its actual
            // Title nerve now advances through the ordinary scene scheduler.
        }

        void create_slots() {
            // Keep calcBasePos(0)'s authored far ring stable. The current
            // decomp FileSelector::control has a cumulative +1000 Y write on
            // every update; that is not an authored placement transform and
            // copying it here would make a general scene owner drift forever.
            for (auto index = std::size_t{}; index < _slots.size(); ++index) {
                const auto base_position = authored_slot_position(index);
                auto host = std::make_unique<FileSelectFarSlotHost>(base_position);
                auto *host_identity = host.get();
                _children.adopt(std::move(host));

                auto *planet = MR::createPartsModelMapObj(
                    host_identity, "ニューフェイス", "FileSelectDataPlanet",
                    host_identity->planet_matrix.toMtxPtr());
                _children.adopt(planet);
                planet->mScale.set(cPlanetScale);
                planet->makeActorDead();

                auto number = std::make_unique<FileSelectNumber>("ファイル番号");
                auto *number_identity = number.get();
                _children.adopt(std::move(number));
                number_identity->initWithoutIter();
                number_identity->setNumber(cFileNumbers[index]);

                _slots[index] = SlotRuntime{
                    .host = host_identity,
                    .view = FileSelectFarSlotView{
                        .file_number = cFileNumbers[index],
                        .base_position = base_position,
                        .planet = planet,
                        .number = number_identity,
                        .planet_matrix = &host_identity->planet_matrix,
                    },
                };
            }
        }

        void begin_far() {
            if (_far_started) throw std::logic_error("File Select far transition has already started");
            _camera->goToFarPoint();
            for (auto& slot : _slots) slot.view.planet->makeActorAppeared();
            _far_started = true;
        }

        void update_after_camera() {
            if (!_far_started) return;
            update_slot_scales();
            update_number_positions();
            if (!_camera->isAtFarPoint()) {
                _far_observation_count = 0U;
                return;
            }
            if (_numbers_visible) {
                return;
            }

            ++_far_observation_count;
            if (_far_observation_count < cFarObservationsBeforeNumbers) {
                return;
            }
            for (auto &slot : _slots) {
                slot.view.number->appear();
            }
            _numbers_visible = true;
        }

        void set_highlighted_slot(
            std::optional<std::size_t> highlighted_slot) {
            if (highlighted_slot.has_value() &&
                *highlighted_slot >= _slots.size()) {
                throw std::out_of_range(
                    "File Select highlighted slot is outside the authored six slots.");
            }
            if (highlighted_slot == _highlighted_slot) {
                return;
            }

            if (_highlighted_slot.has_value()) {
                auto &prior = _slots[*_highlighted_slot];
                prior.view.number->onSelectOut();
                begin_scale_transition(prior, ScaleState::ToSmall);
            }
            if (highlighted_slot.has_value()) {
                auto &selected = _slots[*highlighted_slot];
                selected.view.number->onSelectIn();
                begin_scale_transition(selected, ScaleState::ToBig);
            }
            _highlighted_slot = highlighted_slot;
        }

        void begin_scale_transition(SlotRuntime &slot, ScaleState state) {
            slot.scale_state = state;
            slot.scale_step = 0;
        }

        void update_slot_scales() {
            for (auto &slot : _slots) {
                switch (slot.scale_state) {
                case ScaleState::Small:
                    slot.scale_factor = cNormalScaleFactor;
                    break;
                case ScaleState::Big:
                    slot.scale_factor = cHighlightedScaleFactor;
                    break;
                case ScaleState::ToBig:
                    update_slot_scale_transition(
                        slot, cHighlightedScaleFactor, ScaleState::Big);
                    break;
                case ScaleState::ToSmall:
                    update_slot_scale_transition(
                        slot, cNormalScaleFactor, ScaleState::Small);
                    break;
                }
                slot.view.planet->mScale.setAll<f32>(
                    cPlanetScale * slot.scale_factor);
            }
        }

        void update_slot_scale_transition(SlotRuntime &slot, f32 target,
                                          ScaleState target_state) {
            // FileSelectItemSub::ScaleController::exeToBig/exeToSmall uses
            // the current nerve step as a progressively stronger convergence
            // rate. Keep the step-zero sample and the exact step-30 snap so a
            // mid-transition selection change also starts from its live scale.
            if (slot.scale_step <= cScaleTransitionSteps) {
                const auto rate =
                    static_cast<f32>(slot.scale_step) /
                    static_cast<f32>(cScaleTransitionSteps);
                slot.scale_factor += rate * (target - slot.scale_factor);
            }
            if (slot.scale_step == cScaleTransitionSteps) {
                slot.scale_state = target_state;
                slot.scale_step = 0;
                return;
            }
            ++slot.scale_step;
        }

        void update_number_positions() {
            for (auto &slot : _slots) {
                auto screen_position = TVec2f{};
                (void)MR::calcScreenPosition(
                    &screen_position, slot.host->number_anchor);
                slot.view.number->setTrans(screen_position);
            }
        }

        void end_camera_if_started() noexcept {
            if (!_camera_started || _camera == nullptr) {
                return;
            }
            if (smgpc::runtime::RuntimeContext::try_instance() == _runtime) {
                _camera->kill();
            }
            _camera_started = false;
        }

        [[nodiscard]] std::span<const FileSelectFarSlotView> slot_views() const {
            // SlotRuntime intentionally begins with its public view's host
            // metadata, but it is not layout-compatible with a view span.
            // Keep a compact read-only projection for callers instead.
            for (auto index = std::size_t{}; index < _slots.size(); ++index) {
                _slot_views[index] = _slots[index].view;
            }
            return _slot_views;
        }

        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        std::vector<StagePlacementTable> _placement_tables{};
        StageLightSceneBinding _stage_light;
        NameObjChildOwner _children{};
        std::array<SlotRuntime, 6U> _slots{};
        mutable std::array<FileSelectFarSlotView, 6U> _slot_views{};
        FileSelectCameraController *_camera = nullptr;
        MovementDriver *_driver = nullptr;
        std::size_t _far_observation_count = 0U;
        bool _camera_started = false;
        bool _numbers_visible = false;
        bool _far_started = false;
        std::optional<std::size_t> _highlighted_slot{};
    };

    FileSelectFarVisual::FileSelectFarVisual(smgpc::runtime::RuntimeContext &runtime)
        : _runtime(&runtime) {
        if (smgpc::runtime::RuntimeContext::try_instance() != &runtime)
            throw std::logic_error("File Select composition requires its active RuntimeContext");
        if (runtime.current_stage_name() != std::string_view("FileSelect"))
            throw std::logic_error("File Select composition requires the authored stage");
        _impl = std::make_unique<Impl>(runtime);
    }

    void FileSelectFarVisual::begin_far(TitleFileSelectVisualHandoff &&handoff) {
        if (_handoff || handoff.sky() == nullptr)
            throw std::logic_error("Far transition requires its original title sky exactly once");
        _handoff = std::make_unique<TitleFileSelectVisualHandoff>(std::move(handoff));
        _impl->begin_far();
    }

    FileSelectFarVisual::~FileSelectFarVisual() = default;

    FileSelectSky *FileSelectFarVisual::sky() {
        return _handoff ? _handoff->sky() : nullptr;
    }

    const FileSelectSky *FileSelectFarVisual::sky() const {
        return _handoff ? _handoff->sky() : nullptr;
    }

    FileSelectCameraController *FileSelectFarVisual::camera_controller() {
        return _impl->_camera;
    }

    const FileSelectCameraController *
    FileSelectFarVisual::camera_controller() const {
        return _impl->_camera;
    }

    bool FileSelectFarVisual::is_at_far_point() const {
        return _impl->_camera->isAtFarPoint();
    }

    bool FileSelectFarVisual::numbers_visible() const {
        return _impl->_numbers_visible;
    }

    std::span<const FileSelectFarSlotView> FileSelectFarVisual::slots() const {
        return _impl->slot_views();
    }

    void FileSelectFarVisual::set_highlighted_slot(
        std::optional<std::size_t> slot_index) {
        _impl->set_highlighted_slot(slot_index);
    }

    std::optional<std::size_t> FileSelectFarVisual::highlighted_slot() const {
        return _impl->_highlighted_slot;
    }

}  // namespace smgpc::scene
