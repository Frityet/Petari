#include "compat/MetrowerksStdCompat.hpp"
#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    void near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.0001F) {
            throw std::runtime_error(std::string(message) + ": " + std::to_string(actual));
        }
    }

    struct OneName {
        struct Resource {
            ResNTAB table{};
            char text[24]{};
        } data;
        ResNTAB& resource = data.table;
        JUTNameTab names;

        explicit OneName(const char* value) {
            std::strcpy(data.text, value);
            resource.mEntryNum = 1;
            resource.mEntries[0].mKeyCode = names.calcKeyCode(value);
            resource.mEntries[0].mOffs = offsetof(Resource, text);
            names.setResource(&resource);
        }
    };

    void test_actual_name_search_and_missing_id() {
        OneName body("body"), absent("absent");
        J3DModelData model;
        model.mMaterialTable.mMaterialNum = 1;
        model.mMaterialTable.mMaterialName = &body.names;
        require(body.names.getIndex("body") == 0 && body.names.getIndex("absent") == -1,
                "actual name table uses its retained key and string data");

        u16 colorID = 99, patternID = 99, textureID = 99, postID = 99, cID = 99, kID = 99;
        J3DAnmColorKey color;
        color.mUpdateMaterialNum = 1;
        color.mUpdateMaterialID = &colorID;
        color.mUpdateMaterialName.setResource(&body.resource);
        color.searchUpdateMaterialID(&model);
        require(colorID == 0, "recovered model forwarding finds the material color target");

        J3DAnmTexPattern pattern;
        pattern.mUpdateMaterialNum = 1;
        pattern.mUpdateMaterialID = &patternID;
        pattern.mUpdateMaterialName.setResource(&absent.resource);
        pattern.searchUpdateMaterialID(&model);
        require(patternID == 0xFFFF, "missing texture-pattern target becomes the retail sentinel");

        J3DAnmTextureSRTKey texture;
        texture.mTrackNum = 3;
        texture.field_0x4a = 3;
        texture.mUpdateMaterialID = &textureID;
        texture.mPostUpdateMaterialID = &postID;
        texture.mUpdateMaterialName.setResource(&body.resource);
        texture.mPostUpdateMaterialName.setResource(&absent.resource);
        texture.searchUpdateMaterialID(&model);
        require(textureID == 0 && postID == 0xFFFF, "SRT resolves ordinary and post target names independently");

        J3DAnmTevRegKey tev;
        tev.mCRegUpdateMaterialNum = tev.mKRegUpdateMaterialNum = 1;
        tev.mCRegUpdateMaterialID = &cID;
        tev.mKRegUpdateMaterialID = &kID;
        tev.mCRegUpdateMaterialName.setResource(&body.resource);
        tev.mKRegUpdateMaterialName.setResource(&absent.resource);
        tev.searchUpdateMaterialID(&model);
        require(cID == 0 && kID == 0xFFFF, "TEV ordinary and konst names use independent target lists");
    }

    void test_original_difference_masks() {
        std::array<u32, 3> flags{0x10, 0x20, 0x40};
        std::array<u16, 3> ids{0, 0xFFFF, 2};
        J3DAnmColorKey color;
        color.mUpdateMaterialNum = ids.size();
        color.mUpdateMaterialID = ids.data();
        MR::onDiffFlagBpk(flags.data(), &color, "sample");
        require(flags == std::array<u32, 3>{0x11, 0x20, 0x41}, "BPK sets only MatColor and skips invalid IDs");
        MR::offDiffFlagBpk(flags.data(), &color, "sample");

        J3DAnmTexPattern pattern;
        pattern.mUpdateMaterialNum = ids.size();
        pattern.mUpdateMaterialID = ids.data();
        MR::onDiffFlagBtp(flags.data(), &pattern, "sample");
        require(flags == std::array<u32, 3>{0x20010, 0x20, 0x20040}, "BTP uses original mask0x20000");
        MR::offDiffFlagBtp(flags.data(), &pattern, "sample");

        J3DAnmTextureSRTKey texture;
        texture.mTrackNum = ids.size() * 3;
        texture.mUpdateMaterialID = ids.data();
        MR::onDiffFlagBtk(flags.data(), &texture, "sample");
        require(flags == std::array<u32, 3>{0x210, 0x20, 0x240}, "BTK uses track count divided by3 and original mask0x200");
        MR::offDiffFlagBtk(flags.data(), &texture, "sample");

        J3DAnmTevRegKey tev;
        u16 cID = 1;
        tev.mCRegUpdateMaterialNum = 1;
        tev.mCRegUpdateMaterialID = &cID;
        tev.mKRegUpdateMaterialNum = ids.size();
        tev.mKRegUpdateMaterialID = ids.data();
        MR::onDiffFlagBrk(flags.data(), &tev, "sample");
        require(flags == std::array<u32, 3>{0x1000010, 0x1000020, 0x1000040},
                "both BRK lists use the same original TEV-register mask");
        MR::offDiffFlagBrk(flags.data(), &tev, "sample");
        require(flags == std::array<u32, 3>{0x10, 0x20, 0x40}, "clearing animation bits preserves unrelated flags");
    }

    struct ColorKeys {
        std::array<s16, 6> red{0, -200, 0, 2, -100, 0};
        std::array<s16, 6> green{0, 400, 0, 2, 600, 0};
        std::array<s16, 6> alpha{0, 10, 0, 2, 13, 0};
        J3DAnmColorKeyTable table{};
        J3DAnmColorKey animation;

        ColorKeys() {
            table.mRInfo = {2, 0, 0};
            table.mGInfo = {2, 0, 0};
            table.mAInfo = {2, 0, 0};
            animation.mAnmTable = &table;
            animation.mColorR = red.data();
            animation.mColorG = green.data();
            animation.mColorA = alpha.data();
            animation.setFrame(1);
        }
    };

    void test_original_color_and_tev_sampling() {
        ColorKeys color;
        GXColor sampled{9, 9, 9, 9};
        color.animation.getColor(0, &sampled);
        require(sampled.r == 0 && sampled.g == 255 && sampled.b == 0 && sampled.a == 11,
                "BPK interpolates, clamps, defaults absent channels, and truncates11.5");
        color.table.mRInfo = {1, 1, 0};
        color.red[1] = -1;
        color.animation.getColor(0, &sampled);
        require(sampled.r == 255, "single-key color stores the original low byte without interpolated clamp");

        std::array<s16, 6> low{0, -2000, 0, 2, -1800, 0};
        std::array<s16, 6> high{0, 2000, 0, 2, 1800, 0};
        std::array<s16, 6> fractional{0, -3, 0, 2, -2, 0};
        J3DAnmCRegKeyTable cTable{};
        cTable.mRTable = cTable.mGTable = cTable.mBTable = {2, 0, 0};
        J3DAnmKRegKeyTable kTable{};
        kTable.mRTable = kTable.mGTable = kTable.mATable = {2, 0, 0};
        J3DAnmTevRegKey tev;
        tev.mAnmCRegKeyTable = &cTable;
        tev.mAnmCRegDataR = low.data();
        tev.mAnmCRegDataG = high.data();
        tev.mAnmCRegDataB = fractional.data();
        tev.mAnmKRegKeyTable = &kTable;
        tev.mAnmKRegDataR = low.data();
        tev.mAnmKRegDataG = high.data();
        tev.mAnmKRegDataA = color.alpha.data();
        tev.setFrame(1);
        GXColorS10 signedColor{};
        tev.getTevColorReg(0, &signedColor);
        require(signedColor.r == -1024 && signedColor.g == 1023 && signedColor.b == -2 && signedColor.a == 0,
                "BRK signed register clamps to10-bit range and truncates negative fraction towardzero");
        tev.getTevKonstReg(0, &sampled);
        require(sampled.r == 0 && sampled.g == 255 && sampled.b == 0 && sampled.a == 11,
                "BRK konst register uses its independent byte channels");
    }

    void test_original_srt_and_full_frame_rules() {
        std::array<J3DAnmTransformKeyTable, 3> tracks{};
        std::array<f32, 2> scale{2, 3};
        std::array<s16, 1> rotation{-1234};
        std::array<f32, 6> translation{0, 10, 2, 2, 14, 2};
        tracks[0].mScaleInfo = {1, 0, 0};
        tracks[1].mScaleInfo = {1, 1, 0};
        tracks[2].mRotationInfo = {1, 0, 0};
        tracks[0].mTranslateInfo = {2, 0, 0};
        J3DAnmTextureSRTKey texture;
        texture.mAnmTable = tracks.data();
        texture.mScaleData = scale.data();
        texture.mRotData = rotation.data();
        texture.mTransData = translation.data();
        texture.mDecShift = 1;
        texture.setFrame(1);
        J3DTextureSRTInfo srt{};
        texture.getTransform(0, &srt);
        require(srt.mScaleX == 2 && srt.mScaleY == 3 && srt.mRotation == -2468,
                "BTK uses original separate X/Y scale and signed rotation shift");
        near(srt.mTranslationX, 12, "BTK linear Hermite midpoint");
        require(srt.mTranslationY == 0, "BTK absent translation defaults tozero");
        texture.mDecShift = 32;
        texture.getTransform(0, &srt);
        require(srt.mRotation == 0, "BTK shares the original PPC shift-width rule");

        J3DAnmTexPatternFullTable patternTable{};
        patternTable.mMaxFrame = 3;
        std::array<u16, 3> textureIDs{7, 11, 13};
        J3DAnmTexPattern pattern;
        pattern.mAnmTable = &patternTable;
        pattern.mTextureIndex = textureIDs.data();
        for (auto [frame, expected] : std::array<std::pair<float, u16>, 4>{{{-1, 7}, {0.75F, 7}, {1.75F, 11}, {3, 13}}}) {
            pattern.setFrame(frame);
            u16 actual = 99;
            pattern.getTexNo(0, &actual);
            require(actual == expected, "BTP truncates intermediate frames and clamps endpoints");
        }

        std::array<u8, 3> colors{7, 11, 13};
        J3DAnmColorFullTable fullTable{};
        fullTable.mRMaxFrame = fullTable.mGMaxFrame = fullTable.mBMaxFrame = fullTable.mAMaxFrame = 3;
        J3DAnmColorFull full;
        full.mAnmTable = &fullTable;
        full.mColorR = full.mColorG = full.mColorB = full.mColorA = colors.data();
        full.setFrame(0.5F);
        GXColor result{};
        full.getColor(0, &result);
        require(result.r == 11 && result.a == 11, "full color animation rounds half-frame upward unlike BTP");
    }

    void test_actual_material_animation_owner() {
        J3DMaterialAnm animation;
        animation.calc(nullptr);  // Constructor disables every channel before any material lookup.
        ColorKeys colors;
        J3DMaterial material;
        material.initialize();
        std::unique_ptr<J3DColorBlock> colorBlock(J3DMaterial::createColorBlock(0));
        std::unique_ptr<J3DTexGenBlock> textureBlock(J3DMaterial::createTexGenBlock(0));
        std::unique_ptr<J3DTevBlock> tevBlock(J3DMaterial::createTevBlock(4));
        material.mColorBlock = colorBlock.get();
        material.mTexGenBlock = textureBlock.get();
        material.mTevBlock = tevBlock.get();
        J3DMatColorAnm colorBinding(0, &colors.animation);
        animation.setMatColorAnm(0, &colorBinding);
        colorBinding.setAnmFlag(false);
        animation.calc(&material);
        GXColor* actual = colorBlock->getMatColor(0);
        require(actual->r == 0 && actual->g == 255 && actual->a == 11,
                "actual material animation owns a copied binding and writes the real color block");
        animation.setMatColorAnm(0, nullptr);
        actual->r = 123;
        animation.calc(&material);
        require(actual->r == 123, "null binding disables animation without resetting material data");

        J3DAnmTexPatternFullTable table{};
        table.mMaxFrame = 1;
        u16 textureID = 17;
        J3DAnmTexPattern pattern;
        pattern.mAnmTable = &table;
        pattern.mTextureIndex = &textureID;
        J3DTexNoAnm binding(0, &pattern);
        animation.setTexNoAnm(2, &binding);
        animation.calc(&material);
        require(tevBlock->getTexNo(2) == 17, "real virtual texture-number binding updates the authored slot");
        animation.initialize();
        textureID = 19;
        animation.calc(&material);
        require(tevBlock->getTexNo(2) == 17, "initialize disables retained bindings without rewriting model values");
    }
}  // namespace

int main() {
    try {
        test_actual_name_search_and_missing_id();
        test_original_difference_masks();
        test_original_color_and_tev_sampling();
        test_original_srt_and_full_frame_rules();
        test_actual_material_animation_owner();
        std::cout << "5/5 original material-animation groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
