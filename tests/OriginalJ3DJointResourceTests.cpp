#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include "JSystem/JSupport/JSupport.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"
#include "resource/J3dJointData.hpp"
#include "resource/J3dNameData.hpp"
#include "resource/J3dNativeBlock.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using Bytes = std::vector<std::uint8_t>;
    using View = std::span<const std::uint8_t>;
    using smgpc::resource::J3dJointData;

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void write16(Bytes& bytes, std::size_t offset, std::uint16_t value) {
        bytes.at(offset) = static_cast<std::uint8_t>(value >> 8U);
        bytes.at(offset + 1) = static_cast<std::uint8_t>(value);
    }

    void write32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
        write16(bytes, offset, static_cast<std::uint16_t>(value >> 16U));
        write16(bytes, offset + 2, static_cast<std::uint16_t>(value));
    }

    void write_float(Bytes& bytes, std::size_t offset, float value) {
        write32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    std::uint16_t read16(View bytes, std::size_t offset) {
        return static_cast<std::uint16_t>((std::uint16_t{bytes[offset]} << 8U) | bytes[offset + 1]);
    }

    std::uint32_t read32(View bytes, std::size_t offset) {
        return (std::uint32_t{read16(bytes, offset)} << 16U) | read16(bytes, offset + 2);
    }

    void same_float(float actual, View bytes, std::size_t offset) {
        require(std::bit_cast<std::uint32_t>(actual) == read32(bytes, offset), "native float must preserve its authored bits");
    }

    void tag(Bytes& bytes, std::size_t offset, std::string_view value) {
        std::copy(value.begin(), value.end(), bytes.begin() + offset);
    }

    std::size_t block_offset(View bytes, std::string_view name) {
        std::size_t offset = 0x20;
        for (std::size_t i = 0; i < read32(bytes, 0xc); ++i) {
            if (std::memcmp(bytes.data() + offset, name.data(), 4) == 0) {
                return offset;
            }
            offset += read32(bytes, offset + 4);
        }
        throw std::runtime_error("fixture block is absent");
    }

    View block(View bytes, std::string_view name) {
        const auto offset = block_offset(bytes, name);
        return bytes.subspan(offset, read32(bytes, offset + 4));
    }

    std::uint16_t name_hash(std::string_view name) {
        std::uint16_t value = 0;
        for (char c : name) {
            value = static_cast<std::uint16_t>(value * 3 + c);
        }
        return value;
    }

    Bytes fixture(std::uint16_t mode = 0, bool envelopes = true, bool names = true) {
        Bytes info(0x40);
        tag(info, 0, "INF1");
        write16(info, 8, static_cast<std::uint16_t>(0x1200 | mode));
        write32(info, 0xc, 17);
        write32(info, 0x10, 59);
        write32(info, 0x14, 0x18);
        constexpr std::array<J3DModelHierarchy, 9> hierarchy{{
            {0x10, 0}, {1, 0}, {0x11, 0}, {0x12, 0}, {0x10, 1},
            {1, 0}, {2, 0}, {2, 0}, {0, 0}}};
        for (std::size_t i = 0; i < hierarchy.size(); ++i) {
            write16(info, 0x18 + i * 4, hierarchy[i].mType);
            write16(info, 0x1a + i * 4, hierarchy[i].mValue);
        }
        Bytes joint(0xc0);
        tag(joint, 0, "JNT1");
        write16(joint, 8, 2);
        write32(joint, 0xc, 0x20);
        write32(joint, 0x10, 0x18);
        write32(joint, 0x14, names ? 0xa0 : 0);
        write16(joint, 0x18, 1);
        write16(joint, 0x1a, 0);
        for (std::size_t i = 0; i < 2; ++i) {
            const auto offset = 0x20 + i * 0x40;
            write16(joint, offset, i == 0 ? 0x1234 : 0x56);
            joint[offset + 2] = i == 0 ? 0xff : 2;
            for (std::size_t axis = 0; axis < 3; ++axis) {
                write_float(joint, offset + 4 + axis * 4, static_cast<float>(2 + i + axis));
                write_float(joint, offset + 0x18 + axis * 4, static_cast<float>(11 * (axis + 1) + i));
                write_float(joint, offset + 0x28 + axis * 4, -static_cast<float>(1 + i + axis));
                write_float(joint, offset + 0x34 + axis * 4, static_cast<float>(1 + i + axis));
            }
            write16(joint, offset + 0x10, static_cast<std::uint16_t>(0x8000 + i));
            write16(joint, offset + 0x12, static_cast<std::uint16_t>(0x1234 + i));
            write16(joint, offset + 0x14, static_cast<std::uint16_t>(0x7fff - i));
            write_float(joint, offset + 0x24, static_cast<float>(5 + i));
        }
        if (names) {
            write16(joint, 0xa0, 2);
            write16(joint, 0xa2, 0xabcd);
            write16(joint, 0xa4, name_hash("Root"));
            write16(joint, 0xa6, 12);
            write16(joint, 0xa8, name_hash("Child"));
            write16(joint, 0xaa, 17);
            tag(joint, 0xac, "Root");
            tag(joint, 0xb1, "Child");
        }
        Bytes envelope(0xa0);
        tag(envelope, 0, "EVP1");
        write16(envelope, 8, envelopes ? 1 : 0);
        write32(envelope, 0xc, envelopes ? 0x1c : 0);
        write32(envelope, 0x10, envelopes ? 0x20 : 0);
        write32(envelope, 0x14, envelopes ? 0x28 : 0);
        write32(envelope, 0x18, 0x40);
        envelope[0x1c] = 2;
        write16(envelope, 0x20, 1);
        write16(envelope, 0x22, 0);
        write_float(envelope, 0x28, 0.5F);
        write_float(envelope, 0x2c, 0.5F);
        for (std::size_t matrix = 0; matrix < 2; ++matrix) {
            for (std::size_t component = 0; component < 12; ++component) {
                write_float(envelope, 0x40 + matrix * 48 + component * 4,
                            static_cast<float>(matrix * 100 + component) + 0.25F);
            }
        }
        Bytes draw(0x40);
        tag(draw, 0, "DRW1");
        write16(draw, 8, envelopes ? 4 : 2);
        write32(draw, 0xc, 0x18);
        write32(draw, 0x10, 0x20);
        draw[0x18] = envelopes ? 2 : 0;
        draw[0x1a] = 1;
        draw[0x1b] = 0xfe;
        write16(draw, 0x20, 0);
        write16(draw, 0x22, 1);
        write16(draw, 0x24, 0);
        write16(draw, 0x26, 0xffff);
        Bytes result(0x20);
        tag(result, 0, "J3D2");
        tag(result, 4, "bmd3");
        write32(result, 0xc, 4);
        for (auto* section : {&info, &joint, &envelope, &draw}) {
            write32(*section, 4, static_cast<std::uint32_t>(section->size()));
            result.insert(result.end(), section->begin(), section->end());
        }
        write32(result, 8, static_cast<std::uint32_t>(result.size()));
        return result;
    }

    void test_decoded_ownership() {
        auto bytes = fixture();
        J3dJointData resource(bytes, 0x20000000);
        std::unique_ptr<J3dJointData> moved;
        J3DModelData model;
        int retained_raw_owner = 0;
        model.mpRawData = &retained_raw_owner;
        resource.attach_to(model);
        auto& tree = model.mJointTree;
        require(model.mFlags == 0x20001200 && tree.mFlags == model.mFlags && tree.mModelDataType == 0,
                "INF flags combine with original load flags and the BMD data type");
        require(model.mVertexData.mPacketNum == 17 && model.mVertexData.mVtxNum == 59,
                "INF packet/vertex counts reach the actual embedded vertex data");
        require(model.mpRawData == &retained_raw_owner && tree.mRootNode == nullptr && tree.mJointNum == 2,
                "component preserves the complete owner's raw data and defers hierarchy finalization");
        auto* first = model.getJointNodePointer(0);
        auto* second = model.getJointNodePointer(1);
        require(first->mJntNo == 0 && first->mKind == 0x56 && first->mScaleCompensate == 2 &&
                    second->mJntNo == 1 && second->mKind == 0x34 && second->mScaleCompensate == 0,
                "real joints apply remap, low-byte kind, byte compensation and the original ff sentinel");
        require(first->mTransformInfo.mScale.x == 3 && first->mTransformInfo.mScale.y == 4 &&
                    first->mTransformInfo.mScale.z == 5 && first->mTransformInfo.mRotation.x == -32767 &&
                    first->mTransformInfo.mRotation.y == 0x1235 && first->mTransformInfo.mRotation.z == 32766 &&
                    first->mTransformInfo.mTranslate.x == 12 && first->mTransformInfo.mTranslate.y == 23 &&
                    first->mTransformInfo.mTranslate.z == 34 && first->mBoundingSphereRadius == 6 &&
                    first->mMin.z == -4 && first->mMax.y == 3,
                "all remapped SRT and bound fields are native original joint data");
        require(first->mChild == nullptr && first->mYounger == nullptr && first->mMesh == nullptr && first->mMtxCalc == nullptr,
                "component does not invent any joint/material links");
        require(tree.mHierarchy[2].mType == 0x11 && tree.mHierarchy[3].mType == 0x12 && tree.mHierarchy[8].mType == 0,
                "material/shape commands remain present for complete original finalization");
        auto* names = tree.mJointName;
        require(names != nullptr && names->mResource->_2 == 0xabcd && names->mResource->mEntries[0].mKeyCode == name_hash("Root") &&
                    names->mResource->mEntries[1].mOffs == 17 && names->getIndex("Child") == 1,
                "the original JUTNameTab sees retained native header, hash and offset records");
        require(tree.mWEvlpMtxNum == 1 && tree.mWEvlpMixMtxNum[0] == 2 && tree.mWEvlpMixMtxIndex[0] == 1 &&
                    tree.mWEvlpMixWeight[1] == 0.5F && tree.mInvJointMtx[1][2][3] == 111.25F,
                "original envelope pointers retain every authored typed array");
        require(tree.mDrawMtxData.mEntryNum == 3 && tree.mDrawMtxData.mDrawFullWgtMtxNum == 2 &&
                    tree.mDrawMtxData.mDrawMtxFlag[0] == 2 && tree.mDrawMtxData.mDrawMtxFlag[3] == 0xfe &&
                    tree.mDrawMtxData.mDrawMtxIndex[3] == 0xffff,
                "retail draw-count subtraction and exact flag==1 scan preserve complete raw arrays");
        tree.findImportantMtxIndex();
        require(tree.mWEvlpImportantMtxIdx[0] == 0 && tree.mWEvlpImportantMtxIdx[1] == 1 && tree.mWEvlpImportantMtxIdx[2] == 1,
                "original important-matrix selection keeps the first equal-weight influence");
        std::fill(bytes.begin(), bytes.end(), 0xcc);
        bytes.clear();
        bytes.shrink_to_fit();
        moved = std::make_unique<J3dJointData>(std::move(resource));
        require(std::strcmp(names->getName(0), "Root") == 0 && names->getIndex("Child") == 1 &&
                    first->mTransformInfo.mTranslate.z == 34 && tree.mInvJointMtx[1][2][3] == 111.25F,
                "all pointers survive source retirement and component move");
        bool rejected = false;
        try { moved->attach_to(model); } catch (const std::logic_error&) { rejected = true; }
        require(rejected && model.getJointNodePointer(0) == first, "reattachment must not overwrite a live original tree");
    }

    void test_modes_and_absence() {
        for (std::uint16_t mode : {0, 1, 2}) {
            auto bytes = fixture(mode, false, false);
            tag(bytes, 4, "bdl4");
            J3dJointData resource(bytes);
            J3DModelData model;
            resource.attach_to(model);
            auto* calculator = model.getJointTree().mBasicMtxCalc;
            bool correct = false;
            if (mode == 0) correct = dynamic_cast<J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic>*>(calculator) != nullptr;
            if (mode == 1) correct = dynamic_cast<J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformSoftimage, J3DMtxCalcJ3DSysInitSoftimage>*>(calculator) != nullptr;
            if (mode == 2) correct = dynamic_cast<J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformMaya, J3DMtxCalcJ3DSysInitMaya>*>(calculator) != nullptr;
            require(correct && model.getModelDataType() == 1, "each authored scale mode constructs its actual original calculator");
            require(model.getJointName() == nullptr && model.getWEvlpMtxNum() == 0 && model.getDrawMtxNum() == 2 &&
                        model.getJointTree().getDrawFullWgtMtxNum() == 2 && model.getInvJointMtx(1)[0][0] == 100.25F,
                    "absent names/envelopes remain absent while an unreferenced inverse table remains retained");
            model.getJointTree().findImportantMtxIndex();
            require(model.getWEvlpImportantMtxIndex()[0] == 0 && model.getWEvlpImportantMtxIndex()[1] == 1,
                    "original rigid-only important-matrix indices use the real draw table");
        }
        // readDraw observes the envelope count present at its actual place in
        // the file, not the value assigned by a later EVP1 block.
        auto source = fixture();
        write16(source, block_offset(source, "DRW1") + 0x26, 0);
        Bytes reordered(source.begin(), source.begin() + 0x20);
        for (const auto name : {"INF1", "JNT1", "DRW1", "EVP1"}) {
            const auto section = block(source, name);
            reordered.insert(reordered.end(), section.begin(), section.end());
        }
        J3dJointData resource(reordered);
        J3DModelData model;
        resource.attach_to(model);
        require(model.getDrawMtxNum() == 4 && model.getWEvlpMtxNum() == 1 &&
                    model.getJointTree().getDrawFullWgtMtxNum() == 2,
                "native readDraw count follows original block order before later envelopes are attached");

        smgpc::resource::J3dNameData absent;
        require(absent.table() == nullptr && absent.resource() == nullptr && absent.bytes().empty(),
                "an absent shared name owner must not synthesize a table");
        const std::array<std::uint8_t, 4> empty_bytes{0, 0, 0xab, 0xcd};
        smgpc::resource::J3dNameData empty(empty_bytes);
        require(empty.resource()->mEntryNum == 0 && empty.resource()->_2 == 0xabcd && empty.table() != nullptr &&
                    empty.table()->getName(0) == nullptr && empty.table()->getIndex("missing") == -1,
                "a present zero-entry table retains its header and original guarded lookup behavior");
        auto short_names = fixture();
        write16(short_names, block_offset(short_names, "JNT1") + 0xa0, 1);
        J3dJointData short_resource(short_names);
        J3DModelData short_model;
        short_resource.attach_to(short_model);
        require(short_model.getJointNum() == 2 && short_model.getJointName()->mNameNum == 1 &&
                    std::strcmp(short_model.getJointName()->getName(0), "Root") == 0 &&
                    short_model.getJointName()->getName(1) == nullptr,
                "stored name count remains independent of joint cardinality as in the original loader");
        const auto source_joint = block(source, "JNT1");
        smgpc::resource::J3dNameData names(source_joint.subspan(read32(source_joint, 0x14)));
        smgpc::resource::J3dNativeBlock<J3DShapeBlock>::Builder builder;
        const auto name_offset = builder.append_bytes(names.bytes(), alignof(ResNTAB));
        builder.header.mpNameTable = decltype(builder)::pointer_offset(name_offset);
        auto native = std::move(builder).finish();
        names = smgpc::resource::J3dNameData();
        const auto* name_resource = JSUConvertOffsetToPtr<ResNTAB>(&native->header(),
                                      reinterpret_cast<std::uintptr_t>(native->header().mpNameTable));
        JUTNameTab table(name_resource);
        require(table.getIndex("Root") == 0 && table.getIndex("Child") == 1 &&
                    std::strcmp(table.getName(1), "Child") == 0 && table.mResource->_2 == 0xabcd,
                "shared converted records retain relative strings when moved into a native SDK block arena");
    }

    void test_bad_ranges() {
        const auto reject = [](auto mutate, std::string_view label) {
            auto bytes = fixture();
            mutate(bytes);
            bool rejected = false;
            try { J3dJointData resource(bytes); } catch (const std::runtime_error&) { rejected = true; }
            require(rejected, label);
        };
        reject([](Bytes& b) { write32(b, 8, static_cast<std::uint32_t>(b.size() - 1)); }, "declared file bounds must enclose every block");
        reject([](Bytes& b) { write32(b, 0xc, 3); }, "required construction blocks cannot be absent");
        reject([](Bytes& b) { tag(b, block_offset(b, "DRW1"), "EVP1"); }, "duplicate retained block identities must be rejected");
        reject([](Bytes& b) { write32(b, block_offset(b, "JNT1") + 0xc, 0xb0); }, "JNT records cannot read through the next section");
        reject([](Bytes& b) { write16(b, block_offset(b, "JNT1") + 0x18, 0xffff); }, "remapped joint records must be readable");
        reject([](Bytes& b) { write32(b, block_offset(b, "JNT1") + 0x10, 0); }, "nonempty joint arrays require a remap table");
        reject([](Bytes& b) { write16(b, block_offset(b, "INF1") + 8, 7); }, "undefined original scale modes cannot produce a calculator");
        reject([](Bytes& b) { write16(b, block_offset(b, "INF1") + 0x18, 0x77); }, "unknown INF commands cannot enter original makeHierarchy");
        reject([](Bytes& b) { write16(b, block_offset(b, "INF1") + 0x1a, 2); }, "hierarchy joints must lie in the actual joint table");
        reject([](Bytes& b) { write16(b, block_offset(b, "INF1") + 0x34, 1); }, "unterminated child scopes must be rejected");
        reject([](Bytes& b) { b[block_offset(b, "EVP1") + 0x1c] = 0; }, "original envelope loops require a positive influence count");
        reject([](Bytes& b) { write16(b, block_offset(b, "EVP1") + 0x20, 2); }, "envelope joints must lie in JNT1");
        reject([](Bytes& b) { write32(b, block_offset(b, "EVP1") + 0x18, 0x80); }, "referenced inverse matrices cannot be silently truncated");
        reject([](Bytes& b) { write16(b, block_offset(b, "DRW1") + 8, 0); }, "retail draw-count subtraction must not underflow");
        reject([](Bytes& b) { b[block_offset(b, "DRW1") + 0x1a] = 0; }, "important indices must fit their original allocation");
        reject([](Bytes& b) { write16(b, block_offset(b, "DRW1") + 0x24, 1); }, "weighted draw references must lie in EVP1");
        reject([](Bytes& b) { write16(b, block_offset(b, "JNT1") + 0xaa, 0xffff); }, "name offsets must stay inside their source block");
    }

    void check_real_data(J3DModelData& model, View bytes) {
        const auto info = block(bytes, "INF1");
        const auto joint = block(bytes, "JNT1");
        const auto env = block(bytes, "EVP1");
        const auto draw = block(bytes, "DRW1");
        auto& tree = model.mJointTree;
        require(model.getFlag() == read16(info, 8) && model.getJointNum() == read16(joint, 8) &&
                    model.getWEvlpMtxNum() == read16(env, 8) && model.getDrawMtxNum() == read16(draw, 8) - read16(env, 8),
                "actual retail counts and flags must match original loader semantics");
        const auto init = read32(joint, 0xc), remap = read32(joint, 0x10), names = read32(joint, 0x14);
        for (std::size_t i = 0; i < model.getJointNum(); ++i) {
            const auto offset = init + read16(joint, remap + i * 2) * 0x40U;
            const auto* actual = model.getJointNodePointer(static_cast<u16>(i));
            require(actual->mJntNo == i && actual->mKind == static_cast<u8>(read16(joint, offset)) &&
                        actual->mScaleCompensate == (joint[offset + 2] == 0xff ? 0 : joint[offset + 2]),
                    "each real joint preserves factory numbering, kind and compensation");
            for (const auto pair : {std::pair{&actual->mTransformInfo.mScale, 4U},
                                    std::pair{&actual->mTransformInfo.mTranslate, 0x18U},
                                    std::pair{&actual->mMin, 0x28U}, std::pair{&actual->mMax, 0x34U}}) {
                same_float(pair.first->x, joint, offset + pair.second);
                same_float(pair.first->y, joint, offset + pair.second + 4);
                same_float(pair.first->z, joint, offset + pair.second + 8);
            }
            same_float(actual->mBoundingSphereRadius, joint, offset + 0x24);
            require(static_cast<u16>(actual->mTransformInfo.mRotation.x) == read16(joint, offset + 0x10) &&
                        static_cast<u16>(actual->mTransformInfo.mRotation.y) == read16(joint, offset + 0x12) &&
                        static_cast<u16>(actual->mTransformInfo.mRotation.z) == read16(joint, offset + 0x14),
                    "real signed rotation values preserve the exact stored bits");
            const auto name = reinterpret_cast<const char*>(joint.data() + names + read16(joint, names + 6 + i * 4));
            require(std::strcmp(tree.mJointName->getName(static_cast<u16>(i)), name) == 0 &&
                        tree.mJointName->getIndex(name) >= 0,
                    "actual name records work through original JUTNameTab lookup");
        }
        std::size_t mix = 0;
        for (std::size_t i = 0; i < tree.mWEvlpMtxNum; ++i) {
            require(tree.mWEvlpMixMtxNum[i] == env[read32(env, 0xc) + i], "real envelope mix counts remain bytes");
            for (std::size_t j = 0; j < tree.mWEvlpMixMtxNum[i]; ++j, ++mix) {
                require(tree.mWEvlpMixMtxIndex[mix] == read16(env, read32(env, 0x10) + mix * 2), "real envelope joint indices are native endian");
                same_float(tree.mWEvlpMixWeight[mix], env, read32(env, 0x14) + mix * 4);
            }
        }
        for (std::size_t i = 0; i < model.getJointNum(); ++i) {
            for (std::size_t r = 0; r < 3; ++r) {
                for (std::size_t c = 0; c < 4; ++c) {
                    same_float(tree.mInvJointMtx[i][r][c], env, read32(env, 0x18) + i * 48 + r * 16 + c * 4);
                }
            }
        }
        for (std::size_t i = 0; i < read16(draw, 8); ++i) {
            require(tree.mDrawMtxData.mDrawMtxFlag[i] == draw[read32(draw, 0xc) + i] &&
                        tree.mDrawMtxData.mDrawMtxIndex[i] == read16(draw, read32(draw, 0x10) + i * 2),
                    "all serialized real draw flags and indices remain available");
        }
        tree.findImportantMtxIndex();
        mix = 0;
        for (std::size_t i = 0; i < tree.mWEvlpMtxNum; ++i) {
            const auto count = tree.mWEvlpMixMtxNum[i];
            const auto best = std::max_element(tree.mWEvlpMixWeight + mix, tree.mWEvlpMixWeight + mix + count);
            require(*best > -0.1F && tree.mWEvlpImportantMtxIdx[tree.getDrawFullWgtMtxNum() + i] ==
                        tree.mWEvlpMixMtxIndex[best - tree.mWEvlpMixWeight],
                    "actual important-matrix finalization picks the authored greatest influence");
            mix += count;
        }
    }

    void test_optional_real_disc() {
        const auto* path = std::getenv("SMGPC_REAL_DISC");
        if (path == nullptr || path[0] == '\0') {
            std::cout << "[skip] original Mario joint resources (set SMGPC_REAL_DISC)\n";
            return;
        }
        aurora_dvd_close();
        require(aurora_dvd_open(path), "SMGPC_REAL_DISC must be a readable original disc");
        struct DiscOwner { ~DiscOwner() { aurora_dvd_close(); } } disc;
        DVDInit();
        std::unique_ptr<J3dJointData> resource;
        J3DModelData model;
        std::string first_name;
        float retained_inverse = 0;
        {
            smgpc::runtime::DvdFileSystemService dvd{"/"};
            const auto archive_path = dvd.find_object_archive("Mario");
            require(archive_path.has_value(), "the actual Mario model archive must exist");
            const auto& archive = dvd.archive_for_path(*archive_path);
            const auto* entry = archive.find_by_basename("mario.bdl");
            require(entry != nullptr, "the actual Mario model must be present");
            const auto bytes = archive.file_data(*entry);
            resource = std::make_unique<J3dJointData>(bytes);
            resource->attach_to(model);
            check_real_data(model, bytes);
            require(model.getJointNum() == 30 && model.getWEvlpMtxNum() == 13 && model.getDrawMtxNum() == 35 &&
                        model.getJointTree().getDrawFullWgtMtxNum() == 22,
                    "RMGK01 Mario uses the independently extracted 30/13/35/22 joint/envelope/draw/full counts");
            first_name = model.getJointName()->getName(0);
            retained_inverse = model.getInvJointMtx(29)[2][3];
            std::cout << "[resource] mario.bdl: joints=30, envelopes=13, serialized draw=48, native draw=35, full=22\n";
        }
        require(first_name == model.getJointName()->getName(0) && model.getInvJointMtx(29)[2][3] == retained_inverse &&
                    model.getJointNodePointer(29)->mJntNo == 29,
                "real typed data survives retirement of its source archive and DVD service");
        require(model.getJointTree().getRootNode() == nullptr && model.getMaterialNum() == 0 && model.getShapeNum() == 0,
                "this component fixture does not claim complete Mario model decoding or hierarchy finalization");
    }

}  // namespace

int main() {
    try {
        test_decoded_ownership();
        test_modes_and_absence();
        test_bad_ranges();
        test_optional_real_disc();
        std::cout << "[pass] 4 original J3D joint-resource groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] original J3D joint resource: " << error.what() << '\n';
        return 1;
    }
}
