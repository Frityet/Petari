#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/LiveActor/ShadowVolumeLine.hpp"
#include "Game/LiveActor/ShadowVolumeSphere.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/ActorShadowCsvCompat.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "../aurora/lib/dolphin/gx/__gx.h"
#include "../aurora/lib/gx/fifo.hpp"

#include <aurora/allocation.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include <vector>

#ifndef NDEBUG
namespace {
    void require(bool value, std::string_view message) {
        if (!value) throw std::runtime_error(std::string(message));
    }
    void near(float actual, float expected, const char* message) {
        require(std::isfinite(actual) && std::abs(actual - expected) < 0.0001F, message);
    }
    void write_be16(std::vector< std::uint8_t >& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast< std::uint8_t >(value >> 8U);
        bytes[offset + 1U] = static_cast< std::uint8_t >(value);
    }

    void write_be32(std::vector< std::uint8_t >& bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast< std::uint8_t >(value >> 24U);
        bytes[offset + 1U] = static_cast< std::uint8_t >(value >> 16U);
        bytes[offset + 2U] = static_cast< std::uint8_t >(value >> 8U);
        bytes[offset + 3U] = static_cast< std::uint8_t >(value);
    }

    void write_be_float(std::vector< std::uint8_t >& bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast< std::uint32_t >(value));
    }

    void write_bcsv_field(std::vector< std::uint8_t >& bytes, std::size_t index, std::string_view name, std::uint16_t offset,
                          smgpc::resource::BcsvFieldType type) {
        const auto descriptor = 0x10U + index * 0x0cU;
        write_be32(bytes, descriptor, smgpc::resource::jmap_hash(name));
        write_be32(bytes, descriptor + 0x04U, 0xffffffffU);
        write_be16(bytes, descriptor + 0x08U, offset);
        bytes[descriptor + 0x0aU] = 0U;
        bytes[descriptor + 0x0bU] = static_cast< std::uint8_t >(type);
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_single_file_rarc(std::string_view file_name, const std::vector< std::uint8_t >& file_data) {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto info_offset = std::size_t{0x20U};
        constexpr auto directory_offset = std::size_t{0x40U};
        constexpr auto file_entry_offset = std::size_t{0x50U};
        constexpr auto string_table_offset = std::size_t{0x64U};
        constexpr auto file_data_offset = std::size_t{0x100U};
        require(string_table_offset + file_name.size() + 1U <= file_data_offset, "test RARC filename must fit before data");
        auto bytes = std::vector< std::uint8_t >(file_data_offset + file_data.size(), 0U);
        write_be32(bytes, 0x00U, 0x52415243U);
        write_be32(bytes, 0x04U, static_cast< std::uint32_t >(bytes.size()));
        write_be32(bytes, 0x08U, header_size);
        write_be32(bytes, 0x0cU, file_data_offset - header_size);
        write_be32(bytes, 0x10U, static_cast< std::uint32_t >(file_data.size()));
        write_be32(bytes, info_offset + 0x00U, 1U);
        write_be32(bytes, info_offset + 0x04U, directory_offset - info_offset);
        write_be32(bytes, info_offset + 0x08U, 1U);
        write_be32(bytes, info_offset + 0x0cU, file_entry_offset - info_offset);
        write_be32(bytes, info_offset + 0x10U, static_cast< std::uint32_t >(file_name.size() + 1U));
        write_be32(bytes, info_offset + 0x14U, string_table_offset - info_offset);
        write_be16(bytes, directory_offset + 0x0aU, 1U);
        write_be32(bytes, directory_offset + 0x0cU, 0U);
        write_be16(bytes, file_entry_offset + 0x00U, 0U);
        write_be16(bytes, file_entry_offset + 0x02U, smgpc::resource::RarcArchive::hash_name(file_name));
        bytes[file_entry_offset + 0x04U] = 1U;
        write_be32(bytes, file_entry_offset + 0x08U, 0U);
        write_be32(bytes, file_entry_offset + 0x0cU, static_cast< std::uint32_t >(file_data.size()));
        std::copy(file_name.begin(), file_name.end(), bytes.begin() + string_table_offset);
        std::copy(file_data.begin(), file_data.end(), bytes.begin() + file_data_offset);
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }


    // Only authored name/type/endpoint columns: absent volume offsets must
    // take the CSV default, independently of programmatic constructor defaults.
    smgpc::resource::RarcArchive csv_archive() {
        constexpr std::array fields{"Name", "Type", "LineStart", "LineEnd"};
        constexpr std::array rows{
            std::array{"base", "VolumeSphere", "", ""},
            std::array{"line", "VolumeLine", "base", "line"},
            std::array{"unresolved", "VolumeLine", "later", "base"},
            std::array{"later", "VolumeSphere", "", ""},
        };
        constexpr std::size_t offset = 0x10 + fields.size() * 12;
        constexpr std::size_t stride = fields.size() * 4;
        std::vector<u8> data(offset + stride * rows.size(), 0);
        std::vector<u8> strings;
        write_be32(data, 0, rows.size());
        write_be32(data, 4, fields.size());
        write_be32(data, 8, offset);
        write_be32(data, 12, stride);
        for (std::size_t column = 0; column < fields.size(); ++column)
            write_bcsv_field(data, column, fields[column], column * 4, smgpc::resource::BcsvFieldType::StringOffset);
        for (std::size_t row = 0; row < rows.size(); ++row)
            for (std::size_t column = 0; column < fields.size(); ++column) {
                write_be32(data, offset + row * stride + column * 4, strings.size());
                const std::string_view value = rows[row][column];
                strings.insert(strings.end(), value.begin(), value.end());
                strings.push_back(0);
            }
        data.insert(data.end(), strings.begin(), strings.end());
        return make_single_file_rarc("Shadow.bcsv", data);
    }

    std::vector<u8> shape_bytes(const ShadowVolumeLine& line) {
        const smgpc::compat::J3dCommandScope commands;
        alignas(32) std::array<u8, 4096> bytes{};
        const auto save_context = __gx->dlSaveContext;
        __gx->dlSaveContext = 1;
        GXBeginDisplayList(bytes.data(), bytes.size());
        line.loadModelDrawMtx();
        __GXSetDirtyState();
        if (__gx->vNum != 0 && __gx->bpSent != 0) __GXSendFlushPrim();
        const auto start = aurora::gx::fifo::detail::sDlWritePos;
        line.drawShape();
        const auto end = aurora::gx::fifo::detail::sDlWritePos;
        const auto size = GXEndDisplayList();
        __gx->dlSaveContext = save_context;
        require(size >= end && end >= start, "Original shape commands fit the actual GX display-list recording");
        return {bytes.begin() + start, bytes.begin() + end};
    }

    void verify_shape(const ShadowVolumeLine& line, bool rotated) {
        // Two quads plus the original ten-vertex strip, for analytically chosen
        // endpoints (0,0,0)/(100,0,0), widths 5/10, drop lengths 20/30.
        const std::array<TVec3f, 8> down{{{0,0,-5}, {0,0,5}, {0,-25,-5}, {0,-25,5},
                                         {100,0,-10}, {100,0,10}, {100,-40,-10}, {100,-40,10}}};
        const std::array<TVec3f, 8> back{{{0,5,0}, {0,-5,0}, {0,5,-25}, {0,-5,-25},
                                         {100,10,0}, {100,-10,0}, {100,10,-40}, {100,-10,-40}}};
        const auto& points = rotated ? back : down;
        const auto bytes = shape_bytes(line);
        std::size_t cursor = 0;
        const auto primitive = [&](u8 opcode, std::initializer_list<unsigned> indices) {
            require(cursor + 3 + indices.size() * 12 <= bytes.size(), "Original shape retains every primitive payload");
            require(bytes[cursor++] == opcode && bytes[cursor++] == 0 && bytes[cursor++] == indices.size(),
                    "Original line emits two quads and the ten-vertex closing strip");
            for (const auto index : indices)
                for (const auto expected : {points[index].x, points[index].y, points[index].z}) {
                    const u32 word = u32(bytes[cursor]) << 24 | u32(bytes[cursor+1]) << 16 |
                                     u32(bytes[cursor+2]) << 8 | bytes[cursor+3];
                    cursor += 4;
                    near(std::bit_cast<float>(word), expected, "Original shape emits the analytic endpoint extrusion in big-endian GX floats");
                }
        };
        primitive(GX_QUADS, {1,5,7,3});
        primitive(GX_QUADS, {0,2,6,4});
        primitive(GX_TRIANGLESTRIP, {0,1,2,3,6,7,4,5,0,1});
        require(cursor == bytes.size(), "Original shape emits exactly eighteen positions and no fabricated geometry");
    }

    struct Probe {
        bool exercised = false;
        const NameObj* holder_identity = nullptr;
        std::vector<const NameObj*> drawer_identities;

        void exercise() {
            const auto domain = smgpc::scene::current_scene_allocation_domain();
            require(domain != nullptr, "Actual original scene allocation domain exists");
            auto* holder = MR::getSceneObj<ShadowControllerHolder>(SceneObj_ShadowControllerHolder);
            require(holder != nullptr, "Actual original shadow holder exists");
            holder_identity = holder;
            const auto initial_count = holder->_C.size();
            const auto initial_pending = holder->_18.size();
            const auto initial_actors = smgpc::compat::actor_runtime_state_count();
            smgpc::scene::NameObjChildOwner endpoints, lines, csv;
            LiveActor *from = nullptr, *to = nullptr, *owner = nullptr, *single = nullptr, *csv_actor = nullptr;
            endpoints.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                from = new LiveActor("ShadowLineFromProbe");
                to = new LiveActor("ShadowLineToProbe");
                MR::initShadowController(from, 1);
                MR::initShadowController(to, 1);
                MR::addShadowVolumeSphere(from, "from", 5);
                MR::addShadowVolumeSphere(to, "to", 10);
            });
            auto* from_controller = from->mShadowControllerList->getController(u32(0));
            auto* to_controller = to->mShadowControllerList->getController(u32(0));
            for (auto* controller : {from_controller, to_controller}) {
                auto* sphere = dynamic_cast<ShadowVolumeSphere*>(controller->getShadowDrawer());
                require(sphere && sphere->mModelData, "Programmatic sphere owns the original drawer and actual loaded model");
                require(sphere->mStartDrawShapeOffset == 0 && sphere->mEndDrawShapeOffset == 0,
                        "Programmatic volume offsets preserve original zero constructor defaults");
                drawer_identities.push_back(sphere);
            }
            lines.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                owner = new LiveActor("ShadowLineOwnerProbe");
                MR::initShadowController(owner, 4);
                // Other actors' original one-controller lookup ignores names.
                MR::addShadowVolumeLine(owner, "line", from, "ignored", 5, to, "also ignored", 10);
                MR::addShadowVolumeLine(owner, "self", owner, "line", 3, owner, "self", 4);
                MR::addShadowVolumeLine(owner, "missing", owner, "unknown", 1, owner, "line", 2);
                single = new LiveActor("ShadowLineSingleProbe");
                MR::initShadowController(single, 1);
                MR::addShadowVolumeLine(single, "solo", single, "ignored", 1, single, "also ignored", 2);
            });
            auto* controller = owner->mShadowControllerList->getController("line");
            auto* line = dynamic_cast<ShadowVolumeLine*>(controller->getShadowDrawer());
            require(line && line->mFromShadowController == from_controller && line->mToShadowController == to_controller &&
                        line->mFromWidth == 5 && line->mToWidth == 10 && !controller->isCalcCollision(),
                    "Programmatic line owns its original drawer, borrowed cross-actor endpoints, widths and disabled collision calculation");
            auto* self_controller = owner->mShadowControllerList->getController("self");
            auto* self = dynamic_cast<ShadowVolumeLine*>(self_controller->getShadowDrawer());
            auto* missing = dynamic_cast<ShadowVolumeLine*>(owner->mShadowControllerList->getController("missing")->getShadowDrawer());
            require(self && self->mFromShadowController == controller && self->mToShadowController == self_controller &&
                        missing && !missing->mFromShadowController && missing->mToShadowController == controller,
                    "Same-actor resolution observes the post-add list, including preceding, self and absent endpoint names");
            for (u32 index = 0; index < owner->mShadowControllerList->getControllerCount(); ++index) {
                auto* value = owner->mShadowControllerList->getController(index);
                require(std::find(holder->_C.begin(), holder->_C.end(), value) != holder->_C.end(),
                        "Actual controller holder registers every programmatic line");
                drawer_identities.push_back(value->getShadowDrawer());
                value->appendToHolder();
            }
            auto* single_controller = single->mShadowControllerList->getController(u32(0));
            auto* single_line = dynamic_cast<ShadowVolumeLine*>(single_controller->getShadowDrawer());
            require(single_line && single_line->mFromShadowController == single_controller &&
                        single_line->mToShadowController == single_controller && shape_bytes(*single_line).empty(),
                    "A newly created sole controller resolves both ignored names to itself and emits no coincident geometry");
            drawer_identities.push_back(single_line);
            const auto count_before_rejection = holder->_C.size();
            auto invalid = smgpc::compat::make_actor_shadow_controller_runtime_state(
                owner, "invalid", smgpc::compat::ActorShadowControllerKind::VolumeLine, 0);
            invalid.line_start_controller_index = 99;
            bool rejected = false;
            try { (void)smgpc::compat::add_actor_shadow_controller(owner, std::move(invalid)); }
            catch (const std::out_of_range&) { rejected = true; }
            require(rejected && holder->_C.size() == count_before_rejection && owner->mShadowControllerList->getControllerCount() == 3,
                    "Invalid complete endpoint definition fails before original holder/list publication");
            from->mPosition.set(0,0,0);
            to->mPosition.set(100,0,0);
            TVec3f direction(0,-1,0);
            MR::setShadowDropDirectionPtr(from, nullptr, &direction);
            MR::setShadowDropDirectionPtr(to, nullptr, &direction);
            MR::setShadowDropLength(from, nullptr, 20);
            MR::setShadowDropLength(to, nullptr, 30);
            require(from_controller->mDropDir == &direction && to_controller->mDropDir == &direction,
                    "Drop-direction pointer remains borrowed by actual original controllers");
            verify_shape(*line, false);
            direction.set(0,0,-1);
            verify_shape(*line, true);
            direction.set(1,0,0);
            require(shape_bytes(*line).empty(), "Parallel drop direction retains the original no-shape early return");
            direction.set(0,-1,0);
            to->mPosition = from->mPosition;
            require(shape_bytes(*line).empty(), "Coincident borrowed endpoints retain the original no-shape early return");

            const auto archive = csv_archive();
            csv.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                csv_actor = new LiveActor("ShadowLineCsvProbe");
                smgpc::compat::initialize_actor_shadow_from_archive(csv_actor, archive, "Shadow");
            });
            auto* csv_list = csv_actor->mShadowControllerList;
            auto* csv_controller = csv_list->getController("line");
            auto* csv_line = dynamic_cast<ShadowVolumeLine*>(csv_controller->getShadowDrawer());
            auto* unresolved = dynamic_cast<ShadowVolumeLine*>(csv_list->getController("unresolved")->getShadowDrawer());
            require(csv_line && csv_line->mFromShadowController == csv_list->getController("base") &&
                        csv_line->mToShadowController == csv_controller && csv_line->mStartDrawShapeOffset == 100 &&
                        csv_line->mEndDrawShapeOffset == 100 && unresolved && !unresolved->mFromShadowController,
                    "Actual CSV drawers preserve preceding/self binding, unresolved forward names and explicit 100-unit volume defaults");
            for (u32 index = 0; index < csv_list->getControllerCount(); ++index)
                drawer_identities.push_back(csv_list->getController(index)->getShadowDrawer());
            require(holder->_C.size() == initial_count + 10, "Original holder contains exactly ten new owned controllers");
            csv.clear();
            // Endpoints are borrowed exactly like original drop-position and
            // direction bindings. Retire their dependent line actors first.
            lines.clear();
            require(holder->_C.size() == initial_count + 2 && holder->_18.size() == initial_pending,
                    "Line retirement removes both complete and pending original holder registrations");
            endpoints.clear();
            require(holder->_C.size() == initial_count && smgpc::compat::actor_runtime_state_count() == initial_actors,
                    "Actor retirement restores original holder membership and actor ownership counts");
            for (const auto* drawer : drawer_identities)
                require(!smgpc::compat::has_name_obj_runtime_state(drawer), "Actor ownership retires every original drawer registration");
        }

        void after_frame(GameSystem& system, std::uint64_t frame) {
            if (exercised) return;
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            const aurora::allocation::HostAllocationScope host;
            exercise();
            exercised = true;
            std::fprintf(stderr, "[shadow-line-probe] PASS actual holder/list/drawers, programmatic/CSV bindings, borrowed direction, original GX shapes, actor retirement; frame=%llu\n", static_cast<unsigned long long>(frame));
        }
    };
}
#endif

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "This original-process diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-shadow-line-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original shadow line integration",
            .arguments = {"original-shadow-line-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised,
                "OriginalProcess completes the actual scene diagnostic and normal bounded frame loop");
        require(!smgpc::compat::has_name_obj_runtime_state(probe.holder_identity) &&
                    smgpc::compat::actor_shadow_runtime_state_count() == 0,
                "Normal original-process scene retirement removes the real holder and all shadow owners");
        std::fprintf(stderr, "PASS original-process ShadowVolumeLine: actual controllers/drawers, CSV/programmatic endpoints and defaults, borrowed direction, original command geometry, actor/scene retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process ShadowVolumeLine: %s\n", error.what());
        return 1;
    }
#endif
}
