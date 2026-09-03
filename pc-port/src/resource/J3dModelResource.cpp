#include "J3dModelResource.hpp"
#include "J3dJointData.hpp"
#include "J3dGeometryData.hpp"
#include "J3dMaterialTableData.hpp"
#include "J3dTextureData.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/J3DModelLoaderCompat.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"
#include "JSystem/J3DGraphBase/J3DShapeMtx.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DTevs.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace smgpc::resource {
    namespace {
        using Bytes = std::span<const std::uint8_t>;
        constexpr std::uint32_t tag(char a, char b, char c, char d) {
            return (std::uint32_t(static_cast<unsigned char>(a)) << 24) |
                   (std::uint32_t(static_cast<unsigned char>(b)) << 16) |
                   (std::uint32_t(static_cast<unsigned char>(c)) << 8) |
                   std::uint32_t(static_cast<unsigned char>(d));
        }
        void require_range(std::size_t size, std::size_t offset, std::size_t extent) {
            if (offset > size || extent > size - offset)
                throw std::runtime_error("J3D model range exceeds its retained resource");
        }
        std::uint16_t read_u16(Bytes bytes, std::size_t offset) {
            require_range(bytes.size(), offset, 2);
            return (std::uint16_t(bytes[offset]) << 8) | bytes[offset + 1];
        }
        std::uint32_t read_u32(Bytes bytes, std::size_t offset) {
            return (std::uint32_t(read_u16(bytes, offset)) << 16) | read_u16(bytes, offset + 2);
        }
        struct File {
            Bytes source;
            std::uint32_t magic;
            std::uint32_t type;
            std::vector<Bytes> blocks;
            explicit File(Bytes bytes) : source(bytes), magic(read_u32(bytes, 0)), type(read_u32(bytes, 4)) {
                require_range(bytes.size(), 0, 0x20);
                const auto size = read_u32(bytes, 8);
                if (size < 0x20) throw std::runtime_error("J3D model file size is smaller than its header");
                require_range(bytes.size(), 0, size);
                source = bytes.first(size);
                std::size_t offset = 0x20;
                for (std::uint32_t i = 0; i < read_u32(source, 0xc); ++i) {
                    require_range(source.size(), offset, 8);
                    const auto size = read_u32(source, offset + 4);
                    if (size < 8) throw std::runtime_error("J3D model block is smaller than its header");
                    require_range(source.size(), offset, size);
                    blocks.push_back(source.subspan(offset, size));
                    offset += size;
                }
            }
            Bytes single_block(std::uint32_t type) const {
                Bytes result;
                for (const auto block : blocks) {
                    if (read_u32(block, 0) != type) continue;
                    if (!result.empty()) throw std::runtime_error("J3D model contains duplicate construction blocks");
                    result = block;
                }
                return result;
            }
        };

        // Native construction may run under an existing caller's GD/OS scope.
        // Original SDK routines retain one-time interrupt snapshots. Pin their
        // cooperative CPU ownership for the complete operation and restore this
        // caller's state after those unchanged routines finish.
        class CommandScope final {
            BOOL interrupts;
            GDLObj* previous;
        public:
            CommandScope() : interrupts(OSDisableInterrupts()) {
                OSDisableScheduler();
                OSRestoreInterrupts(interrupts);
                previous = __GDCurrentDL;
            }
            ~CommandScope() {
                GDSetCurrent(previous);
                OSRestoreInterrupts(interrupts);
                OSEnableScheduler();
            }
            CommandScope(const CommandScope&) = delete;
        };

        // Model the original pointer links before entering its recursive walk.
        // This validates storage bounds/cycles while retaining legal overwrites
        // and shared links; it does not replace the original hierarchy builder.
        void validate_hierarchy(J3DModelData& model, Bytes information) {
            constexpr int absent = -1;
            const auto joint_count = model.getJointNum();
            const auto material_count = model.getMaterialNum();
            std::vector<int> child(joint_count, absent), younger(joint_count, absent), mesh(joint_count, absent);
            std::vector<int> next_material(material_count, absent), material_shape(material_count, absent);
            struct Frame { int parent; int current; };
            std::vector<Frame> frames{{absent, absent}};
            std::size_t offset = read_u32(information, 0x14);
            for (;;) {
                const auto type = read_u16(information, offset);
                const auto value = read_u16(information, offset + 2);
                offset += 4;
                auto& frame = frames.back();
                if (type == 0) {
                    if (frames.size() != 1) throw std::runtime_error("J3D hierarchy has an unclosed child scope");
                    break;
                }
                if (type == 1) {
                    const auto parent = frame.current;
                    frames.push_back({parent, parent});
                    continue;
                }
                if (type == 2) {
                    if (frames.size() == 1) throw std::runtime_error("J3D hierarchy closes an absent child scope");
                    frames.pop_back();
                    continue;
                }
                if (type == 0x10) {
                    if (value >= joint_count) throw std::runtime_error("J3D hierarchy joint is outside JNT1");
                    frame.current = value;
                    if (frame.parent != absent) {
                        auto& first = child[frame.parent];
                        if (first == absent) first = value;
                        else {
                            auto cursor = first;
                            std::size_t traversed = 0;
                            while (younger[cursor] != absent) {
                                if (++traversed > joint_count) throw std::runtime_error("J3D hierarchy contains a sibling cycle");
                                cursor = younger[cursor];
                            }
                            younger[cursor] = value;
                        }
                    }
                } else if (type == 0x11) {
                    if (value >= material_count) throw std::runtime_error("J3D hierarchy material is outside MAT3/MDL3");
                    if (frame.parent == absent) throw std::runtime_error("J3D hierarchy material has no parent joint");
                    if (mesh[frame.parent] != absent) next_material[value] = mesh[frame.parent];
                    mesh[frame.parent] = value;
                } else if (type == 0x12) {
                    if (value >= model.getShapeNum()) throw std::runtime_error("J3D hierarchy shape is outside SHP1");
                    if (frame.parent == absent || mesh[frame.parent] == absent)
                        throw std::runtime_error("J3D hierarchy shape has no parent material");
                    material_shape[mesh[frame.parent]] = value;
                } else {
                    throw std::runtime_error("J3D hierarchy contains an unknown command");
                }
            }
            // The two SDK links form a directed graph. Sharing is legal; an
            // active-path cycle would make original traversal nonterminating.
            std::vector<unsigned char> visited(joint_count);
            std::vector<std::pair<int, bool>> pending;
            for (int root = 0; root < joint_count; ++root) {
                pending.push_back({root, false});
                while (!pending.empty()) {
                    const auto [node, leaving] = pending.back();
                    pending.pop_back();
                    if (leaving) { visited[node] = 2; continue; }
                    if (visited[node] == 2) continue;
                    if (visited[node] == 1) throw std::runtime_error("J3D hierarchy contains a joint cycle");
                    visited[node] = 1;
                    pending.push_back({node, true});
                    if (younger[node] != absent) pending.push_back({younger[node], false});
                    if (child[node] != absent) pending.push_back({child[node], false});
                }
            }
            for (const auto first : mesh) {
                std::size_t traversed = 0;
                for (auto material = first; material != absent; material = next_material[material]) {
                    if (++traversed > material_count) throw std::runtime_error("J3D hierarchy contains a material cycle");
                    if (material_shape[material] == absent)
                        throw std::runtime_error("J3D joint mesh has no shape for original finalization");
                }
            }
        }

        void validate_shape_matrices(J3DModelData& model) {
            for (u16 i = 0; i < model.getShapeNum(); ++i) {
                auto* shape = model.getShapeNodePointer(i);
                for (u16 group = 0; group < shape->getMtxGroupNum(); ++group) {
                    auto* matrix = shape->getShapeMtx(group);
                    for (u16 slot = 0; slot < matrix->getUseMtxNum(); ++slot) {
                        const auto index = matrix->getUseMtxIndex(slot);
                        if (index != 0xffff && index >= model.getDrawMtxNum())
                            throw std::runtime_error("J3D shape matrix is outside the original draw-matrix allocation");
                    }
                }
            }
        }

        void validate_display_lists(J3DMaterialTable& table, const J3dMaterialTableData& materials) {
            compat::JkrHostAllocationScope host;
            struct List { std::uintptr_t start, end; u16 material; };
            struct AlignedDelete {
                void operator()(std::uint8_t* bytes) const noexcept { ::operator delete[](bytes, std::align_val_t{32}); }
            };
            struct Region { std::uintptr_t start, end; std::unique_ptr<std::uint8_t[], AlignedDelete> bytes; };
            std::vector<List> lists;
            for (u16 i = 0; i < table.getMaterialNum(); ++i) {
                auto* material = table.getMaterialNodePointer(i);
                auto* display = material->getSharedDisplayListObj();
                if (!display || !display->getDisplayList(0) || !material->getTevBlock())
                    throw std::runtime_error("J3D binary model lacks its original shared display list/TEV block");
                const auto start = reinterpret_cast<std::uintptr_t>(display->getDisplayList(0));
                const auto end = start + display->getDisplayListSize();
                if (end < start || end - start < 2 || (start & 31) != 0)
                    throw std::runtime_error("J3D display-list address range/alignment is invalid for GD");
                lists.push_back({start, end, i});
            }
            // Retain overlapping command views in the same preview allocation.
            // Later materials must read the bytes produced by earlier patches,
            // exactly as indexToPtr does on the real aliased MDL3 allocation.
            auto ordered = lists;
            std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) { return a.start < b.start; });
            std::vector<Region> regions;
            for (const auto& list : ordered) {
                if (regions.empty() || list.start >= regions.back().end) regions.push_back({list.start, list.end, {}});
                else regions.back().end = std::max(regions.back().end, list.end);
            }
            for (auto& region : regions) region.bytes.reset(static_cast<std::uint8_t*>(
                ::operator new[](region.end - region.start, std::align_val_t{32})));
            for (const auto& list : lists) {
                auto& region = *std::find_if(regions.begin(), regions.end(), [&](const auto& r) {
                    return list.start >= r.start && list.end <= r.end;
                });
                std::memcpy(region.bytes.get() + list.start - region.start, reinterpret_cast<const void*>(list.start), list.end - list.start);
            }
            struct PreviewState {
                J3DTexture* texture = j3dSys.getTexture();
                GDLObj* display = __GDCurrentDL;
                std::array<J3DTexCoordScaleInfo, 8> scales;
                PreviewState() { std::copy_n(J3DSys::sTexCoordScaleTable, 8, scales.begin()); }
                ~PreviewState() {
                    std::copy(scales.begin(), scales.end(), J3DSys::sTexCoordScaleTable);
                    j3dSys.setTexture(texture);
                    GDSetCurrent(display);
                }
            } preview;
            j3dSys.setTexture(table.getTexture());
            for (const auto& list : lists) {
                const auto i = list.material;
                auto& region = *std::find_if(regions.begin(), regions.end(), [&](const auto& r) {
                    return list.start >= r.start && list.end <= r.end;
                });
                auto* start = region.bytes.get() + list.start - region.start;
                const Bytes commands(start, list.end - list.start);
                GDLObj output;
                GDInitGDLObj(&output, start, static_cast<u32>(commands.size()));
                GDSetCurrent(&output);
                std::size_t offset = materials.tex_no_patch_offset(i);
                for (std::size_t slot = 0;; ++slot) {
                    require_range(commands.size(), offset, 2);
                    const auto reg = commands[offset + 1];
                    if (reg < 0x80 || reg > 0xbb) break;
                    require_range(commands.size(), offset, 5);
                    // Original getTexNoReg narrows the low 24 bits to u16.
                    const auto texture = static_cast<u16>(read_u32(commands, offset + 1));
                    if (slot >= 8 || !table.getTexture() || texture >= table.getTexture()->getNum())
                        throw std::runtime_error("J3D display-list texture reference is outside TEX1/GX slots");
                    const auto* image = table.getTexture()->getResTIMG(texture);
                    const std::size_t written = image->mPaletteName == 1 ? 0x37 : 0x14;
                    require_range(commands.size(), offset, written);
                    GDSetCurrOffset(static_cast<u32>(offset));
                    // Use the actual SDK primitive for preview bytes as well:
                    // texture/palette addresses, register encodings and write
                    // ordering must match the ensuing original patch loop.
                    loadTexNo(static_cast<u32>(slot), texture);
                    offset += written;
                }
            }
        }

        struct LoadedData {
            std::shared_ptr<compat::JkrAllocationDomain> domain;
            std::unique_ptr<J3dJointData> joints;
            std::unique_ptr<J3dGeometryData> geometry;
            std::unique_ptr<J3dMaterialTableData> materials;
            std::unique_ptr<J3dTextureData> textures;
            std::unique_ptr<J3DTexture> empty_texture;
            std::unique_ptr<J3DMaterialTable> table;
            std::unique_ptr<J3DModelData> model;

            explicit LoadedData(std::shared_ptr<compat::JkrAllocationDomain> value) : domain(std::move(value)) {}
            ~LoadedData() {
                compat::JkrHostAllocationScope host;
                compat::JkrAllocationScope original(domain);
                CommandScope commands;
                const auto* texture = textures ? &textures->texture() : empty_texture.get();
                if (texture && j3dSys.getTexture() == texture) j3dSys.setTexture(nullptr);
                model.reset();
                table.reset();
                materials.reset();
                geometry.reset();
                joints.reset();
                textures.reset();
                empty_texture.reset();
            }
        };

        struct Registry {
            struct Entry {
                std::weak_ptr<void> owner;
                std::uint64_t generation;
                std::size_t references;
            };
            std::mutex mutex;
            std::map<const void*, Entry> resources;
            std::uint64_t next_generation = 1;
        };
        Registry& registry() { static Registry value; return value; }
    }

    struct J3dModelResource::Storage {
        std::shared_ptr<compat::JkrAllocationDomain> domain;
        std::shared_ptr<Mem1ResourceHeap> mem1;
        std::vector<std::uint8_t> source;
        std::mutex mutex;
        std::uint64_t generation = 0;
        std::vector<std::unique_ptr<LoadedData>> loads;

        Storage(Bytes bytes, std::shared_ptr<compat::JkrAllocationDomain> allocation,
                std::shared_ptr<Mem1ResourceHeap> texture_heap)
            : domain(std::move(allocation)), mem1(std::move(texture_heap)), source(bytes.begin(), bytes.end()) {
            if (!domain) throw std::invalid_argument("J3D model owner requires an original allocation domain");
            if (source.empty()) throw std::invalid_argument("J3D model source is empty");
        }
        ~Storage() {
            compat::JkrHostAllocationScope host;
            {
                auto& r = registry();
                std::lock_guard lock(r.mutex);
                const auto found = r.resources.find(source.data());
                if (found != r.resources.end() && found->second.generation == generation) r.resources.erase(found);
            }
            loads.clear();
        }

        void attach_textures(const File& file, LoadedData& result, J3DMaterialTable& table, bool material_table) {
            const auto texture = file.single_block(tag('T','E','X','1'));
            if (!texture.empty()) {
                result.textures = std::make_unique<J3dTextureData>(texture, mem1);
                result.textures->attach_to(table);
            } else if (material_table) {
                compat::JkrAllocationScope original(domain);
                result.empty_texture = std::make_unique<J3DTexture>(0, nullptr);
                table.mTexture = result.empty_texture.get();
            }
        }

        J3DModelData* load_model(std::uint32_t flags, bool binary) {
            compat::JkrHostAllocationScope host;
            if (read_u32(source, 0) != tag('J','3','D','2')) return nullptr;
            const auto type = read_u32(source, 4);
            if (!binary && type == tag('b','m','d','2'))
                throw std::runtime_error("Original v21 J3D model loading is not yet provided on this host");
            if (binary) {
                if (type != tag('b','d','l','3') && type != tag('b','d','l','4')) return nullptr;
            } else if (type != tag('b','m','d','3')) return nullptr;
            const File file(source);
            auto result = std::make_unique<LoadedData>(domain);
            // Typed endian/native-pointer construction components are host
            // owned. SDK factory allocations enter the retained domain inside
            // their owner; later SDK finalization uses that same domain.
            result->joints = std::make_unique<J3dJointData>(source, flags);
            result->geometry = std::make_unique<J3dGeometryData>(source, flags);
            result->materials = std::make_unique<J3dMaterialTableData>(source, flags,
                binary ? J3dMaterialTableData::Mode::BinaryModel : J3dMaterialTableData::Mode::Model, domain);
            {
                compat::JkrAllocationScope original(domain);
                result->model = std::make_unique<J3DModelData>();
                result->model->clear();
            }
            auto& model = *result->model;
            model.mpRawData = source.data();
            result->joints->attach_to(model);
            result->geometry->attach_to(model);
            result->materials->attach_to(model.mMaterialTable);
            attach_textures(file, *result, model.mMaterialTable, false);
            validate_hierarchy(model, file.single_block(tag('I','N','F','1')));
            validate_shape_matrices(model);
            {
                compat::JkrAllocationScope original(domain);
                CommandScope commands;
                if (binary) validate_display_lists(model.mMaterialTable, *result->materials);
                compat::finalize_j3d_model(model, result->geometry->shape_block(), binary);
            }
            auto* pointer = result->model.get();
            std::lock_guard lock(mutex);
            loads.push_back(std::move(result));
            return pointer;
        }

        J3DMaterialTable* load_table() {
            compat::JkrHostAllocationScope host;
            if (read_u32(source, 0) != tag('J','3','D','2') || read_u32(source, 4) != tag('b','m','t','3')) return nullptr;
            const File file(source);
            auto result = std::make_unique<LoadedData>(domain);
            result->materials = std::make_unique<J3dMaterialTableData>(source, 0x51100000,
                J3dMaterialTableData::Mode::MaterialTable, domain);
            {
                compat::JkrAllocationScope original(domain);
                result->table = std::make_unique<J3DMaterialTable>();
                result->table->clear();
            }
            result->materials->attach_to(*result->table);
            attach_textures(file, *result, *result->table, true);
            auto* pointer = result->table.get();
            std::lock_guard lock(mutex);
            loads.push_back(std::move(result));
            return pointer;
        }
    };

    J3dModelResource::J3dModelResource(Bytes bytes, std::shared_ptr<compat::JkrAllocationDomain> domain,
                                     std::shared_ptr<Mem1ResourceHeap> mem1) {
        compat::JkrHostAllocationScope host;
        _storage = std::make_shared<Storage>(bytes, std::move(domain), std::move(mem1));
        auto& r = registry();
        std::lock_guard lock(r.mutex);
        _storage->generation = r.next_generation++;
        if (_storage->generation == 0) throw std::overflow_error("J3D model registration identity exhausted");
        r.resources.emplace(_storage->source.data(), Registry::Entry{_storage, _storage->generation, 1});
    }
    struct J3dModelSourceRegistration::State {
        std::shared_ptr<void> owner;
        const void* identity;
        std::uint64_t generation;
        State(std::shared_ptr<void> value, const void* key, std::uint64_t id) : owner(std::move(value)), identity(key), generation(id) {}
        ~State() {
            compat::JkrHostAllocationScope host;
            auto& r = registry();
            std::lock_guard lock(r.mutex);
            const auto found = r.resources.find(identity);
            if (found != r.resources.end() && found->second.generation == generation && --found->second.references == 0)
                r.resources.erase(found);
        }
    };
    J3dModelSourceRegistration::J3dModelSourceRegistration(std::unique_ptr<State> state) : _state(std::move(state)) {}
    J3dModelSourceRegistration::~J3dModelSourceRegistration() = default;
    J3dModelSourceRegistration::J3dModelSourceRegistration(J3dModelSourceRegistration&&) noexcept = default;
    J3dModelSourceRegistration& J3dModelSourceRegistration::operator=(J3dModelSourceRegistration&&) noexcept = default;
    J3dModelSourceRegistration J3dModelResource::register_source(Bytes bytes) {
        compat::JkrHostAllocationScope host;
        if (!_storage || bytes.size() != _storage->source.size() || !std::equal(bytes.begin(), bytes.end(), _storage->source.begin()))
            throw std::invalid_argument("J3D model alias does not match its complete retained source");
        auto& r = registry();
        std::lock_guard lock(r.mutex);
        auto found = r.resources.find(bytes.data());
        std::uint64_t generation;
        if (found != r.resources.end()) {
            if (found->second.owner.lock().get() != _storage.get())
                throw std::logic_error("J3D model identity belongs to a different retained owner");
            generation = found->second.generation;
            ++found->second.references;
        } else {
            generation = r.next_generation++;
            if (generation == 0) throw std::overflow_error("J3D model registration identity exhausted");
            r.resources.emplace(bytes.data(), Registry::Entry{_storage, generation, 1});
        }
        try {
            return J3dModelSourceRegistration(std::make_unique<J3dModelSourceRegistration::State>(_storage, bytes.data(), generation));
        } catch (...) {
            found = r.resources.find(bytes.data());
            if (--found->second.references == 0) r.resources.erase(found);
            throw;
        }
    }
    J3dModelResource::~J3dModelResource() = default;
    const void* J3dModelResource::data() const noexcept { return _storage ? _storage->source.data() : nullptr; }
    Bytes J3dModelResource::bytes() const noexcept { return _storage ? Bytes(_storage->source) : Bytes{}; }
    J3DModelData* J3dModelResource::load(std::uint32_t flags) {
        if (!_storage) return nullptr;
        const auto type = read_u32(bytes(), 4);
        return load_registered_j3d_model(data(), flags, type == tag('b','d','l','3') || type == tag('b','d','l','4'));
    }
    J3DMaterialTable* J3dModelResource::load_material_table() { return load_registered_j3d_material_table(data()); }
    J3DModelData* load_registered_j3d_model(const void* data, std::uint32_t flags, bool binary) {
        if (!data) return nullptr;
        compat::JkrHostAllocationScope host;
        std::shared_ptr<J3dModelResource::Storage> owner;
        {
            auto& r = registry();
            std::lock_guard lock(r.mutex);
            const auto found = r.resources.find(data);
            if (found != r.resources.end()) owner = std::static_pointer_cast<J3dModelResource::Storage>(found->second.owner.lock());
        }
        if (!owner) throw std::runtime_error("J3D model loading requires a registered bounded resource owner");
        return owner->load_model(flags, binary);
    }
    J3DMaterialTable* load_registered_j3d_material_table(const void* data) {
        if (!data) return nullptr;
        compat::JkrHostAllocationScope host;
        std::shared_ptr<J3dModelResource::Storage> owner;
        {
            auto& r = registry();
            std::lock_guard lock(r.mutex);
            const auto found = r.resources.find(data);
            if (found != r.resources.end()) owner = std::static_pointer_cast<J3dModelResource::Storage>(found->second.owner.lock());
        }
        if (!owner) throw std::runtime_error("J3D material-table loading requires a registered bounded resource owner");
        return owner->load_table();
    }
}
