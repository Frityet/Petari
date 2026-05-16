#include "assets/layout/Bmg.hpp"
#include "assets/layout/Brlyt.hpp"
#include "game/Game/Screen/LayoutActor.hpp"
#include "game/Game/Util/LayoutUtil.hpp"
#include "game/compat/RuntimeContext.hpp"
#include "game/layout/LayoutArchiveLoader.hpp"
#include "game/layout/LayoutRuntimeActor.hpp"
#include "game/nw4r/lyt/texMap.h"
#include "render/layout/LayoutDrawList.hpp"
#include "tests/TestHarness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    [[nodiscard]] smgpc::assets::layout::PaneDefinition make_picture_root_pane() {
        smgpc::assets::layout::PaneDefinition pane{};
        pane.type = smgpc::assets::layout::PaneType::Picture;
        pane.name = "RootPicture";
        pane.visible = true;
        pane.location_adjust = false;
        pane.alpha = 255U;
        pane.scale = {.x = 1.0F, .y = 1.0F};
        pane.size = {.x = 64.0F, .y = 64.0F};
        pane.material_index = 0;
        pane.base_position = 0U;
        pane.tex_coords = {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F};
        pane.vertex_colors = std::array< smgpc::assets::layout::Color, 4 >{
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
        };
        return pane;
    }

    [[nodiscard]] smgpc::assets::layout::tpl::DecodedImage make_solid_texture(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
        smgpc::assets::layout::tpl::DecodedImage image{};
        image.width = 2U;
        image.height = 2U;
        image.rgba8.resize(2U * 2U * 4U);
        for (std::size_t pixel = 0; pixel < 4U; ++pixel) {
            const std::size_t base = pixel * 4U;
            image.rgba8[base + 0U] = r;
            image.rgba8[base + 1U] = g;
            image.rgba8[base + 2U] = b;
            image.rgba8[base + 3U] = a;
        }
        return image;
    }

    [[nodiscard]] smgpc::assets::layout::tpl::DecodedImage make_sized_solid_texture(
        std::uint16_t width,
        std::uint16_t height,
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b,
        std::uint8_t a) {
        smgpc::assets::layout::tpl::DecodedImage image{};
        image.width = width;
        image.height = height;
        image.rgba8.resize(static_cast< std::size_t >(width) * static_cast< std::size_t >(height) * 4U);
        for (std::size_t pixel = 0U; pixel < image.rgba8.size() / 4U; ++pixel) {
            const std::size_t base = pixel * 4U;
            image.rgba8[base + 0U] = r;
            image.rgba8[base + 1U] = g;
            image.rgba8[base + 2U] = b;
            image.rgba8[base + 3U] = a;
        }
        return image;
    }

    [[nodiscard]] smgpc::assets::layout::tpl::DecodedImage make_alpha_mask_texture() {
        smgpc::assets::layout::tpl::DecodedImage image{};
        image.width = 2U;
        image.height = 2U;
        image.rgba8 = {
            255U, 255U, 255U, 0U, 255U, 255U, 255U, 255U, 255U, 255U, 255U, 0U, 255U, 255U, 255U, 255U,
        };
        return image;
    }

    struct TestTimgStorage {
        ResTIMG header {};
        std::array<std::byte, 32> image {};
    };

    void fill_rgb565_test_timg(TestTimgStorage *storage, std::uint16_t color) {
        storage->header.mFormat = GX_TF_RGB565;
        storage->header.mWidth = 4U;
        storage->header.mHeight = 4U;
        storage->header.mWrapS = GX_CLAMP;
        storage->header.mWrapT = GX_CLAMP;
        storage->header.mImageDataOffset = static_cast<u32>(offsetof(TestTimgStorage, image));

        for (std::size_t i = 0U; i < storage->image.size(); i += 2U) {
            storage->image[i + 0U] = static_cast<std::byte>((color >> 8U) & 0xFFU);
            storage->image[i + 1U] = static_cast<std::byte>(color & 0xFFU);
        }
    }

    void append_u8(std::vector< std::byte >* bytes, std::uint8_t value) {
        bytes->push_back(static_cast< std::byte >(value));
    }

    void append_u16_be(std::vector< std::byte >* bytes, std::uint16_t value) {
        append_u8(bytes, static_cast< std::uint8_t >((value >> 8U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >(value & 0xFFU));
    }

    void append_u32_be(std::vector< std::byte >* bytes, std::uint32_t value) {
        append_u8(bytes, static_cast< std::uint8_t >((value >> 24U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >((value >> 16U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >((value >> 8U) & 0xFFU));
        append_u8(bytes, static_cast< std::uint8_t >(value & 0xFFU));
    }

    void append_ascii(std::vector< std::byte >* bytes, std::string_view text) {
        for (const char ch : text) {
            append_u8(bytes, static_cast< std::uint8_t >(ch));
        }
    }

    void patch_u32_be(std::vector< std::byte >* bytes, std::size_t offset, std::uint32_t value) {
        (*bytes)[offset + 0U] = static_cast< std::byte >((value >> 24U) & 0xFFU);
        (*bytes)[offset + 1U] = static_cast< std::byte >((value >> 16U) & 0xFFU);
        (*bytes)[offset + 2U] = static_cast< std::byte >((value >> 8U) & 0xFFU);
        (*bytes)[offset + 3U] = static_cast< std::byte >(value & 0xFFU);
    }

    [[nodiscard]] std::vector< std::byte > make_test_brfnt() {
        std::vector< std::byte > bytes{};
        bytes.reserve(256U);

        append_ascii(&bytes, "RFNT");
        append_u16_be(&bytes, 0xFEFFU);
        append_u16_be(&bytes, 0x0104U);
        const auto file_size_offset = bytes.size();
        append_u32_be(&bytes, 0U);
        append_u16_be(&bytes, 0x0010U);
        append_u16_be(&bytes, 4U);

        append_ascii(&bytes, "FINF");
        append_u32_be(&bytes, 0x20U);
        append_u8(&bytes, 1U);
        append_u8(&bytes, 8U);
        append_u16_be(&bytes, 0U);
        append_u8(&bytes, 0U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 1U);
        const auto tglp_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        const auto cwdh_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        const auto cmap_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 7U);
        append_u8(&bytes, 0U);

        append_ascii(&bytes, "TGLP");
        append_u32_be(&bytes, 0xA4U);
        const auto tglp_payload_offset = bytes.size();
        append_u8(&bytes, 8U);
        append_u8(&bytes, 8U);
        append_u8(&bytes, 7U);
        append_u8(&bytes, 8U);
        append_u32_be(&bytes, 128U);
        append_u16_be(&bytes, 1U);
        append_u16_be(&bytes, 0U);
        append_u16_be(&bytes, 2U);
        append_u16_be(&bytes, 1U);
        append_u16_be(&bytes, 32U);
        append_u16_be(&bytes, 8U);
        append_u32_be(&bytes, static_cast< std::uint32_t >(tglp_payload_offset + 28U));
        append_u32_be(&bytes, 0U);
        bytes.insert(bytes.end(), 128U, static_cast< std::byte >(0xFFU));

        append_ascii(&bytes, "CWDH");
        append_u32_be(&bytes, 0x18U);
        const auto cwdh_payload_offset = bytes.size();
        append_u16_be(&bytes, 0U);
        append_u16_be(&bytes, 1U);
        append_u32_be(&bytes, 0U);
        append_u8(&bytes, 0U);
        append_u8(&bytes, 0U);
        append_u8(&bytes, 4U);
        append_u8(&bytes, 1U);
        append_u8(&bytes, 5U);
        append_u8(&bytes, 7U);
	        append_u16_be(&bytes, 0U);

	        append_ascii(&bytes, "CMAP");
	        append_u32_be(&bytes, 0x30U);
	        const auto cmap_payload_offset = bytes.size();
	        append_u16_be(&bytes, 0x20U);
	        append_u16_be(&bytes, 0x69U);
	        append_u16_be(&bytes, 2U);
	        append_u16_be(&bytes, 0U);
	        append_u32_be(&bytes, 0U);
	        append_u16_be(&bytes, 6U);
	        append_u16_be(&bytes, 0x20U);
	        append_u16_be(&bytes, 0U);
	        append_u16_be(&bytes, 0x30U);
	        append_u16_be(&bytes, 1U);
	        append_u16_be(&bytes, 0x31U);
	        append_u16_be(&bytes, 1U);
	        append_u16_be(&bytes, 0x41U);
	        append_u16_be(&bytes, 1U);
	        append_u16_be(&bytes, 0x42U);
	        append_u16_be(&bytes, 1U);
	        append_u16_be(&bytes, 0x69U);
	        append_u16_be(&bytes, 1U);
	        append_u16_be(&bytes, 0U);

        patch_u32_be(&bytes, file_size_offset, static_cast< std::uint32_t >(bytes.size()));
        patch_u32_be(&bytes, tglp_offset_patch, static_cast< std::uint32_t >(tglp_payload_offset));
        patch_u32_be(&bytes, cwdh_offset_patch, static_cast< std::uint32_t >(cwdh_payload_offset));
        patch_u32_be(&bytes, cmap_offset_patch, static_cast< std::uint32_t >(cmap_payload_offset));

        return bytes;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_text_resource(std::u16string text, float char_space = 0.0F,
                                                                                               std::uint8_t text_position = 0U) {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.font_names.push_back("TestFont.brfnt");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "TextMaterial";
        resource->layout.materials.push_back(std::move(material));

        smgpc::assets::layout::PaneDefinition pane{};
        pane.type = smgpc::assets::layout::PaneType::Text;
        pane.name = "Text";
        pane.visible = true;
        pane.location_adjust = false;
        pane.alpha = 255U;
        pane.scale = {.x = 1.0F, .y = 1.0F};
        pane.size = {.x = 64.0F, .y = 16.0F};
        pane.material_index = 0;
        pane.font_index = 0;
        pane.text_position = text_position;
        pane.text_alignment = 0U;
        pane.text = std::move(text);
        pane.text_font_size = {.x = 8.0F, .y = 8.0F};
        pane.text_char_space = char_space;
        pane.text_colors[0] = {.r = 255U, .g = 0U, .b = 0U, .a = 255U};
        pane.text_colors[1] = {.r = 0U, .g = 0U, .b = 255U, .a = 128U};
        resource->layout.panes.push_back(std::move(pane));

        const auto parsed_font = smgpc::assets::layout::parse_brfnt(make_test_brfnt(), "TestFont");
        if (not parsed_font) {
            throw std::runtime_error(parsed_font.failure().message);
        }
        resource->fonts_by_name.emplace("testfont", *parsed_font);

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_parented_text_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.font_names.push_back("TestFont.brfnt");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "TextMaterial";
        resource->layout.materials.push_back(std::move(material));

        smgpc::assets::layout::PaneDefinition parent{};
        parent.type = smgpc::assets::layout::PaneType::Pane;
        parent.name = "FileNumber";
        parent.visible = true;
        parent.location_adjust = false;
        parent.alpha = 255U;
        parent.scale = {.x = 1.0F, .y = 1.0F};
        parent.size = {.x = 64.0F, .y = 16.0F};
        parent.children.push_back(1);
        resource->layout.panes.push_back(std::move(parent));

        smgpc::assets::layout::PaneDefinition text{};
        text.type = smgpc::assets::layout::PaneType::Text;
        text.name = "TxtFileNumber";
        text.parent = 0;
        text.visible = true;
        text.location_adjust = false;
        text.alpha = 255U;
        text.scale = {.x = 1.0F, .y = 1.0F};
        text.size = {.x = 64.0F, .y = 16.0F};
        text.material_index = 0;
        text.font_index = 0;
        text.text_position = 0U;
        text.text = u"A";
        text.text_font_size = {.x = 8.0F, .y = 8.0F};
        text.text_colors[0] = {.r = 255U, .g = 255U, .b = 255U, .a = 255U};
        text.text_colors[1] = {.r = 255U, .g = 255U, .b = 255U, .a = 255U};
        resource->layout.panes.push_back(std::move(text));

        const auto parsed_font = smgpc::assets::layout::parse_brfnt(make_test_brfnt(), "TestFont");
        if (not parsed_font) {
            throw std::runtime_error(parsed_font.failure().message);
        }
        resource->fonts_by_name.emplace("testfont", *parsed_font);

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_single_texture_resource(
        std::array< std::uint8_t, 4 > texture_color = {255U, 255U, 255U, 255U},
        std::array< std::uint8_t, 4 > font_color = {255U, 255U, 255U, 255U},
        std::int32_t tev_stage_count = 0) {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.panes.push_back(make_picture_root_pane());
        resource->layout.texture_names.push_back("Color.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "SingleTexture";
        material.texture_index = 0;
        material.texture_indices.push_back(0);
        material.texture_color = texture_color;
        material.font_color = font_color;
        material.tev_stage_count = tev_stage_count;
        resource->layout.materials.push_back(std::move(material));

        resource->textures_by_name.emplace("color", make_solid_texture(64U, 128U, 192U, 255U));
        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_texture_pattern_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.panes.push_back(make_picture_root_pane());
        resource->layout.panes[0U].name = "PicPicture";
        resource->layout.texture_names.push_back("MyP2Manual1.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "PicPicture";
        material.texture_index = 0;
        material.texture_indices.push_back(0);
        resource->layout.materials.push_back(std::move(material));

        resource->textures_by_name.emplace("myp2manual1", make_solid_texture(64U, 64U, 64U, 255U));
        resource->textures_by_name.emplace("myp2manual2", make_solid_texture(128U, 128U, 128U, 255U));
        resource->textures_by_name.emplace("myp2manual3", make_solid_texture(192U, 192U, 192U, 255U));

        smgpc::assets::layout::BrlanAnimation animation{};
        animation.name = "Picture";
        animation.frame_size = 3U;
        animation.loop = false;
        animation.tracks.push_back(smgpc::assets::layout::BrlanTrack{
            .pane_name = "PicPicture",
            .kind = "RLTP",
            .target = 0U,
            .curve_type = smgpc::assets::layout::BrlanCurveType::Step,
            .keys = {
                smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 0.0F, .slope = 0.0F},
                smgpc::assets::layout::BrlanKey{.frame = 1.0F, .value = 1.0F, .slope = 0.0F},
                smgpc::assets::layout::BrlanKey{.frame = 2.0F, .value = 2.0F, .slope = 0.0F},
            },
        });
        resource->animations_by_name.emplace("picture", std::move(animation));

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_named_texture_pattern_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.panes.push_back(make_picture_root_pane());
        resource->layout.panes[0U].name = "PicMario";
        resource->layout.texture_names.push_back("MyMiiMario.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "PicMario";
        material.texture_index = 0;
        material.texture_indices.push_back(0);
        resource->layout.materials.push_back(std::move(material));

        resource->textures_by_name.emplace("mymiimario", make_solid_texture(220U, 32U, 32U, 255U));
        resource->textures_by_name.emplace("mymiiyoshi", make_solid_texture(32U, 220U, 32U, 255U));

        smgpc::assets::layout::BrlanAnimation animation{};
        animation.name = "Character";
        animation.frame_size = 2U;
        animation.loop = false;
        animation.texture_names = {"MyMiiMario.tpl", "MyMiiYoshi.tpl"};
        animation.tracks.push_back(smgpc::assets::layout::BrlanTrack{
            .pane_name = "PicMario",
            .kind = "RLTP",
            .target = 0U,
            .curve_type = smgpc::assets::layout::BrlanCurveType::Step,
            .keys = {
                smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 0.0F, .slope = 0.0F},
                smgpc::assets::layout::BrlanKey{.frame = 1.0F, .value = 1.0F, .slope = 0.0F},
            },
        });
        resource->animations_by_name.emplace("character", std::move(animation));

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_pane_animation_persistence_resource() {
        auto resource = make_single_texture_resource();

        smgpc::assets::layout::BrlanAnimation appear {};
        appear.name = "ButtonAppear";
        appear.frame_size = 10U;
        appear.loop = false;
        appear.tracks.push_back(smgpc::assets::layout::BrlanTrack{
            .pane_name = "RootPicture",
            .kind = "RLVC",
            .target = 0U,
            .curve_type = smgpc::assets::layout::BrlanCurveType::Hermite,
            .keys = {
                smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 255.0F, .slope = 0.0F},
                smgpc::assets::layout::BrlanKey{.frame = 10.0F, .value = 150.0F, .slope = 0.0F},
            },
        });
        resource->animations_by_name.emplace("buttonappear", std::move(appear));

        smgpc::assets::layout::BrlanAnimation wait {};
        wait.name = "ButtonWait";
        wait.frame_size = 30U;
        wait.loop = true;
        wait.tracks.push_back(smgpc::assets::layout::BrlanTrack{
            .pane_name = "RootPicture",
            .kind = "RLPA",
            .target = 6U,
            .curve_type = smgpc::assets::layout::BrlanCurveType::Hermite,
            .keys = {
                smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 2.0F, .slope = 0.0F},
            },
        });
        resource->animations_by_name.emplace("buttonwait", std::move(wait));

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_two_texture_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.panes.push_back(make_picture_root_pane());
        resource->layout.texture_names.push_back("Mask.tpl");
        resource->layout.texture_names.push_back("Color.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "MaskedTexture";
        material.texture_index = 0;
        material.texture_indices = {0, 1};
        resource->layout.materials.push_back(std::move(material));

        resource->textures_by_name.emplace("mask", make_alpha_mask_texture());
        resource->textures_by_name.emplace("color", make_solid_texture(32U, 96U, 160U, 255U));
        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_hand_pointer_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.panes.push_back(make_picture_root_pane());
        resource->layout.texture_names.push_back("HandPointerGradation.tpl");
        resource->layout.texture_names.push_back("HandPointerPoint.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "NarrowLookupHand";
        material.texture_index = 0;
        material.texture_indices = {0, 1};
        material.tev_stage_count = 2;
        resource->layout.materials.push_back(std::move(material));

        resource->textures_by_name.emplace("handpointergradation", make_sized_solid_texture(8U, 32U, 128U, 128U, 128U, 220U));
        auto hand = make_sized_solid_texture(64U, 64U, 255U, 255U, 255U, 255U);
        for (std::uint16_t y = 0U; y < 64U; ++y) {
            for (std::uint16_t x = 0U; x < 64U; ++x) {
                if (x < 8U || y < 8U) {
                    hand.rgba8[(static_cast< std::size_t >(y) * 64U + x) * 4U + 3U] = 0U;
                }
            }
        }
        resource->textures_by_name.emplace("handpointerpoint", std::move(hand));
        return resource;
    }

    [[nodiscard]] smgpc::assets::layout::tpl::DecodedImage make_alpha_hand_texture(
        std::uint8_t r,
        std::uint8_t g,
        std::uint8_t b) {
        auto image = make_sized_solid_texture(64U, 64U, r, g, b, 255U);
        for (std::uint16_t y = 0U; y < 64U; ++y) {
            for (std::uint16_t x = 0U; x < 64U; ++x) {
                if (x < 8U || y < 8U) {
                    image.rgba8[(static_cast< std::size_t >(y) * 64U + x) * 4U + 3U] = 0U;
                }
            }
        }
        return image;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_dpd_hand_type_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.panes.push_back(make_picture_root_pane());
        resource->layout.panes[0U].name = "PicHand";
        resource->layout.texture_names.push_back("HandPointerGradation.tpl");
        resource->layout.texture_names.push_back("HandPointerPoint.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "PicHand";
        material.texture_index = 0;
        material.texture_indices = {0, 1};
        material.tev_stage_count = 2;
        resource->layout.materials.push_back(std::move(material));

        resource->textures_by_name.emplace("handpointergradation", make_sized_solid_texture(8U, 32U, 128U, 128U, 128U, 220U));
        resource->textures_by_name.emplace("handpointergrip", make_alpha_hand_texture(220U, 32U, 32U));
        resource->textures_by_name.emplace("handpointergripshadow", make_alpha_hand_texture(80U, 32U, 32U));
        resource->textures_by_name.emplace("handpointerpoint", make_alpha_hand_texture(32U, 220U, 32U));
        resource->textures_by_name.emplace("handpointerpointshadow", make_alpha_hand_texture(32U, 80U, 32U));
        resource->textures_by_name.emplace("handpointerrelease", make_alpha_hand_texture(32U, 32U, 220U));
        resource->textures_by_name.emplace("handpointerreleaseshadow", make_alpha_hand_texture(32U, 32U, 80U));

        smgpc::assets::layout::BrlanAnimation animation{};
        animation.name = "HandType";
        animation.frame_size = 3U;
        animation.loop = false;
        animation.texture_names = {
            "HandPointerGrip.tpl",
            "HandPointerGripShadow.tpl",
            "HandPointerPoint.tpl",
            "HandPointerPointShadow.tpl",
            "HandPointerRelease.tpl",
            "HandPointerReleaseShadow.tpl",
        };
        animation.tracks.push_back(smgpc::assets::layout::BrlanTrack{
            .pane_name = "PicHand",
            .kind = "RLTP",
            .target = 0x100U,
            .curve_type = smgpc::assets::layout::BrlanCurveType::Step,
            .keys = {
                smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 2.0F, .slope = 0.0F},
                smgpc::assets::layout::BrlanKey{.frame = 1.0F, .value = 0.0F, .slope = 0.0F},
                smgpc::assets::layout::BrlanKey{.frame = 2.0F, .value = 4.0F, .slope = 0.0F},
            },
        });
        resource->animations_by_name.emplace("handtype", std::move(animation));

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_localized_visibility_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.texture_names.push_back("Plate.tpl");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "Plate";
        material.texture_index = 0;
        material.texture_indices.push_back(0);
        resource->layout.materials.push_back(std::move(material));

        smgpc::assets::layout::PaneDefinition root{};
        root.type = smgpc::assets::layout::PaneType::Pane;
        root.name = "Plate";
        root.visible = true;
        root.location_adjust = false;
        root.alpha = 255U;
        root.scale = {.x = 1.0F, .y = 1.0F};
        root.size = {.x = 64.0F, .y = 64.0F};
        root.children = {1, 2, 3};
        resource->layout.panes.push_back(std::move(root));

        for (const auto& [name, visible] : {std::pair{"PicPlate", false}, std::pair{"PicPlateKrKo", true}, std::pair{"PicPlateCnSi", false}}) {
            auto pane = make_picture_root_pane();
            pane.name = name;
            pane.parent = 0;
            pane.visible = visible;
            pane.size = {.x = 8.0F, .y = 8.0F};
            resource->layout.panes.push_back(std::move(pane));
        }

        resource->textures_by_name.emplace("plate", make_solid_texture(255U, 255U, 255U, 255U));
        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_localized_text_resource(bool add_visibility_animation = false) {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.font_names.push_back("TestFont.brfnt");

        smgpc::assets::layout::MaterialDefinition material{};
        material.name = "TextMaterial";
        resource->layout.materials.push_back(std::move(material));

        smgpc::assets::layout::PaneDefinition root{};
        root.type = smgpc::assets::layout::PaneType::Pane;
        root.name = "RootPane";
        root.visible = true;
        root.alpha = 255U;
        root.size = {.x = 64.0F, .y = 64.0F};
        root.children = {1};
        resource->layout.panes.push_back(std::move(root));

        smgpc::assets::layout::PaneDefinition left{};
        left.type = smgpc::assets::layout::PaneType::Pane;
        left.name = "Left";
        left.parent = 0;
        left.visible = true;
        left.alpha = 255U;
        left.size = {.x = 64.0F, .y = 64.0F};
        left.children = {2, 3, 4};
        resource->layout.panes.push_back(std::move(left));

        const auto make_text = [](const char* name, bool visible) {
            smgpc::assets::layout::PaneDefinition pane{};
            pane.type = smgpc::assets::layout::PaneType::Text;
            pane.name = name;
            pane.parent = 1;
            pane.visible = visible;
            pane.alpha = 255U;
            pane.size = {.x = 64.0F, .y = 16.0F};
            pane.material_index = 0;
            pane.font_index = 0;
            pane.text_position = 0U;
            pane.text = u"A";
            pane.text_font_size = {.x = 8.0F, .y = 8.0F};
            pane.text_colors[0] = {.r = 255U, .g = 255U, .b = 255U, .a = 255U};
            pane.text_colors[1] = {.r = 255U, .g = 255U, .b = 255U, .a = 255U};
            return pane;
        };

        resource->layout.panes.push_back(make_text("TxtLeft", false));
        resource->layout.panes.push_back(make_text("TxtLeftKrKo", true));
        resource->layout.panes.push_back(make_text("TxtLeftCnSi", false));

        const auto parsed_font = smgpc::assets::layout::parse_brfnt(make_test_brfnt(), "TestFont");
        if (parsed_font) {
            resource->fonts_by_name.emplace("testfont", *parsed_font);
        }

        if (add_visibility_animation) {
            smgpc::assets::layout::BrlanAnimation animation{};
            animation.name = "ShowCnSi";
            animation.frame_size = 1U;
            animation.loop = false;
            animation.tracks.push_back(smgpc::assets::layout::BrlanTrack{
                .pane_name = "TxtLeftCnSi",
                .kind = "RLVI",
                .target = 0U,
                .curve_type = smgpc::assets::layout::BrlanCurveType::Step,
                .keys = {
                    smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 1.0F, .slope = 0.0F},
                },
            });
            resource->animations_by_name.emplace("showcnsi", std::move(animation));
        }

        return resource;
    }

    [[nodiscard]] std::shared_ptr< smgpc::game::layout::LayoutArchiveData > make_window_resource() {
        auto resource = std::make_shared< smgpc::game::layout::LayoutArchiveData >();
        resource->layout.center_origin = false;
        resource->layout.size = {.x = 64.0F, .y = 64.0F};
        resource->layout.root_pane = 0;
        resource->layout.texture_names.push_back("WindowBase.tpl");
        resource->layout.texture_names.push_back("WindowFrame.tpl");

        smgpc::assets::layout::MaterialDefinition base_material {};
        base_material.name = "WindowBase";
        base_material.texture_index = 0;
        base_material.texture_indices.push_back(0);
        resource->layout.materials.push_back(std::move(base_material));

        smgpc::assets::layout::MaterialDefinition frame_material {};
        frame_material.name = "WindowFrame";
        frame_material.texture_index = 1;
        frame_material.texture_indices.push_back(1);
        resource->layout.materials.push_back(std::move(frame_material));

        smgpc::assets::layout::PaneDefinition pane {};
        pane.type = smgpc::assets::layout::PaneType::Window;
        pane.name = "InfoWindow";
        pane.visible = true;
        pane.location_adjust = false;
        pane.alpha = 255U;
        pane.scale = {.x = 1.0F, .y = 1.0F};
        pane.size = {.x = 64.0F, .y = 16.0F};
        pane.material_index = 0;
        pane.window_frame_material_index = 1;
        pane.window_frames.push_back(smgpc::assets::layout::WindowFrameDefinition{
            .material_index = 1,
        });
        pane.base_position = 0U;
        pane.vertex_colors = std::array< smgpc::assets::layout::Color, 4 >{
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
            smgpc::assets::layout::Color{.r = 255U, .g = 255U, .b = 255U, .a = 255U},
        };
        resource->layout.panes.push_back(std::move(pane));

        resource->textures_by_name.emplace("windowbase", make_solid_texture(40U, 120U, 220U, 255U));
        resource->textures_by_name.emplace("windowframe", make_solid_texture(120U, 200U, 255U, 255U));
        return resource;
    }

}  // namespace

$test("LayoutRuntimeActor single-texture material keeps color texture") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource());
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require(not quad.use_mask_texture);
    $pc_port_require(quad.texture.id != 0U);
    $pc_port_require(quad.texture.width > 0U);
    $pc_port_require(quad.texture.height > 0U);
}

$test("LayoutRuntimeActor picture material color tints textured quads") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource({229U, 220U, 185U, 255U}));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(229U, 220U, 185U, 255U));
    $pc_port_require_eq(quad.color_tr, smgpc::render::layout::pack_abgr(229U, 220U, 185U, 255U));
    $pc_port_require_eq(quad.color_bl, smgpc::render::layout::pack_abgr(229U, 220U, 185U, 255U));
    $pc_port_require_eq(quad.color_br, smgpc::render::layout::pack_abgr(229U, 220U, 185U, 255U));
}

$test("LayoutRuntimeActor falls back to font color for black textured material color") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource({0U, 0U, 0U, 255U}, {255U, 255U, 255U, 255U}));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
}

$test("LayoutRuntimeActor falls back to font color for dark textured material color") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource({20U, 20U, 20U, 0U}, {255U, 255U, 255U, 255U}));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
}

$test("LayoutRuntimeActor falls back to font color for IconAButton dark textured material color") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource({50U, 50U, 50U, 0U}, {255U, 255U, 255U, 255U}));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
}

$test("LayoutRuntimeActor emits one-texture color-register lerp for grayscale material pairs") {
    auto resource = make_single_texture_resource({255U, 255U, 255U, 255U}, {200U, 220U, 245U, 255U});
    resource->textures_by_name["color"] = make_solid_texture(128U, 128U, 128U, 255U);
    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require(quad.texture_color_lerp);
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(200U, 220U, 245U, 255U));
}

$test("LayoutRuntimeActor applies texture color to multi-stage alpha materials") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource({0U, 160U, 255U, 255U}, {255U, 255U, 255U, 255U}, 2));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require(quad.texture_alpha_only);
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
    $pc_port_require_eq(quad.tev_color0, smgpc::render::layout::pack_abgr(0U, 160U, 255U, 255U));
    $pc_port_require_eq(quad.tev_color1, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
}

$test("LayoutRuntimeActor applies two-color IA path to frame-named materials") {
    auto resource = make_single_texture_resource({0U, 160U, 255U, 255U}, {255U, 255U, 255U, 255U}, 2);
    resource->layout.materials[0U].name = "PicLCFrame";
    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    $pc_port_require(draw_list.quads().front().texture_alpha_only);
}

$test("LayoutRuntimeActor applies BRLAN texture pattern animation to numbered sibling textures") {
    auto resource = make_texture_pattern_resource();
    const auto expected = reinterpret_cast< std::uintptr_t >(&resource->textures_by_name.at("myp2manual2"));
    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();
    actor.startAnim("Picture", 0U);
    actor.setAnimFrame(1.0F, 0U);

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.texture.id, static_cast< std::uint64_t >(expected));
}

$test("LayoutRuntimeActor applies BRLAN texture pattern animation to named texture table entries") {
    auto resource = make_named_texture_pattern_resource();
    const auto expected = reinterpret_cast< std::uintptr_t >(&resource->textures_by_name.at("mymiiyoshi"));
    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();
    actor.startAnim("Character", 0U);
    actor.setAnimFrame(1.0F, 0U);

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.texture.id, static_cast< std::uint64_t >(expected));
}

$test("LayoutRuntimeActor commits pane animation channels across slot replacements") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_pane_animation_persistence_resource());
    actor.appear();

    actor.startPaneAnim("RootPicture", "ButtonAppear", 0U);
    actor.setPaneAnimFrame("RootPicture", 10.0F, 0U);

    smgpc::render::layout::LayoutDrawList appear_draw_list {};
    actor.appendDrawCommands(&appear_draw_list);
    $pc_port_require(not appear_draw_list.quads().empty());
    $pc_port_require_eq(appear_draw_list.quads().front().color_tl, smgpc::render::layout::pack_abgr(150U, 255U, 255U, 255U));

    actor.startPaneAnim("RootPicture", "ButtonWait", 0U);

    smgpc::render::layout::LayoutDrawList wait_draw_list {};
    actor.appendDrawCommands(&wait_draw_list);
    $pc_port_require(not wait_draw_list.quads().empty());
    $pc_port_require_eq(wait_draw_list.quads().front().color_tl, smgpc::render::layout::pack_abgr(150U, 255U, 255U, 255U));
    $pc_port_require(std::fabs(wait_draw_list.quads().front().x1 - 128.0F) < 0.0001F);

    actor.appear();

    smgpc::render::layout::LayoutDrawList reset_draw_list {};
    actor.appendDrawCommands(&reset_draw_list);
    $pc_port_require(not reset_draw_list.quads().empty());
    $pc_port_require_eq(reset_draw_list.quads().front().color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
    $pc_port_require(std::fabs(reset_draw_list.quads().front().x1 - 64.0F) < 0.0001F);
}

$test("LayoutRuntimeActor commits full animation channels across layer replacements") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_pane_animation_persistence_resource());
    actor.appear();

    actor.startAnim("ButtonAppear", 0U);
    actor.setAnimFrame(10.0F, 0U);

    smgpc::render::layout::LayoutDrawList appear_draw_list {};
    actor.appendDrawCommands(&appear_draw_list);
    $pc_port_require(not appear_draw_list.quads().empty());
    $pc_port_require_eq(appear_draw_list.quads().front().color_tl, smgpc::render::layout::pack_abgr(150U, 255U, 255U, 255U));

    actor.startAnim("ButtonWait", 0U);

    smgpc::render::layout::LayoutDrawList wait_draw_list {};
    actor.appendDrawCommands(&wait_draw_list);
    $pc_port_require(not wait_draw_list.quads().empty());
    $pc_port_require_eq(wait_draw_list.quads().front().color_tl, smgpc::render::layout::pack_abgr(150U, 255U, 255U, 255U));
    $pc_port_require(std::fabs(wait_draw_list.quads().front().x1 - 128.0F) < 0.0001F);

    actor.kill();
    actor.appear();

    smgpc::render::layout::LayoutDrawList reset_draw_list {};
    actor.appendDrawCommands(&reset_draw_list);
    $pc_port_require(not reset_draw_list.quads().empty());
    $pc_port_require_eq(reset_draw_list.quads().front().color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
    $pc_port_require(std::fabs(reset_draw_list.quads().front().x1 - 64.0F) < 0.0001F);
}

$test("LayoutRuntimeActor pane texture override replaces and clears one material stage") {
    auto resource = make_single_texture_resource();
    const auto original = reinterpret_cast<std::uintptr_t>(&resource->textures_by_name.at("color"));
    smgpc::game::layout::LayoutRuntimeActor actor(resource);

    auto override_image = make_solid_texture(220U, 40U, 80U, 255U);
    nw4r::lyt::TexMap override_texture(&override_image);

    actor.replacePaneTexture("RootPicture", &override_texture, 0U);
    actor.appear();

    smgpc::render::layout::LayoutDrawList replaced_draw_list {};
    actor.appendDrawCommands(&replaced_draw_list);
    $pc_port_require(not replaced_draw_list.quads().empty());
    $pc_port_require_eq(replaced_draw_list.quads().front().texture.id, override_texture.textureId());
    $pc_port_require_eq(replaced_draw_list.quads().front().texture.width, override_image.width);

    actor.replacePaneTexture("RootPicture", nullptr, 0U);
    smgpc::render::layout::LayoutDrawList restored_draw_list {};
    actor.appendDrawCommands(&restored_draw_list);
    $pc_port_require(not restored_draw_list.quads().empty());
    $pc_port_require_eq(restored_draw_list.quads().front().texture.id, static_cast<std::uint64_t>(original));
}

$test("LayoutRuntimeActor pane texture override is scoped to the selected pane") {
    auto resource = make_single_texture_resource();
    auto second = make_picture_root_pane();
    second.name = "SecondPicture";
    second.parent = 0;
    second.translate.x = 64.0F;
    resource->layout.panes[0U].children.push_back(1);
    resource->layout.panes.push_back(second);

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    auto override_image = make_solid_texture(12U, 200U, 80U, 255U);
    nw4r::lyt::TexMap override_texture(&override_image);
    actor.replacePaneTexture("SecondPicture", &override_texture, 0U);

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);
    $pc_port_require_eq(draw_list.quads().size(), static_cast<std::size_t>(2U));
    $pc_port_require(draw_list.quads()[0U].texture.id != override_texture.textureId());
    $pc_port_require_eq(draw_list.quads()[1U].texture.id, override_texture.textureId());
}

$test("LayoutUtil replacePaneTexture wrapper reaches LayoutRuntimeActor") {
    auto resource = make_single_texture_resource();
    auto runtime = std::make_shared<smgpc::game::layout::LayoutRuntimeActor>(resource);
    LayoutActor actor("TextureReplaceTest", runtime);
    actor.appear();

    auto override_image = make_solid_texture(20U, 30U, 240U, 255U);
    nw4r::lyt::TexMap override_texture(&override_image);

    MR::replacePaneTexture(&actor, "RootPicture", &override_texture, 0U);
    $pc_port_require(MR::getLytTexMap(&actor, "RootPicture", 0U) == &override_texture);

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);
    $pc_port_require(not draw_list.quads().empty());
    $pc_port_require_eq(draw_list.quads().front().texture.id, override_texture.textureId());
}

$test("TexMap created from ResTIMG refreshes when source pixels change") {
    TestTimgStorage storage {};
    fill_rgb565_test_timg(&storage, 0xF800U);
    auto *timg = &storage.header;
    auto *tex_map = MR::createLytTexMap(timg);
    $pc_port_require(tex_map != nullptr);

    const auto *red_image = tex_map->decodedImage();
    const auto red_id = tex_map->textureId();
    $pc_port_require(red_image != nullptr);
    $pc_port_require(red_image->rgba8[0U] > 200U);
    $pc_port_require(red_image->rgba8[2U] < 16U);

    fill_rgb565_test_timg(&storage, 0x001FU);
    const auto *blue_image = tex_map->decodedImage();
    const auto blue_id = tex_map->textureId();
    $pc_port_require(blue_image != nullptr);
    $pc_port_require(blue_image->rgba8[0U] < 16U);
    $pc_port_require(blue_image->rgba8[2U] > 200U);
    $pc_port_require(red_id != blue_id);

    delete tex_map;
}

$test("LayoutRuntimeActor applies widescreen location adjust scale to flagged panes") {
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .is_widescreen = true,
    });
    auto resource = make_single_texture_resource();
    resource->layout.panes[0U].location_adjust = true;
    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require(std::fabs(quad.x0 - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.x1 - 48.0F) < 0.0001F);
}

$test("LayoutRuntimeActor pane follow position overrides a center-origin root pane") {
    auto resource = make_single_texture_resource();
    resource->layout.center_origin = true;

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    float follow_x = 30.0F;
    float follow_y = 40.0F;
    actor.setPaneFollowPosition(nullptr, &follow_x, &follow_y);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require(std::fabs(quad.x0 - 30.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.y0 - 40.0F) < 0.0001F);
}

$test("LayoutRuntimeActor two-texture material can emit mask rendering state") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_two_texture_resource());
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require(quad.texture.id != 0U);
    $pc_port_require(quad.texture.width > 0U);
    $pc_port_require(quad.texture.height > 0U);
    $pc_port_require(quad.use_mask_texture);
    $pc_port_require(quad.mask_texture.id != 0U);
    $pc_port_require(quad.mask_texture.width > 0U);
    $pc_port_require(quad.mask_texture.height > 0U);
}

$test("LayoutRuntimeActor narrow auxiliary texture pair uses visible alpha texture as primary") {
    auto resource = make_hand_pointer_resource();
    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.texture.width, static_cast< std::uint16_t >(64U));
    $pc_port_require_eq(quad.texture.height, static_cast< std::uint16_t >(64U));
    $pc_port_require(!quad.texture_alpha_only);
    $pc_port_require(!quad.use_mask_texture);
}

$test("LayoutRuntimeActor RLTP target 0x100 animates second texture stage") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_dpd_hand_type_resource());
    actor.appear();
    actor.startAnim("HandType", 0U);

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(!draw_list.quads().empty());
    $pc_port_require_eq(draw_list.quads().front().texture.rgba8[1U], static_cast< std::uint8_t >(220U));

    actor.setAnimFrame(1.0F, 0U);
    draw_list.clear();
    actor.appendDrawCommands(&draw_list);
    $pc_port_require(!draw_list.quads().empty());
    $pc_port_require_eq(draw_list.quads().front().texture.rgba8[0U], static_cast< std::uint8_t >(220U));

    actor.setAnimFrame(2.0F, 0U);
    draw_list.clear();
    actor.appendDrawCommands(&draw_list);
    $pc_port_require(!draw_list.quads().empty());
    $pc_port_require_eq(draw_list.quads().front().texture.rgba8[2U], static_cast< std::uint8_t >(220U));
}

$test("LayoutRuntimeActor base pane visibility follows selected localized sibling") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_localized_visibility_resource());
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);
    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));

    actor.setPaneVisible("PicPlate", false);
    draw_list.clear();
    actor.appendDrawCommands(&draw_list);
    $pc_port_require(draw_list.quads().empty());

    actor.setPaneVisible("PicPlate", true);
    draw_list.clear();
    actor.appendDrawCommands(&draw_list);
    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
}

$test("LayoutRuntimeActor recursive visibility preserves localized child gates") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_localized_text_resource());
    actor.appear();

    actor.setPaneVisibleRecursive("Left", false);
    actor.setTextBoxTextRecursive("Left", u"A");
    actor.setPaneVisibleRecursive("Left", true);

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
}

$test("LayoutRuntimeActor RLVI cannot re-show non-selected localized panes") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_localized_text_resource(true));
    actor.appear();
    actor.startAnim("ShowCnSi", 0U);

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
}

$test("LayoutRuntimeActor window panes draw content and frame geometry") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_window_resource());
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(5U));
    $pc_port_require(draw_list.quads()[0U].texture.id != 0U);
    $pc_port_require(draw_list.quads()[1U].texture.id != 0U);
    $pc_port_require(draw_list.quads()[0U].texture.id != draw_list.quads()[1U].texture.id);
    $pc_port_require(std::fabs(draw_list.quads()[0U].x0 - 2.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[0U].y0 - 2.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[0U].x1 - 62.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[0U].y1 - 14.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[1U].x0 - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[1U].y0 - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[1U].x1 - 62.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[1U].y1 - 2.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[2U].x0 - 62.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[2U].y0 - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[2U].x1 - 64.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[2U].y1 - 14.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[2U].u0 - 1.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[2U].u1 - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[3U].u0 - 31.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[3U].v0 - 1.0F) < 0.0001F);
}

$test("LayoutRuntimeActor applies BRLAN pane Z rotation to emitted quad vertices") {
    auto resource = make_single_texture_resource();
    smgpc::assets::layout::BrlanAnimation animation{};
    animation.name = "Rotate";
    animation.frame_size = 120U;
    animation.loop = true;
    animation.tracks.push_back(smgpc::assets::layout::BrlanTrack{
        .pane_name = "RootPicture",
        .kind = "RLPA",
        .target = 5U,
        .curve_type = smgpc::assets::layout::BrlanCurveType::Hermite,
        .keys = {
            smgpc::assets::layout::BrlanKey{.frame = 0.0F, .value = 0.0F, .slope = 0.0F},
            smgpc::assets::layout::BrlanKey{.frame = 30.0F, .value = 90.0F, .slope = 0.0F},
        },
    });
    resource->animations_by_name.emplace("rotate", std::move(animation));

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();
    actor.startAnim("Rotate", 0U);
    actor.setAnimFrame(30.0F, 0U);

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
    const auto& quad = draw_list.quads().front();
    $pc_port_require(quad.use_custom_vertices);
    $pc_port_require(std::fabs(quad.x_tl - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.y_tl - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.x_tr - 0.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.y_tr - 64.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.x_bl + 64.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.y_bl - 0.0F) < 0.0001F);
}

$test("LayoutRuntimeActor BRFNT text uses glyph width and skips zero-width glyph quads") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_text_resource(u" A"));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
    const auto& quad = draw_list.quads().front();
    $pc_port_require(std::fabs(quad.x0 - 5.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.x1 - 10.0F) < 0.0001F);
    $pc_port_require(std::fabs(quad.u0 - (10.0F / 32.0F)) < 0.0001F);
    $pc_port_require(std::fabs(quad.u1 - (15.0F / 32.0F)) < 0.0001F);
}

$test("LayoutRuntimeActor BRFNT text applies top and bottom text colors") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_text_resource(u"A"));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(255U, 0U, 0U, 255U));
    $pc_port_require_eq(quad.color_tr, smgpc::render::layout::pack_abgr(255U, 0U, 0U, 255U));
    $pc_port_require_eq(quad.color_bl, smgpc::render::layout::pack_abgr(0U, 0U, 255U, 128U));
    $pc_port_require_eq(quad.color_br, smgpc::render::layout::pack_abgr(0U, 0U, 255U, 128U));
}

$test("BRFNT grayscale font sheets render as white alpha masks") {
    auto bytes = make_test_brfnt();
    const std::array< std::byte, 4 > marker{
        static_cast< std::byte >('T'),
        static_cast< std::byte >('G'),
        static_cast< std::byte >('L'),
        static_cast< std::byte >('P'),
    };
    const auto tglp = std::search(bytes.begin(), bytes.end(), marker.begin(), marker.end());
    $pc_port_require(tglp != bytes.end());

    const auto tglp_offset = static_cast< std::size_t >(std::distance(bytes.begin(), tglp));
    const auto read_u32 = [&](std::size_t offset) {
        return (static_cast< std::uint32_t >(bytes[offset + 0U]) << 24U) | (static_cast< std::uint32_t >(bytes[offset + 1U]) << 16U) |
               (static_cast< std::uint32_t >(bytes[offset + 2U]) << 8U) | static_cast< std::uint32_t >(bytes[offset + 3U]);
    };
    const auto image_offset = static_cast< std::size_t >(read_u32(tglp_offset + 8U + 20U));
    $pc_port_require(image_offset < bytes.size());
    bytes[image_offset] = static_cast< std::byte >(0x80U);

    const auto parsed_font = smgpc::assets::layout::parse_brfnt(bytes, "TestFont");
    $pc_port_require(parsed_font);
    $pc_port_require(!parsed_font->sheets().empty());

    const auto& sheet = parsed_font->sheets().front();
    $pc_port_require_eq(sheet.rgba8[0U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(sheet.rgba8[1U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(sheet.rgba8[2U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(sheet.rgba8[3U], static_cast< std::uint8_t >(0x88U));
    $pc_port_require_eq(sheet.rgba8[4U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(sheet.rgba8[5U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(sheet.rgba8[6U], static_cast< std::uint8_t >(255U));
    $pc_port_require_eq(sheet.rgba8[7U], static_cast< std::uint8_t >(0U));
}

$test("LayoutRuntimeActor BMG picture tags render with PictureFont and untinted color") {
    auto resource = make_text_resource(std::u16string(1U, smgpc::assets::layout::make_bmg_picture_font_tag(0x0011U)));
    const auto parsed_font = smgpc::assets::layout::parse_brfnt(make_test_brfnt(), "PictureFont");
    $pc_port_require(parsed_font);
    resource->fonts_by_name.emplace("picturefont", *parsed_font);
    const auto expected_texture = reinterpret_cast< std::uintptr_t >(&resource->fonts_by_name.at("picturefont").sheets().front());

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
    const auto& quad = draw_list.quads().front();
    $pc_port_require_eq(quad.texture.id, static_cast< std::uint64_t >(expected_texture));
    $pc_port_require_eq(quad.color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
    $pc_port_require_eq(quad.color_bl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
}

$test("LayoutRuntimeActor static BRLYT button escapes render with PictureFont") {
    auto resource = make_text_resource(u"BiA");
    const auto parsed_font = smgpc::assets::layout::parse_brfnt(make_test_brfnt(), "PictureFont");
    $pc_port_require(parsed_font);
    resource->fonts_by_name.emplace("picturefont", *parsed_font);
    const auto expected_picture_texture = reinterpret_cast< std::uintptr_t >(&resource->fonts_by_name.at("picturefont").sheets().front());
    const auto expected_text_texture = reinterpret_cast< std::uintptr_t >(&resource->fonts_by_name.at("testfont").sheets().front());

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(2U));
    $pc_port_require_eq(draw_list.quads()[0U].texture.id, static_cast< std::uint64_t >(expected_picture_texture));
    $pc_port_require_eq(draw_list.quads()[0U].color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
    $pc_port_require_eq(draw_list.quads()[1U].texture.id, static_cast< std::uint64_t >(expected_text_texture));
}

$test("LayoutRuntimeActor Korean PressStart button markers render with PictureFont") {
    auto resource = make_text_resource(u"\uFF21\uC640B\uB97C");
    const auto parsed_font = smgpc::assets::layout::parse_brfnt(make_test_brfnt(), "PictureFont");
    $pc_port_require(parsed_font);
    resource->fonts_by_name.emplace("picturefont", *parsed_font);
    const auto expected_picture_texture = reinterpret_cast< std::uintptr_t >(&resource->fonts_by_name.at("picturefont").sheets().front());

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(2U));
    $pc_port_require_eq(draw_list.quads()[0U].texture.id, static_cast< std::uint64_t >(expected_picture_texture));
    $pc_port_require_eq(draw_list.quads()[1U].texture.id, static_cast< std::uint64_t >(expected_picture_texture));
    $pc_port_require_eq(draw_list.quads()[0U].color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
    $pc_port_require_eq(draw_list.quads()[1U].color_tl, smgpc::render::layout::pack_abgr(255U, 255U, 255U, 255U));
}

$test("LayoutRuntimeActor BRFNT text measures character spacing between glyphs") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_text_resource(u"AA", 4.0F, 1U));
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(2U));
    $pc_port_require(std::fabs(draw_list.quads()[0U].x0 - 24.0F) < 0.0001F);
    $pc_port_require(std::fabs(draw_list.quads()[1U].x0 - 35.0F) < 0.0001F);
}

$test("LayoutRuntimeActor can override recursive text vertical position") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_text_resource(u"A", 0.0F, 1U));
    actor.appear();
    actor.setTextBoxVerticalPositionRecursive("Text", 2U);

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
    $pc_port_require(std::fabs(draw_list.quads().front().y0 - 8.0F) < 0.0001F);
}

$test("LayoutRuntimeActor bottom text uses BRFNT line feed for block height") {
    auto resource = make_text_resource(u"A", 0.0F, 1U);
    auto bytes = make_test_brfnt();
    const std::array< std::byte, 4 > marker{
        static_cast< std::byte >('F'),
        static_cast< std::byte >('I'),
        static_cast< std::byte >('N'),
        static_cast< std::byte >('F'),
    };
    const auto finf = std::search(bytes.begin(), bytes.end(), marker.begin(), marker.end());
    $pc_port_require(finf != bytes.end());
    bytes[static_cast< std::size_t >(std::distance(bytes.begin(), finf)) + 8U + 1U] = static_cast< std::byte >(12U);

    const auto parsed_font = smgpc::assets::layout::parse_brfnt(bytes, "TestFont");
    $pc_port_require(parsed_font);
    resource->fonts_by_name["testfont"] = *parsed_font;

    smgpc::game::layout::LayoutRuntimeActor actor(resource);
    actor.appear();
    actor.setTextBoxVerticalPositionRecursive("Text", 2U);

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(1U));
    $pc_port_require(std::fabs(draw_list.quads().front().y0 - 4.0F) < 0.0001F);
}

$test("LayoutRuntimeActor can override a text pane recursively") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_parented_text_resource());
    actor.appear();
    actor.setTextBoxTextRecursive("FileNumber", u"AA");

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(2U));
}

$test("LayoutRuntimeActor can clear a text pane recursively") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_parented_text_resource());
    actor.appear();
    actor.clearTextBoxTextRecursive("FileNumber");

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(draw_list.quads().empty());
}

$test("LayoutRuntimeActor can override all text panes with null recursive root") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_parented_text_resource());
    actor.appear();
    actor.setTextBoxTextRecursive(nullptr, u"AA");

    smgpc::render::layout::LayoutDrawList draw_list{};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require_eq(draw_list.quads().size(), static_cast< std::size_t >(2U));
}
