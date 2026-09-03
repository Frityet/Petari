#include "compat/ResourceHolderCompat.hpp"
#include "compat/J3dCommandScope.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "runtime/RuntimeServices.hpp"
#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "Game/Animation/BpkPlayer.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <dolphin/gd.h>
#include <dolphin/os.h>

#include <array>
#include <bit>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    using namespace smgpc::resource;
    using namespace smgpc::compat;
    using Bytes = std::vector<std::uint8_t>;
    void require(bool condition, const char* message) {
        if (!condition) { JkrHostAllocationScope host; throw std::runtime_error(message); }
    }
    template<class F> void rejects(F call, const char* message) {
        bool caught = false;
        try { call(); } catch (const std::exception&) { caught = true; }
        require(caught, message);
    }
    void put16(Bytes& b, std::size_t p, unsigned v) { b.at(p) = v >> 8; b.at(p + 1) = v; }
    void put32(Bytes& b, std::size_t p, std::uint32_t v) {
        put16(b, p, v >> 16); put16(b, p + 2, v);
    }
    void tag(Bytes& b, std::size_t p, std::string_view s) { std::memcpy(b.data() + p, s.data(), s.size()); }
    struct File { std::string name; Bytes bytes; bool nested = false; };
    std::shared_ptr<RarcArchive> archive(std::vector<File> input) {
        // Three real directory records: root, nested, empty, and their dot links.
        // Payload IDs deliberately differ from file-table indexes.
        Bytes strings;
        auto str = [&](std::string_view s) {
            auto p = strings.size(); strings.insert(strings.end(), s.begin(), s.end()); strings.push_back(0); return p;
        };
        auto root = str("fixture_root"), nest = str("nested"), empty = str("empty"), dot = str("."), parent = str("..");
        std::vector<std::size_t> names;
        for (const auto& f : input) names.push_back(str(f.name));
        const std::size_t roots = std::ranges::count_if(input, [](const auto& f) { return !f.nested; });
        const std::size_t count = input.size() + 8, dirs = 0x40, files = dirs + 48;
        const auto string_offset = files + count * 20;
        const auto data_offset = (string_offset + strings.size() + 31) & ~std::size_t{31};
        Bytes data;
        std::vector<std::size_t> offsets;
        for (const auto& f : input) {
            while (data.size() % 32) data.push_back(0);
            offsets.push_back(data.size()); data.insert(data.end(), f.bytes.begin(), f.bytes.end());
        }
        Bytes out(data_offset + data.size());
        tag(out, 0, "RARC"); put32(out, 4, out.size()); put32(out, 8, 0x20);
        put32(out, 12, data_offset - 0x20); put32(out, 16, data.size());
        put32(out, 0x20, 3); put32(out, 0x24, dirs - 0x20);
        put32(out, 0x28, count); put32(out, 0x2c, files - 0x20);
        put32(out, 0x30, strings.size()); put32(out, 0x34, string_offset - 0x20); put16(out, 0x38, 200);
        auto directory = [&](unsigned index, unsigned name, unsigned n, unsigned first) {
            auto p = dirs + index * 16; tag(out, p, "ROOT"); put32(out, p + 4, name);
            put16(out, p + 8, RarcArchive::hash_name(reinterpret_cast<const char*>(strings.data() + name)));
            put16(out, p + 10, n); put32(out, p + 12, first);
        };
        directory(0, root, roots + 4, 0);
        directory(1, nest, input.size() - roots + 2, roots + 4);
        directory(2, empty, 2, input.size() + 6);
        auto entry = [&](unsigned index, unsigned id, unsigned name, unsigned flags, unsigned offset, unsigned size) {
            auto p = files + index * 20; put16(out, p, id);
            put16(out, p + 2, RarcArchive::hash_name(reinterpret_cast<const char*>(strings.data() + name)));
            put32(out, p + 4, (flags << 24) | name); put32(out, p + 8, offset); put32(out, p + 12, size);
        };
        unsigned index = 0;
        for (std::size_t i = 0; i < input.size(); ++i)
            if (!input[i].nested) entry(index++, 100 + i, names[i], 0x11, offsets[i], input[i].bytes.size());
        entry(index++, 0xffff, nest, 2, 1, 16); entry(index++, 0xffff, empty, 2, 2, 16);
        entry(index++, 0xffff, dot, 2, 0, 16); entry(index++, 0xffff, parent, 2, 0xffffffff, 16);
        for (std::size_t i = 0; i < input.size(); ++i)
            if (input[i].nested) entry(index++, 100 + i, names[i], 0x11, offsets[i], input[i].bytes.size());
        entry(index++, 0xffff, dot, 2, 1, 16); entry(index++, 0xffff, parent, 2, 0, 16);
        entry(index++, 0xffff, dot, 2, 2, 16); entry(index++, 0xffff, parent, 2, 0, 16);
        require(index == count, "fixture directory count");
        std::copy(strings.begin(), strings.end(), out.begin() + string_offset);
        std::copy(data.begin(), data.end(), out.begin() + data_offset);
        return std::make_shared<RarcArchive>(RarcArchive::from_bytes(std::move(out)));
    }
    Bytes file(std::string_view type, Bytes block) {
        Bytes b(0x20); tag(b, 0, "J3D1"); tag(b, 4, type); put32(b, 12, 1);
        b.insert(b.end(), block.begin(), block.end()); put32(b, 8, b.size()); return b;
    }
    Bytes transform(bool key) {
        Bytes b(0x24); tag(b, 0, key ? "ANK1" : "ANF1"); b[8] = 3; b[9] = 2;
        put16(b, 0xa, 7); put16(b, 0xc, 1);
        for (int p : {0xe, 0x10, 0x12}) put16(b, p, key ? 1 : 2);
        put32(b, 0x14, b.size()); const auto table = b.size(); b.resize(table + (key ? 54 : 36));
        for (int i = 0; i < 9; ++i) put16(b, table + i * (key ? 6 : 4), key ? 1 : 2);
        const auto f = [&](int field, float a, float c) {
            put32(b, field, b.size()); const auto p = b.size(); b.resize(p + (key ? 4 : 8));
            put32(b, p, std::bit_cast<std::uint32_t>(a)); if (!key) put32(b, p + 4, std::bit_cast<std::uint32_t>(c));
        };
        f(0x18, 2, 3); put32(b, 0x1c, b.size()); auto p = b.size(); b.resize(p + 4);
        put16(b, p, -3); put16(b, p + 2, 30); f(0x20, 4, 9); put32(b, 4, b.size());
        return file(key ? "bck1" : "bca1", std::move(b));
    }
    constexpr auto long_name = "AuthoredAnimationNameRetainedBeyondLocalJMapReader";
    Bytes control() {
        constexpr std::array fields{"name", "interpole", "play_frame", "start_frame", "end_frame", "attribute"};
        const auto start = 16 + fields.size() * 12, stride = fields.size() * 4;
        Bytes b(start + stride * 2); put32(b, 0, 2); put32(b, 4, fields.size()); put32(b, 8, start); put32(b, 12, stride);
        for (std::size_t i = 0; i < fields.size(); ++i) {
            auto p = 16 + i * 12; put32(b, p, jmap_hash(fields[i])); put32(b, p + 4, 0xffffffff);
            put16(b, p + 8, i * 4); b[p + 11] = i == 0 ? 6 : 0;
        }
        for (int row = 0; row < 2; ++row) {
            auto p = start + row * stride; auto name = std::string_view(row ? long_name : "_default");
            put32(b, p, b.size() - (start + stride * 2));
            for (int col = 1; col < 6; ++col) put32(b, p + col * 4, row ? col + 2 : col);
            b.insert(b.end(), name.begin(), name.end()); b.push_back(0);
        }
        return b;
    }
    Bytes color(std::string_view name) {
        Bytes b(0x34); tag(b, 0, "PAK1"); b[8] = 3; put16(b, 0xa, 7); put16(b, 0xc, 7); put16(b, 0xe, 1);
        for (int p = 0x10; p < 0x18; p += 2) put16(b, p, 1);
        put32(b, 0x18, b.size()); auto p = b.size(); b.resize(p + 24);
        for (int c = 0; c < 4; ++c) put16(b, p + c * 6, 1);
        put32(b, 0x1c, b.size()); p = b.size(); b.resize(p + 4); put16(b, p, 0xffff);
        put32(b, 0x20, b.size()); p = b.size(); b.resize(p + 8 + name.size() + 1);
        put16(b, p, 1); put16(b, p + 2, 0xffff); put16(b, p + 4, RarcArchive::hash_name(name)); put16(b, p + 6, 8);
        tag(b, p + 8, name);
        for (int c = 0; c < 4; ++c) {
            put32(b, 0x24 + c * 4, b.size()); p = b.size(); b.resize(p + 4); put16(b, p, 10 + c * 20);
        }
        put32(b, 4, b.size()); return file("bpk1", std::move(b));
    }

    void test_original_csv_reader(GameResourceRuntime& process) {
        // A raw filename deliberately has no recognized table extension. The
        // original parser decides its type; the archive service supplies bounds.
        constexpr std::array fields{"name", "frame", "flag", "value", "vectorX", "vectorY"};
        constexpr unsigned start = 16 + fields.size() * 12, stride = fields.size() * 4;
        Bytes bytes(start + 2 * stride);
        put32(bytes, 0, 2); put32(bytes, 4, fields.size()); put32(bytes, 8, start); put32(bytes, 12, stride);
        for (unsigned i = 0; i < fields.size(); ++i) {
            const auto p = 16 + i * 12;
            put32(bytes, p, jmap_hash(fields[i]));
            put32(bytes, p + 4, i == 2 ? 0x40 : 0xffffffff);
            put16(bytes, p + 8, i * 4);
            bytes[p + 11] = i == 0 ? 6 : (i == 1 || i >= 4) ? 2 : 0;
        }
        put32(bytes, start + 4, std::bit_cast<std::uint32_t>(12.5f));
        put32(bytes, start + 8, 0x40); put32(bytes, start + 12, 0x1fe);
        put32(bytes, start + 16, std::bit_cast<std::uint32_t>(1.25f));
        put32(bytes, start + 20, std::bit_cast<std::uint32_t>(-0.5f));
        put32(bytes, start + stride, std::strlen(long_name) + 1);
        put32(bytes, start + stride + 4, std::bit_cast<std::uint32_t>(-2.25f));
        bytes.insert(bytes.end(), long_name, long_name + std::strlen(long_name) + 1);
        bytes.push_back(0);
        auto source = archive({{"authored.table", std::move(bytes)}, {"unrelated.raw", {1, 2, 3}}});
        const std::weak_ptr<const RarcArchive> weak_source = source;
        auto domain = process.create_cohort();
        auto owner = std::make_unique<ResourceArchiveOwner>(source, "/retained/Csv.arc", domain, process.mem1_heap());
        auto& holder = owner->holder();
        require(MR::isExistFileInArc(&holder, "%s.%s", "authored", "table"), "original variadic filename lookup");
        require(MR::tryCreateCsvParser(&holder, "%s.table", "missing") == nullptr, "missing optional table remains null");
        std::unique_ptr<JMapInfo> parser(MR::createCsvParser(&holder, "%s.%s", "authored", "table"));
        require(parser && MR::getCsvDataElementNum(parser.get()) == 2, "original parser attaches raw archive table");
        const void* identity = holder.mFileInfoTable->getRes("authored.table");
        const char* name = nullptr;
        MR::getCsvDataStr(&name, parser.get(), "name", 0);
        require(name && std::string_view(name) == long_name, "original string field");
        const char* empty = "initial";
        MR::getCsvDataStrOrNULL(&empty, parser.get(), "name", 1);
        require(empty == nullptr, "original empty string becomes null");
        float frame = 0;
        MR::getCsvDataF32(&frame, parser.get(), "frame", 0);
        require(frame == 12.5f, "authored fractional frame decoded without integer conversion");
        MR::getCsvDataF32(&frame, parser.get(), "missing", 0);
        require(frame == 12.5f, "missing float preserves original caller default");
        MR::getCsvDataF32(&frame, parser.get(), "frame", 1);
        require(frame == -2.25f, "second row reads negative authored float");
        Vec vector{7, 8, 9};
        MR::getCsvDataVec(&vector, parser.get(), "vector", 0);
        require(vector.x == 1.25f && vector.y == -0.5f && vector.z == 9,
                "original vector reader formats XYZ keys and preserves missing component defaults");
        bool flag = false;
        MR::getCsvDataBool(&flag, parser.get(), "flag", 0);
        require(flag, "original masked boolean");
        MR::getCsvDataBool(&flag, parser.get(), "missing", 0);
        require(flag, "missing boolean preserves original caller default");
        MR::getCsvDataBool(&flag, parser.get(), "flag", 1);
        require(!flag, "zero masked boolean");
        u8 small = 0;
        MR::getCsvDataU8(&small, parser.get(), "value", 0);
        require(small == 0xfe, "original byte conversion truncates to eight bits");
        MR::getCsvDataU8(&small, parser.get(), "missing", 0);
        require(small == 0, "original missing byte uses zero local");
        rejects([&] { JMapInfo invalid; invalid.attach(holder.mFileInfoTable->getRes("unrelated.raw")); },
                "unrelated bytes are parsed only when explicitly attached and then rejected");
        owner.reset(); source.reset(); domain.reset();
        require(weak_source.expired() && !find_jmap_resource(identity), "archive retirement removes raw identity and original owner");
        const char* surviving = nullptr;
        MR::getCsvDataStr(&surviving, parser.get(), "name", 0);
        require(surviving == name && std::string_view(surviving) == long_name, "attached parser retains decoded names independently");
    }

    void test_original_constructor(GameResourceRuntime& process) {
        auto source = archive({{"Key.bck", transform(true)}, {"Full.bca", transform(false), true},
                               {"fixture_root.banmt", control()}, {"Mixed.BCK", {7, 8}, true}, {"raw.pa", {9}, true},
                               {"Empty.bck", {}}});
        const std::weak_ptr<const RarcArchive> weak_source = source;
        auto domain = process.create_cohort(); const std::weak_ptr<JkrAllocationDomain> weak_domain = domain;
        auto owner = std::make_unique<ResourceArchiveOwner>(source, "/retained/Fixture.arc", domain, process.mem1_heap());
        auto& h = owner->holder();
        require(h.mHeap == &domain->heap() && JKRHeap::findFromRoot(&h) == h.mHeap, "actual holder allocated in mounted cohort");
        require(h.mMotionResTable->mCount == 3 && h.mBanmtResTable->mCount == 1 && h.mFileInfoTable->mCount == 2,
                "original case-sensitive dispatch and recursive counts");
        const auto* empty_info = h.mMotionResTable->findFileInfo("Empty");
        require(empty_info && empty_info->mResource == nullptr && empty_info->_8 == nullptr && empty_info->_4 == 0,
                "original zero-length BCK preserves its explicit null resource entry");
        require(h.mModelResTable == &h.mDefaultTable && h.mMaterialBuf == nullptr && h.mBackupMaterialData == nullptr,
                "actual default table and absent model state");
        const auto* key_info = h.mMotionResTable->findFileInfo("KEY");
        require(key_info && key_info->_C == 100 && key_info->_8 == source->resource_data("Key.bck").data() &&
                key_info->_4 == transform(true).size() && key_info->mResource != key_info->_8,
                "actual hashed names, non-index file ID, raw metadata and separate typed object");
        require(h.mFileInfoTable->getRes("raw.pa") == source->resource_data("raw.pa").data(), "raw resources preserve byte identity");
        auto* key = dynamic_cast<J3DAnmTransformKey*>(static_cast<J3DAnmBase*>(key_info->mResource));
        auto* full = dynamic_cast<J3DAnmTransformFull*>(static_cast<J3DAnmBase*>(h.mMotionResTable->getRes("Full")));
        require(key && full && key->getFrameMax() == 7 && key->mAttribute == 3, "original concrete animation classes and metadata");
        J3DTransformInfo result; key->mFrame = 4; key->getTransform(0, &result);
        require(result.mScale.x == 2 && result.mRotation.x == -12 && result.mTranslate.x == 4, "original key sampler");
        full->mFrame = 1; full->getTransform(0, &result);
        require(result.mScale.x == 3 && result.mRotation.x == 30 && result.mTranslate.x == 9, "original full sampler");
        require(h.mBckCtrl && h.mBckCtrl->mDefaultCtrlData.mPlayFrame == 2, "actual original BckCtrl default row");
        const auto* row = h.mBckCtrl->find(long_name);
        require(row && row->mPlayFrame == 4 && row->mInterpole == 3 && row->mLoopMode == 7, "actual authored BckCtrl settings");
        auto* borrowed = row->mName;
        require(JKRHeap::findFromRoot(const_cast<char*>(borrowed)) == nullptr, "shared JMap string cache is host-owned");
        source.reset(); domain.reset();
        for (int i = 0; i < 16; ++i) {
            auto churn = JMapResource(control());
            require(std::string_view(borrowed) == long_name && h.mBckCtrl->find(long_name)->mName == borrowed,
                    "borrowed name survives local reader/source destruction and other tables");
        }
        require(!weak_source.expired() && !weak_domain.expired(), "typed owner retains source and Game arena");
        const auto* raw_map = h.mBanmtResTable->getRes("fixture_root");
        owner.reset();
        require(weak_source.expired() && weak_domain.expired() && !find_jmap_resource(raw_map), "full teardown removes aliases before source and arena expire");
    }

    void test_failure_scope(GameResourceRuntime& process) {
        Bytes bad(0x20); tag(bad, 0, "J3D2bdl4"); put32(bad, 8, bad.size());
        auto source = archive({{"bad.bdl", bad}});
        auto domain = process.create_cohort();
        alignas(32) std::array<u8, 64> bytes{}; GDLObj prior{}; GDInitGDLObj(&prior, bytes.data(), bytes.size());
        auto* old_gd = __GDCurrentDL; GDSetCurrent(&prior);
        auto* old_heap = JKRHeap::sCurrentHeap;
        OSLockMutex(&MR::MutexHolder<0>::sMutex); OSLockMutex(&MR::MutexHolder<0>::sMutex);
        rejects([&] { ResourceArchiveOwner failed(source, "bad.arc", domain, process.mem1_heap()); }, "malformed real loader must fail construction");
        require(MR::MutexHolder<0>::sMutex.thread == OSGetCurrentThread() && MR::MutexHolder<0>::sMutex.count == 2,
                "host exception restores exactly caller's recursive load-mutex depth");
        require(__GDCurrentDL == &prior && JKRHeap::sCurrentHeap == old_heap, "host exception restores GD and current heap");
        OSUnlockMutex(&MR::MutexHolder<0>::sMutex); OSUnlockMutex(&MR::MutexHolder<0>::sMutex);
        auto previous = OSDisableInterrupts(); require(previous, "failure releases added interrupt suppression"); OSRestoreInterrupts(previous);
        require(OSDisableScheduler() == 0, "failure releases added scheduler suppression"); OSEnableScheduler();
        GDSetCurrent(old_gd);
        bool acquired = false;
        std::thread other([&] { acquired = OSTryLockMutex(&MR::MutexHolder<0>::sMutex); if (acquired) OSUnlockMutex(&MR::MutexHolder<0>::sMutex); });
        other.join(); require(acquired, "another SDK thread can acquire load mutex after failure");
        ResourceArchiveOwner valid(archive({{"raw", {1}}}), "valid.arc", domain, process.mem1_heap());
        require(valid.holder().mFileInfoTable->mCount == 1, "failed holder never prevents subsequent valid construction");
    }

    void test_service_lifetime(GameResourceRuntime& process) {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        if (!disc || !*disc) { std::cout << "SKIP real DVD cache/service lifetime (SMGPC_REAL_DISC)\n"; return; }
        require(aurora_dvd_open(disc), "real-disc fixture opens for DVD lifetime");
        struct Close { ~Close() { aurora_dvd_close(); } } close;
        std::shared_ptr<const ResourceArchiveOwner> retained;
        {
            smgpc::runtime::DvdFileSystemService dvd("/");
            auto temporary_domain = process.create_cohort();
            const std::weak_ptr<JkrAllocationDomain> weak_temporary = temporary_domain;
            RarcArchive* cached = nullptr;
            {
                JkrAllocationScope original(temporary_domain);
                cached = &dvd.archive("/ObjectData/InvisibleWall10x10.arc");
                require(JKRHeap::findFromRoot(cached) == nullptr &&
                            JKRHeap::findFromRoot(const_cast<u8*>(cached->bytes().data())) == nullptr,
                        "direct DVD cache entry and retained byte buffer escape temporary Game allocations");
            }
            temporary_domain.reset();
            require(weak_temporary.expired() && cached->resource_data("InvisibleWall10x10.kcl").size() == 1222,
                    "DVD cache survives the original scope and allocation domain that first requested it");
            ResourceHolderService service(dvd, process.create_cohort(), process.mem1_heap());
            auto* h = service.create_and_add("InvisibleWall10x10.arc");
            require(h == service.create_and_add("/ObjectData/InvisibleWall10x10.arc"), "exact resolved archive identity deduplicates holder");
            retained = service.retain(*h);
            require(dvd.archive_load_count("/ObjectData/InvisibleWall10x10.arc") == 1, "retained raw handle preserves DVD cache count");
        }
        require(ResourceHolderService::active() == nullptr, "service unregisters at teardown");
        require(retained->holder().mArchive->getResSize(retained->holder().mFileInfoTable->getRes("CollisionVersion")) == 7,
                "explicit holder lease survives original DVD cache/service destruction");
        retained.reset();
        smgpc::runtime::DvdFileSystemService fresh("/");
        ResourceHolderService next(fresh, process.create_cohort(), process.mem1_heap());
        require(ResourceHolderService::active() == &next, "same process heaps support a subsequent resource context without OS reinit");
    }

    void test_real_model_and_material(GameResourceRuntime& process) {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        if (!disc || !*disc) { std::cout << "SKIP real model plus material-animation holder (SMGPC_REAL_DISC)\n"; return; }
        require(aurora_dvd_open(disc), "real-disc fixture opens");
        struct Close { ~Close() { aurora_dvd_close(); } } close;
        smgpc::runtime::DvdFileSystemService dvd("/");
        ResourceHolderService service(dvd, process.create_cohort(), process.mem1_heap());
        auto* original = service.create_and_add("Mario.arc");
        auto* model = static_cast<J3DModelData*>(original->mModelResTable->getRes(original->getModelName()));
        require(model && model->getMaterialNum() == 9, "real Mario holder contains actual complete nine-material model");
        const auto source = service.backing(*original).archive().resource_data("Mario.bdl");
        const auto material_name = std::string_view(model->getMaterialName()->getName(0));
        const auto before = process.mem1_heap()->available_bytes();
        {
            auto mixed = archive({{"Model.bdl", Bytes(source.begin(), source.end())}, {"Color.bpk", color(material_name)}});
            ResourceArchiveOwner owner(mixed, "Mixed.arc", service.allocation_domain(), process.mem1_heap());
            auto& h = owner.holder(); auto* m = static_cast<J3DModelData*>(h.mModelResTable->getRes("Model"));
            std::vector<std::array<float, 16>> initial_effect_matrices(m->getMaterialNum() * 8);
            for (unsigned i = 0; i < m->getMaterialNum(); ++i) for (unsigned j = 0; j < 8; ++j) {
                auto* tex = m->getMaterialNodePointer(i)->getTexGenBlock()->getTexMtx(j);
                for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col) {
                    const auto expected = tex ? tex->getTexMtxInfo().mEffectMtx[row][col] : float(row == col);
                    initial_effect_matrices[i * 8 + j][row * 4 + col] = expected;
                    require(h.getInitEffectMtx(i, j)[row][col] == expected, "original effect-matrix copy/identity backup");
                }
            }
            auto* animation = static_cast<J3DAnmColorKey*>(h.mBpkResTable->getRes("Color"));
            require(h.isCreatedAtSameHeap(original) && h.mMaterialBuf != nullptr, "same cohort identity and original combined material-animation constructor");
            require(animation->getUpdateMaterialID(0) == 0 && h.mMaterialBuf->getDiffFlag(0) != 0 &&
                    m->getMaterialNodePointer(0)->getMaterialAnm() == h.mMaterialBuf->_0,
                    "actual material-name lookup, diff flags and attached original material-animation array");
            BpkPlayer player(&h, m);
            player.start("Color");
            player.update();
            require(player.isPlaying("color") && !player.isStop() && player.mFrameCtrl.getFrame() == 1 && animation->mFrame == 0,
                    "original player advances its controller before reflecting the resource frame");
            player.beginDiff();
            auto* material = m->getMaterialNodePointer(0);
            material->getMaterialAnm()->calc(material);
            auto* rgba = material->getColorBlock()->getMatColor(0);
            require(animation->mFrame == 1 && rgba->r == 10 && rgba->g == 30 && rgba->b == 50 && rgba->a == 70,
                    "original player attachment and material calc apply the authored BPK channels");
            player.endDiff();
            rgba->r = 201; rgba->g = 202; rgba->b = 203; rgba->a = 204;
            material->getMaterialAnm()->calc(material);
            require(rgba->r == 201 && rgba->g == 202 && rgba->b == 203 && rgba->a == 204,
                    "original endDiff removes the material animator");
            player.stop();
            player.update();
            require(player.isStop() && player.mAnmRes == animation && player.mFrameCtrl.getFrame() == 1,
                    "original stop retains the selected resource while stopping its frame controller");
            auto* first_tex = m->getMaterialNodePointer(0)->getTexGenBlock()->getTexMtx(0);
            require(first_tex != nullptr, "real Mario material has an authored texture matrix");
            first_tex->getTexMtxInfo().mEffectMtx[0][0] = 7.0f;
            for (unsigned i = 0; i < m->getMaterialNum(); ++i) for (unsigned j = 0; j < 8; ++j) {
                for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col) {
                    require(h.getInitEffectMtx(i, j)[row][col] == initial_effect_matrices[i * 8 + j][row * 4 + col],
                            "original backup remains independent of live material animation and effect-matrix mutation");
                }
            }
        }
        require(process.mem1_heap()->available_bytes() == before, "actual model texture owner releases its mapped MEM1 allocations");
        std::cout << "resource cohort used=" << service.allocation_domain()->heap().mSize - service.allocation_domain()->heap().getTotalFreeSize()
                  << " MEM1 available=" << process.mem1_heap()->available_bytes() << '\n';
    }
}
int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        GameResourceRuntime process;
        test_original_constructor(process); std::cout << "PASS original holder, typed animation, control table and lifetime\n";
        test_original_csv_reader(process); std::cout << "PASS original CSV helpers and deferred archive tables\n";
        test_failure_scope(process); std::cout << "PASS original loader exception restoration\n";
        test_service_lifetime(process); std::cout << "PASS service/archive ownership and process reuse\n";
        test_real_model_and_material(process); std::cout << "PASS real model/material holder\n";
    } catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
