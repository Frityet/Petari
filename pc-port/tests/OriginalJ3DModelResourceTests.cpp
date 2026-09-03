#include "resource/J3dModelResource.hpp"
#include "resource/J3dAllocationIdentity.hpp"
#include "resource/Mem1ResourceHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/RuntimeServices.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <dolphin/os.h>

#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }

namespace {
    using namespace smgpc::resource;
    using namespace smgpc::compat;
    using Bytes = std::vector<std::uint8_t>;
    using View = std::span<const std::uint8_t>;
    void require(bool condition, const char* message) {
        if (!condition) {
            JkrHostAllocationScope host;
            throw std::runtime_error(message);
        }
    }
    template<class F> void rejects(F&& operation) {
        bool caught = false;
        try { operation(); } catch (const std::exception&) { caught = true; }
        require(caught, "invalid native model boundary must reject before unsafe original access");
    }
    void write32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
        for (int i = 0; i < 4; ++i) bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (24 - i * 8));
    }
    std::uint16_t read16(View bytes, std::size_t offset) {
        return (std::uint16_t(bytes[offset]) << 8) | bytes[offset + 1];
    }
    std::uint32_t read32(View bytes, std::size_t offset) {
        return (std::uint32_t(read16(bytes, offset)) << 16) | read16(bytes, offset + 2);
    }
    void set_type(Bytes& bytes, std::string_view type) { std::memcpy(bytes.data() + 4, type.data(), 4); }
    Bytes empty_file(std::string_view type) {
        Bytes bytes(0x20);
        std::memcpy(bytes.data(), "J3D2", 4);
        set_type(bytes, type);
        write32(bytes, 8, bytes.size());
        return bytes;
    }
    View block(View bytes, std::string_view type) {
        std::size_t offset = 0x20;
        for (std::size_t i = 0; i < read32(bytes, 0xc); ++i) {
            const auto size = read32(bytes, offset + 4);
            if (std::memcmp(bytes.data() + offset, type.data(), 4) == 0) return bytes.subspan(offset, size);
            offset += size;
        }
        throw std::runtime_error("fixture block missing");
    }

    void test_sdk_boundary(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        require(J3DModelLoaderDataBase::load(nullptr, 0) == nullptr &&
                J3DModelLoaderDataBase::loadBinaryDisplayList(nullptr, 0) == nullptr &&
                J3DModelLoaderDataBase::loadMaterialTable(nullptr) == nullptr, "original null dispatch");
        const auto* invalid = reinterpret_cast<const void*>(std::uintptr_t{1});
        rejects([&] { (void)J3DModelLoaderDataBase::load(invalid, 0); });
        rejects([&] { (void)J3DModelLoaderDataBase::loadBinaryDisplayList(invalid, 0); });
        rejects([&] { (void)J3DModelLoaderDataBase::loadMaterialTable(invalid); });
        auto domain = JkrAllocationDomain::create(runtime, 128 * 1024);
        for (const auto type : {"bmd1", "bmt2", "test"}) {
            auto bytes = empty_file(type);
            bytes.resize(8); // Unsupported dispatch reads only the two tags.
            J3dModelResource resource(bytes, domain, {});
            require(!resource.load(0) && !resource.load_material_table(), "unsupported original formats return null");
        }
        auto bytes = empty_file("bmt3");
        J3dModelResource table(bytes, domain, {});
        auto* actual = table.load_material_table();
        require(actual && actual->getMaterialNum() == 0 && actual->getTexture() && actual->getTexture()->getNum() == 0,
                "original empty material table owns its actual empty texture object");
        require(domain->heap().find(actual) == &domain->heap(), "actual table allocation belongs to retained original heap");
    }

    void test_alias_lifetime(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        auto bytes = empty_file("bmt3");
        const void* alias_pointer = bytes.data();
        std::weak_ptr<JkrAllocationDomain> weak;
        std::optional<J3dModelSourceRegistration> alias;
        {
            auto domain = JkrAllocationDomain::create(runtime, 128 * 1024);
            weak = domain;
            J3dModelResource owner(bytes, domain, {});
            auto corrupted = bytes; corrupted[0] = 0;
            rejects([&] { (void)owner.register_source(corrupted); });
            rejects([&] { (void)owner.register_source(View(bytes).first(8)); });
            alias.emplace(owner.register_source(bytes));
            {
                auto duplicate = owner.register_source(bytes);
                require(J3DModelLoaderDataBase::loadMaterialTable(alias_pointer), "duplicate alias preserves original dispatch");
            }
        }
        require(!weak.expired(), "source alias retains the resource and its allocation domain");
        auto* table = J3DModelLoaderDataBase::loadMaterialTable(alias_pointer);
        require(table && table->getTexture()->getNum() == 0, "alias remains usable after external resource owner retires");
        alias.reset();
        require(weak.expired(), "last alias destroys actual SDK objects before retiring their heap");
        rejects([&] { (void)J3DModelLoaderDataBase::loadMaterialTable(alias_pointer); });
    }

    void test_identity(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        auto domain = JkrAllocationDomain::create(runtime, 128 * 1024);
        std::optional<J3dAllocationIdentity> first;
        std::optional<J3dAllocationIdentity> second;
        std::uint32_t freed = 0;
        {
            JkrAllocationScope original(domain);
            first.emplace(0x120);
            second.emplace(0x80);
            freed = first->address(0);
            require(first->address(0x11f) < second->address(0), "live identities do not alias");
            require((first->address(0) >> 4 & 0xc0000000U) == 0 && first->address(0) >= 0x80000000U,
                    "shifted and unshifted original address-state bits are preserved");
        }
        domain.reset();
        require(second->address(0x7f) != 0, "identity registry lives outside temporary Game allocation domains");
        first.reset();
        J3dAllocationIdentity reused(0x120);
        require(reused.address(0) == freed, "released original-width reservation is reclaimed");
        rejects([&] { (void)reused.address(0x120); });
    }

    void check_original_model(J3DModelData& model, View bytes) {
        require(model.getJointNum() == read16(block(bytes, "JNT1"), 8), "actual joint count");
        require(model.getMaterialNum() == read16(block(bytes, "MAT3"), 8), "actual material count");
        require(model.getShapeNum() == read16(block(bytes, "SHP1"), 8), "actual shape count");
        require(model.getTexture()->getNum() == read16(block(bytes, "TEX1"), 8), "actual texture count");
        require(model.getJointTree().getRootNode() != nullptr, "original hierarchy built its root joint");
        for (u16 i = 0; i < model.getMaterialNum(); ++i) {
            auto* material = model.getMaterialNodePointer(i);
            require(material->getJoint() && material->getShape(), "original hierarchy links real joints/materials/shapes");
            require(material->getShape()->getMaterial() == material, "original reverse shape material link");
            require(material->getSharedDisplayListObj() && material->getSharedDisplayListObj()->getDisplayListSize() > 0,
                    "original shared material display list is present");
        }
        for (u16 i = 0; i < model.getShapeNum(); ++i) {
            auto* shape = model.getShapeNodePointer(i);
            require(shape->mDrawMtxData == model.getDrawMtxData() && shape->mVertexData == &model.getVertexData(),
                    "original initShapeNodes attaches actual model data");
        }
        J3DMtxBuffer matrices;
        require(matrices.create(&model, 0) == 0, "actual matrix buffer creates for complete model");
        Mtx identity; PSMTXIdentity(identity);
        model.getJointTree().calc(&matrices, Vec{1, 1, 1}, identity);
        for (u16 joint = 0; joint < model.getJointNum(); ++joint)
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 4; ++col)
                    require(std::isfinite(matrices.getAnmMtx(joint)[row][col]), "original full-model joint calculation is finite");
    }

    void test_retail(const std::shared_ptr<JkrHeapRuntime>& runtime, const std::shared_ptr<Mem1ResourceHeap>& mem1) {
        const auto* disc_path = std::getenv("SMGPC_REAL_DISC");
        if (!disc_path || !*disc_path) { std::cout << "[skip] complete retail model (set SMGPC_REAL_DISC)\n"; return; }
        require(aurora_dvd_open(disc_path), "retail disc opens");
        struct Disc { ~Disc() { aurora_dvd_close(); } } disc;
        DVDInit();
        Bytes bytes;
        {
            smgpc::runtime::DvdFileSystemService dvd{"/"};
            const auto path = dvd.find_object_archive("Mario");
            require(path.has_value(), "actual Mario archive exists");
            const auto& archive = dvd.archive_for_path(*path);
            const auto* entry = archive.find_by_basename("mario.bdl");
            require(entry, "actual Mario model exists");
            const auto source = archive.file_data(*entry);
            bytes.assign(source.begin(), source.end());
        }
        bool first_load = true;
        for (const auto flags : {0x01200000U, 0x01201000U, 0x01202000U}) {
            require(j3dSys.getTexture() == nullptr, "retiring the selected model clears its borrowed global texture");
            auto domain = JkrAllocationDomain::create(runtime, 8 * 1024 * 1024);
            J3dModelResource owner(bytes, domain, mem1);
            auto alias = owner.register_source(bytes);
            JkrAllocationScope original(domain);
            alignas(32) std::array<u8, 32> caller_bytes{};
            GDLObj caller;
            GDInitGDLObj(&caller, caller_bytes.data(), caller_bytes.size());
            GDSetCurrent(&caller);
            const auto desired_interrupts = first_load ? FALSE : TRUE;
            OSRestoreInterrupts(desired_interrupts);
            auto* model = J3DModelLoaderDataBase::loadBinaryDisplayList(bytes.data(), flags);
            require(__GDCurrentDL == &caller, "model construction restores the caller's current GD object");
            require(OSDisableInterrupts() == desired_interrupts, "model construction preserves this caller's interrupt state");
            OSRestoreInterrupts(TRUE);
            GDSetCurrent(nullptr);
            first_load = false;
            require(model && model->getModelDataType() == 1, "actual v26 binary model dispatch");
            require(domain->heap().find(model) == &domain->heap(), "actual model belongs to original heap domain");
            check_original_model(*model, bytes);
            std::cout << "[resource] complete mario.bdl flags=" << std::hex << flags << std::dec << '\n';
        }
        auto domain = JkrAllocationDomain::create(runtime, 8 * 1024 * 1024);
        {
            auto a = std::make_unique<J3dModelResource>(bytes, domain, mem1);
            auto b = std::make_unique<J3dModelResource>(bytes, domain, mem1);
            auto* model_a = a->load(0x01200000U);
            auto* model_b = b->load(0x01200000U);
            require(j3dSys.getTexture() == model_b->getTexture() && model_a->getTexture() != model_b->getTexture(),
                    "latest actual model selects its retained texture");
            a.reset();
            require(j3dSys.getTexture() == model_b->getTexture(), "retiring an older model preserves another live model selection");
            b.reset();
            require(j3dSys.getTexture() == nullptr, "retiring the selected model does not leave a dangling texture");
        }
        {
            auto aliased = bytes;
            const auto mdl = block(aliased, "MDL3");
            const auto base = static_cast<std::size_t>(mdl.data() - aliased.data());
            const auto descriptors = base + read32(mdl, 0xc);
            const auto first_start = descriptors + read32(aliased, descriptors);
            const auto first_size = read32(aliased, descriptors + 4);
            write32(aliased, descriptors + 8, static_cast<std::uint32_t>(first_start - descriptors - 8));
            write32(aliased, descriptors + 12, first_size);
            const auto patches = base + read32(mdl, 0x10);
            aliased[patches + 0x16] = aliased[patches + 6];
            aliased[patches + 0x17] = aliased[patches + 7];
            const auto patch_offset = read16(aliased, patches + 6);
            require(read16(aliased, first_start + patch_offset + 3) < read16(block(aliased, "TEX1"), 8),
                    "both aliased pristine texture commands begin with a valid texture index");
            J3dModelResource unsafe_alias(aliased, domain, mem1);
            rejects([&] { (void)unsafe_alias.load(0x01200000U); });
            require(j3dSys.getTexture() == nullptr && __GDCurrentDL == nullptr,
                    "failed ordered patch preview restores graphics state");
        }
        {
            auto bmd = bytes; set_type(bmd, "bmd3");
            J3dModelResource authored_blocks(bmd, domain, mem1);
            auto* model = authored_blocks.load(0x11300100U);
            require(model && model->getModelDataType() == 0, "original BMD3 path builds from native authored blocks");
            for (u16 i = 0; i < model->getShapeNum(); ++i)
                require(model->getShapeNodePointer(i)->checkFlag(0x200), "original BMD no-matrix flag finalization");
        }
        auto bdl3 = bytes; set_type(bdl3, "bdl3");
        J3dModelResource legacy(bdl3, domain, mem1);
        require(legacy.load(0x01200000U), "original BDL3 dispatch uses v26 native construction");
        auto malformed = bytes;
        const auto inf = block(malformed, "INF1");
        const auto hierarchy = static_cast<std::size_t>(inf.data() - malformed.data()) + read32(inf, 0x14);
        malformed[hierarchy + 2] = 0xff; malformed[hierarchy + 3] = 0xff;
        J3dModelResource invalid(malformed, domain, mem1);
        rejects([&] { (void)invalid.load(0x01200000U); });
    }
}

int main() {
    try {
        aurora::g_config.mem1Size = 24 * 1024 * 1024;
        OSInit();
        auto mem1 = Mem1ResourceHeap::create(8 * 1024 * 1024);
        auto runtime = JkrHeapRuntime::create(24 * 1024 * 1024);
        test_sdk_boundary(runtime);
        test_alias_lifetime(runtime);
        test_identity(runtime);
        test_retail(runtime, mem1);
        runtime.reset();
        require(JKRHeap::sRootHeap == nullptr, "all model resource domains retire before original root heap");
        std::cout << "[pass] 4 original complete-model resource groups\n";
    } catch (const std::exception& error) {
        std::cerr << "[fail] original complete model: " << error.what() << '\n';
        return 1;
    }
}
