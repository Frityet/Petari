#include "layout/Nw4rLayoutRecords.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/LytTexMap.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/Util/MessageUtil.hpp"
#include <nw4r/lyt/group.h>
#include <nw4r/lyt/textBox.h>
#include <nw4r/lyt/picture.h>
#include <nw4r/lyt/window.h>
#include <nw4r/lyt/bounding.h>
#include <nw4r/lyt/resourceAccessor.h>
#include <dolphin/os/OSFastCast.h>
#include <nw4r/lyt/material.h>
#include <nw4r/lyt/layout.h>
#include "Game/Screen/CustomTagProcessor.hpp"
#include <cstdlib>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef NDEBUG
#include <iomanip>
#include <ostream>
#include <nw4r/ut/Font.h>
#include <nw4r/ut/Rect.h>
#endif

namespace smgpc::layout {
namespace {
constexpr u32 kDetachedPane = std::numeric_limits<u32>::max();
class NativePaneIdentity {
public:
    NativePaneIdentity(Nw4rLayoutRecords& owner, u32 index) : owner(owner), index(index) {}
    virtual ~NativePaneIdentity() = default;
    Nw4rLayoutRecords& owner;
    u32 index;
};

class NativePaneRecord final : public nw4r::lyt::Pane, public NativePaneIdentity {
public:
    NativePaneRecord(const nw4r::lyt::res::Pane& resource, Nw4rLayoutRecords& owner, u32 index)
        : Pane(&resource), NativePaneIdentity(owner, index) { mbUserAllocated = true; }
};

class NativeResourceAccessor final : public nw4r::lyt::ResourceAccessor {
public:
    explicit NativeResourceAccessor(const std::vector<LayoutRuntime::RenderTexture>& textures,
                                    const resource::RarcArchive* archive = nullptr) : textures(textures), archive(archive) {}
    const nw4r::lyt::TexMap& texture(const char* name) {
        const auto it = std::ranges::find(textures, name, &LayoutRuntime::RenderTexture::name);
        if (it != textures.end() && it->native_texture) return *it->native_texture;
        if (auto cached = animation_textures.find(name); cached != animation_textures.end()) return cached->second;
        if (archive) {
            auto* entry = archive->find_resource(name);
            if (!entry) entry = archive->find_by_basename(name);
            if (entry) return animation_textures.emplace(name, make_tex_map(name, archive->file_data(*entry), nullptr)).first->second;
        }
        aurora::throw_host_exception<std::runtime_error>("Missing retained layout texture: " + std::string(name));
    }
    void* GetResource(nw4r::lyt::ResType type, const char* name, u32* size) override {
        if (type != 0x74696d67)
            aurora::throw_host_exception<std::invalid_argument>("The material requested a non-texture layout resource");
        const auto& state = texture(name).GetHostResourceState();
        if (!state || !state->tpl_palette)
            aurora::throw_host_exception<std::invalid_argument>("A layout material requires a retained native TPL palette");
        if (size) *size = sizeof(TPLPalette);
        return const_cast<TPLPalette*>(state->tpl_palette);
    }
    std::shared_ptr<const nw4r::lyt::HostTextureResourceState> GetHostTextureResourceState(const char* name) override {
        return texture(name).GetHostResourceState();
    }
    const std::vector<LayoutRuntime::RenderTexture>& textures;
    const resource::RarcArchive* archive;
    std::map<std::string, nw4r::lyt::TexMap, std::less<>> animation_textures;
};

class NativeMaterialResources {
public:
    NativeMaterialResources(const BrlytLayout& source, const std::vector<LayoutRuntime::RenderTexture>& textures)
        : source(source), accessor(textures) {
        using namespace nw4r::lyt;
        const auto table_size = source.texture_names.size() * sizeof(res::Texture);
        std::size_t size = sizeof(res::TextureList) + table_size;
        for (const auto& name : source.texture_names) size += name.size() + 1;
        names.resize(size);
        auto* list = reinterpret_cast<res::TextureList*>(names.data());
        list->texNum = static_cast<u16>(source.texture_names.size());
        auto* table = reinterpret_cast<res::Texture*>(names.data() + sizeof(res::TextureList));
        std::size_t offset = table_size;
        for (std::size_t i = 0; i < source.texture_names.size(); ++i) {
            const auto& name = source.texture_names[i];
            table[i].nameStrOffset = static_cast<u32>(offset);
            std::memcpy(reinterpret_cast<char*>(table) + offset, name.c_str(), name.size() + 1);
            offset += name.size() + 1;
        }
        blocks.pTextureList = list;
        blocks.pResAccessor = &accessor;
    }
    nw4r::lyt::Material* create(std::size_t index) {
        using namespace nw4r::lyt;
        const auto& material = source.materials.at(index);
        if (material.native_resource.size() < sizeof(res::Material))
            aurora::throw_host_exception<std::runtime_error>("Missing native BRLYT material record");
        for (const auto& texture : material.textures) {
            if (texture.texture_index >= source.texture_names.size())
                aurora::throw_host_exception<std::runtime_error>("BRLYT material texture index is out of range");
            accessor.GetResource(0x74696d67, texture.texture_name.c_str(), nullptr);
        }
        auto* memory = Layout::AllocMemory(sizeof(Material));
        if (!memory) aurora::throw_host_exception<std::runtime_error>("NW4R material allocation failed");
        Material* result;
        try { result = new (memory) Material(reinterpret_cast<const res::Material*>(material.native_resource.data()), blocks); }
        catch (...) { Layout::FreeMemory(memory); throw; }
        return result;
    }
    const BrlytLayout& source;
    NativeResourceAccessor accessor;
    std::vector<std::uint8_t> names;
    nw4r::lyt::ResBlockSet blocks{};
};

void set_tex_coords(nw4r::lyt::detail::TexCoordAry& target, const std::vector<std::array<BrlytTexCoord, 4>>& source) {
    const auto count = static_cast<u8>(std::min<std::size_t>(source.size(), GX_MAX_TEXCOORD));
    target.Reserve(count);
    target.SetSize(count);
    for (u32 set = 0; set < target.GetSize(); ++set)
        for (u32 corner = 0; corner < 4; ++corner)
            target.mData[set][corner] = {source[set][corner].u, source[set][corner].v};
}
GXColor native_color(const std::array<std::uint8_t, 4>& color) { return {color[0], color[1], color[2], color[3]}; }

class NativePictureRecord final : public nw4r::lyt::Picture, public NativePaneIdentity {
public:
    NativePictureRecord(const nw4r::lyt::res::Pane& pane, const BrlytPicturePane& picture, NativeMaterialResources& resources,
                        Nw4rLayoutRecords& owner, u32 index)
        : Picture(&pane, 0), NativePaneIdentity(owner, index) {
        mbUserAllocated = true;
        for (std::size_t i = 0; i < 4; ++i) mVtxColors[i] = native_color(picture.vertex_colors[i]);
        set_tex_coords(mTexCoordAry, picture.tex_coord_sets);
        mpMaterial = resources.create(picture.material_index);
    }
};
class NativeWindowRecord final : public nw4r::lyt::Window, public NativePaneIdentity {
public:
    NativeWindowRecord(const nw4r::lyt::res::Pane& pane, const BrlytWindowPane& window, NativeMaterialResources& resources,
                       Nw4rLayoutRecords& owner, u32 index)
        : Window(&pane), NativePaneIdentity(owner, index) {
        mbUserAllocated = true;
        mContentInflation = {window.content_inflation.left, window.content_inflation.right,
                             window.content_inflation.top, window.content_inflation.bottom};
        for (std::size_t i = 0; i < 4; ++i) mContent.vtxColors[i] = native_color(window.content.vertex_colors[i]);
        set_tex_coords(mContent.texCoordAry, window.content.tex_coord_sets);
        mpMaterial = resources.create(window.content.material_index);
        if (!window.frames.empty()) {
            mFrames = nw4r::lyt::Layout::NewArray<Frame>(static_cast<u32>(window.frames.size()));
            if (!mFrames) aurora::throw_host_exception<std::runtime_error>("NW4R window frame allocation failed");
            for (const auto& frame : window.frames) {
                mFrames[mFrameNum].textureFlip = frame.texture_flip;
                mFrames[mFrameNum].pMaterial = resources.create(frame.material_index);
                ++mFrameNum;
            }
        }
    }
};
class NativeBoundingRecord final : public nw4r::lyt::Bounding, public NativePaneIdentity {
public:
    NativeBoundingRecord(const nw4r::lyt::res::Bounding& pane, Nw4rLayoutRecords& owner, u32 index)
        : Bounding(&pane, nw4r::lyt::ResBlockSet{}), NativePaneIdentity(owner, index) { mbUserAllocated = true; }
};
class NativeTextBoxRecord final : public nw4r::lyt::TextBox, public NativePaneIdentity {
public:
    NativeTextBoxRecord(const nw4r::lyt::res::Pane& pane, const BrlytTextBox& text,
                        NativeMaterialResources& resources, const nw4r::ut::Font* font,
                        Nw4rLayoutRecords& owner, u32 index)
        : TextBox(&pane, text.resource.buffer_length), NativePaneIdentity(owner, index) {
        mbUserAllocated = true;
        mpMaterial = resources.create(text.material_index);
        mpFont = font;
        mFontSize = {text.resource.font_width, text.resource.font_height};
        mCharSpace = text.resource.char_space;
        mLineSpace = text.resource.line_space;
        mTextPosition = text.text_position;
        mBits.textAlignment = text.text_alignment;
        mTextColors[0] = native_color(text.resource.top_color);
        mTextColors[1] = native_color(text.resource.bottom_color);
        std::wstring message;
        message.reserve(text.raw_text.size());
        for (auto unit : text.raw_text) message.push_back(static_cast<wchar_t>(unit));
        SetString(message.data(), 0, static_cast<u16>(message.size()));
    }
    ~NativeTextBoxRecord() override {
        delete static_cast<CustomTagProcessor*>(mpTagProcessor);
        mpTagProcessor = nullptr;
    }
};

void* allocate_layout(MEMAllocator*, u32 size) {
    const aurora::allocation::HostAllocationScope host;
    return std::malloc(size);
}
void free_layout(MEMAllocator*, void* data) {
    const aurora::allocation::HostAllocationScope host;
    std::free(data);
}
const MEMAllocatorFunc layout_allocator_functions{allocate_layout, free_layout};
MEMAllocator layout_allocator{&layout_allocator_functions, nullptr, 0, 0};

class NativeGroupRecord final : public nw4r::lyt::Group {
public:
    NativeGroupRecord(const nw4r::lyt::res::Group* resource, nw4r::lyt::Pane* root, Nw4rLayoutRecords& owner)
        : Group(resource, root), owner(owner) { mbUserAllocated = true; }
    Nw4rLayoutRecords& owner;
};
struct PublishedPane {
    nw4r::math::VEC3 translate;
    nw4r::math::VEC3 rotate;
    nw4r::math::VEC2 scale;
    nw4r::lyt::Size size;
    u8 alpha;
    u8 flags;
};
PublishedPane published(const nw4r::lyt::Pane& pane) {
    return {pane.mTranslate, pane.mRotate, pane.mScale, pane.mSize, pane.mAlpha, pane.mFlag};
}
}

struct Nw4rLayoutRecords::State {
    explicit State(LayoutRuntime& runtime) : runtime(runtime) {}
    ~State() {
        retiring = true;
        if (layout) {
            // Animation links are embedded in the transforms. Remove them from
            // the borrowed panes/materials before the actual Layout retires them.
            layout->UnbindAllAnimation();
            for (auto& pane : detached_panes) pane->UnbindAllAnimation(true);
            layout->mpRootPane = nullptr;
            layout.reset();
        }
        groups.clear();
        // All linked records remain alive until every intrusive child list has
        // been detached, including construction failures and vector teardown.
        for (auto& pane : panes) {
            while (pane->mChildList.GetSize()) {
                auto* child = &*pane->mChildList.GetBeginIter();
                pane->mChildList.Erase(child);
                child->mpParent = nullptr;
            }
        }
        for (auto& pane : detached_panes) {
            while (pane->mChildList.GetSize()) {
                auto* child = &*pane->mChildList.GetBeginIter();
                pane->mChildList.Erase(child);
                child->mpParent = nullptr;
            }
        }
    }
    LayoutRuntime& runtime;
    bool published = false;
    bool retiring = false;
    bool synchronizing = false;
    std::unique_ptr<nw4r::lyt::Layout> layout;
    std::vector<std::unique_ptr<nw4r::lyt::Pane>> panes;
    // Retail RemoveChild only unlinks. Groups already constructed from the
    // resource keep their original pane identities until the layout retires.
    std::vector<std::unique_ptr<nw4r::lyt::Pane>> detached_panes;
    std::vector<PublishedPane> previous;
    std::vector<std::unique_ptr<nw4r::lyt::Group>> groups;
};

Nw4rLayoutRecords::Nw4rLayoutRecords(LayoutRuntime& runtime) {
    const aurora::allocation::HostAllocationScope host;
    _state = std::make_unique<State>(runtime);
    if (!nw4r::lyt::Layout::mspAllocator) nw4r::lyt::Layout::mspAllocator = &layout_allocator;
    runtime.loadRenderData();
    const auto& layout = runtime.mBrlytLayout;
    if (layout.panes.empty()) aurora::throw_host_exception<std::logic_error>("NW4R pane ownership requires an actual BRLYT root");
    NativeMaterialResources materials(layout, runtime.mRenderTextures);
    _state->panes.reserve(layout.panes.size());
    _state->previous.resize(layout.panes.size());
    for (const auto& source : layout.panes) {
        nw4r::lyt::res::Pane resource{};
        std::copy(source.resource_kind.begin(), source.resource_kind.end(), resource.blockHeader.kind);
        resource.blockHeader.size = sizeof(resource);
        resource.flag = (source.visible ? 1 : 0) | (source.influenced_alpha ? 2 : 0) | (source.location_adjust ? 4 : 0);
        resource.basePosition = source.base_position;
        resource.alpha = source.alpha;
        std::strncpy(resource.name, source.name.c_str(), sizeof(resource.name));
        std::strncpy(resource.userData, source.user_data.c_str(), sizeof(resource.userData));
        resource.translate.x = source.translate_x; resource.translate.y = source.translate_y; resource.translate.z = source.translate_z;
        resource.rotate.x = source.rotate_x; resource.rotate.y = source.rotate_y; resource.rotate.z = source.rotate_z;
        resource.scale.x = source.scale_x; resource.scale.y = source.scale_y;
        resource.size.width = source.width; resource.size.height = source.height;
        const auto index = static_cast<u32>(_state->panes.size());
        if (source.resource_kind == std::array<char, 4>{'t', 'x', 't', '1'}) {
            const auto text = std::ranges::find_if(layout.text_boxes, [index](const auto& entry) { return entry.pane_index == index; });
            if (text == layout.text_boxes.end())
                aurora::throw_host_exception<std::logic_error>("NW4R text pane has no decoded text resource");
            const nw4r::ut::Font* font = nullptr;
            if (runtime.mArchiveOwner) {
                font = runtime.mArchiveOwner->GetFont(text->font_name.c_str());
            } else {
                const auto entry = std::ranges::find(runtime.mRenderFonts, text->font_name, &LayoutRuntime::RenderFont::name);
                if (entry != runtime.mRenderFonts.end()) font = entry->native_font.get();
            }
            _state->panes.push_back(std::make_unique<NativeTextBoxRecord>(resource, *text, materials, font, *this, index));
        } else if (source.resource_kind == std::array<char, 4>{'p', 'i', 'c', '1'}) {
            const auto picture = std::ranges::find(layout.pictures, index, &BrlytPicturePane::pane_index);
            if (picture == layout.pictures.end()) aurora::throw_host_exception<std::logic_error>("Missing decoded picture record");
            _state->panes.push_back(std::make_unique<NativePictureRecord>(resource, *picture, materials, *this, index));
        } else if (source.resource_kind == std::array<char, 4>{'w', 'n', 'd', '1'}) {
            const auto window = std::ranges::find(layout.windows, index, &BrlytWindowPane::pane_index);
            if (window == layout.windows.end()) aurora::throw_host_exception<std::logic_error>("Missing decoded window record");
            _state->panes.push_back(std::make_unique<NativeWindowRecord>(resource, *window, materials, *this, index));
        } else if (source.resource_kind == std::array<char, 4>{'b', 'n', 'd', '1'}) {
            nw4r::lyt::res::Bounding bounding{};
            static_cast<nw4r::lyt::res::Pane&>(bounding) = resource;
            _state->panes.push_back(std::make_unique<NativeBoundingRecord>(bounding, *this, index));
        } else if (source.resource_kind == std::array<char, 4>{'p', 'a', 'n', '1'}) {
            _state->panes.push_back(std::make_unique<NativePaneRecord>(resource, *this, index));
        } else {
            aurora::throw_host_exception<std::logic_error>("Unknown BRLYT pane kind has no native SDK class");
        }
    }
    for (std::size_t i = 0; i < layout.panes.size(); ++i) {
        const auto parent = layout.panes[i].parent_index;
        if (parent >= 0) _state->panes.at(static_cast<std::size_t>(parent))->AppendChild(_state->panes[i].get());
    }
    _state->layout = std::make_unique<nw4r::lyt::Layout>();
    _state->layout->mpRootPane = _state->panes.front().get();
    _state->layout->mLayoutSize = nw4r::lyt::Size(layout.width, layout.height);
    _state->layout->_20 = layout.origin_type != 0 ? nw4r::lyt::ORIGINTYPE_CENTER : nw4r::lyt::ORIGINTYPE_TOPLEFT;
    _state->layout->mpGroupContainer = nw4r::lyt::Layout::NewObj<nw4r::lyt::GroupContainer>();
    if (!_state->layout->mpGroupContainer)
        aurora::throw_host_exception<std::runtime_error>("NW4R group container allocation failed");
    for (const auto& source : layout.groups) {
        // NW4R Layout::Build retains only immediate children of the root group.
        if (source.root_group || source.nest_level != 1) continue;
        std::vector<u8> decoded(sizeof(nw4r::lyt::res::Group) + source.pane_names.size() * 16);
        auto* resource = reinterpret_cast<nw4r::lyt::res::Group*>(decoded.data());
        std::copy_n("grp1", 4, resource->blockHeader.kind);
        resource->blockHeader.size = static_cast<u32>(decoded.size());
        std::strncpy(resource->name, source.name.c_str(), sizeof(resource->name));
        resource->paneNum = static_cast<u16>(source.pane_names.size());
        auto* names = reinterpret_cast<char*>(decoded.data()) + sizeof(*resource);
        for (std::size_t i = 0; i < source.pane_names.size(); ++i)
            std::strncpy(names + 16 * i, source.pane_names[i].c_str(), 16);
        _state->groups.push_back(std::make_unique<NativeGroupRecord>(resource, _state->panes.front().get(), *this));
        _state->layout->mpGroupContainer->AppendGroup(_state->groups.back().get());
    }
    synchronize();
}
Nw4rLayoutRecords::~Nw4rLayoutRecords() = default;



nw4r::lyt::Layout& Nw4rLayoutRecords::layout() { return *_state->layout; }

nw4r::lyt::AnimTransform* Nw4rLayoutRecords::create_animation(const void* resource) {
    const aurora::allocation::HostAllocationScope host;
    const auto& runtime = _state->runtime;
    NativeResourceAccessor accessor(runtime.mRenderTextures, runtime.mArchiveOwner ? &runtime.mArchiveOwner->nativeResourceSource() : nullptr);
    auto* transform = _state->layout->CreateAnimTransform(resource, &accessor);
    if (!transform) aurora::throw_host_exception<std::runtime_error>("NW4R animation resource could not create a transform");
    return transform;
}

void Nw4rLayoutRecords::synchronize() {
    const aurora::allocation::HostAllocationScope host;
    auto& state = *_state;
    if (state.synchronizing) return;
    state.synchronizing = true;
    struct Restore { bool& value; ~Restore() { value = false; } } restore{state.synchronizing};
    if (state.published) for (u32 i = 0; i < state.panes.size(); ++i) import_pane(i);
    for (u32 i = 0; i < state.panes.size(); ++i) publish_pane(i, true);
    state.published = true;
}

void Nw4rLayoutRecords::import_pane(u32 i) {
    auto& state = *_state;
    auto& runtime = state.runtime;
    auto& pane = *state.panes[i];
    const auto& old = state.previous[i];
    auto& source = runtime.mBrlytLayout.panes[i];
    const auto* expected_parent = source.parent_index < 0 ? nullptr : state.panes.at(source.parent_index).get();
    if (std::strncmp(pane.mName, source.name.c_str(), 16) != 0 || pane.mpParent != expected_parent)
        aurora::throw_host_exception<std::logic_error>("Changing a published NW4R resource name/hierarchy requires matching renderer topology support");
    auto& frame = runtime.mCommittedPaneFrames[source.name];
    if (pane.mSize.width != old.size.width) frame.width = pane.mSize.width;
    if (pane.mSize.height != old.size.height) frame.height = pane.mSize.height;
    if (pane.mTranslate.x != old.translate.x) frame.translate_x = pane.mTranslate.x;
    if (pane.mTranslate.y != old.translate.y) frame.translate_y = pane.mTranslate.y;
    if (pane.mTranslate.z != old.translate.z) frame.translate_z = pane.mTranslate.z;
    if (pane.mRotate.x != old.rotate.x) frame.rotate_x = pane.mRotate.x;
    if (pane.mRotate.y != old.rotate.y) frame.rotate_y = pane.mRotate.y;
    if (pane.mScale.x != old.scale.x) frame.scale_x = pane.mScale.x;
    if (pane.mScale.y != old.scale.y) frame.scale_y = pane.mScale.y;
    if (pane.mRotate.z != old.rotate.z) frame.rotate_z = pane.mRotate.z;
    if (pane.mAlpha != old.alpha) runtime.mPaneAlphaOverrides[source.name] = pane.mAlpha;
    if ((pane.mFlag & 1) != (old.flags & 1)) runtime.setPaneVisible(source.name, pane.IsVisible());
    if ((pane.mFlag & 2) != (old.flags & 2)) source.influenced_alpha = pane.IsInfluencedAlpha();
    if ((pane.mFlag & 4) != (old.flags & 4)) source.location_adjust = pane.IsLocationAdjust();
}

void Nw4rLayoutRecords::publish_pane(u32 i, bool matrices) {
    auto& state = *_state;
    auto& runtime = state.runtime;
    auto& pane = *state.panes[i];
    const auto& source = runtime.mBrlytLayout.panes[i];
    const auto anim = runtime.animationFrameForPane(source.name);
    pane.mTranslate.x = anim.translate_x.value_or(source.translate_x);
    pane.mTranslate.y = anim.translate_y.value_or(source.translate_y);
    pane.mTranslate.z = anim.translate_z.value_or(source.translate_z);
    pane.mRotate.x = anim.rotate_x.value_or(source.rotate_x);
    pane.mRotate.y = anim.rotate_y.value_or(source.rotate_y);
    pane.mRotate.z = anim.rotate_z.value_or(source.rotate_z);
    pane.mScale.x = anim.scale_x.value_or(source.scale_x);
    pane.mScale.y = anim.scale_y.value_or(source.scale_y);
    pane.mSize.width = anim.width.value_or(source.width);
    pane.mSize.height = anim.height.value_or(source.height);
    pane.mAlpha = static_cast<u8>(std::clamp(anim.alpha.value_or(float(source.alpha)), 0.0F, 255.0F));
    if (const auto override = runtime.mPaneAlphaOverrides.find(source.name); override != runtime.mPaneAlphaOverrides.end())
        pane.mAlpha = static_cast<u8>(std::clamp(override->second, 0.0F, 255.0F));
    auto visible = anim.visible.value_or(source.visible);
    if (const auto override = runtime.mPaneVisibilityOverrides.find(source.name); override != runtime.mPaneVisibilityOverrides.end())
        visible = override->second;
    pane.mFlag = (visible ? 1 : 0) | (source.influenced_alpha ? 2 : 0) | (source.location_adjust ? 4 : 0);
    if (matrices) {
        // Use the very same active BRLAN, pane follow and actor transforms as
        // the renderer. These are live SDK matrices, not resource-only copies.
        const auto global = runtime.paneRenderState(i);
        PSMTXCopy(global.matrix, pane.mGlbMtx.m);
        pane.mGlbMtx.m[0][3] += runtime.mTransX;
        pane.mGlbMtx.m[1][3] += runtime.mTransY;
        pane.mGlbAlpha = static_cast<u8>(std::clamp(global.alpha, 0.0F, 255.0F));
        runtime.paneLocalMatrix(i, pane.mMtx.m);
    }
    const auto animate_material = [&](nw4r::lyt::Material* material) {
        if (!material) return;
        const auto frame = runtime.materialFrameForContent(material->GetName());
        const auto apply_color = [&](const auto& values, u32 first) {
            for (u32 component = 0; component < values.size(); ++component) {
                if (!values[component]) continue;
                f32 value = *values[component] + 0.5f;
                s16 converted;
                OSf32tos16(&value, &converted);
                material->SetColorElement(first + component, std::clamp(converted, s16(-1024), s16(1023)));
            }
        };
        apply_color(frame.material_color, 0);
        for (u32 color = 0; color < frame.tev_colors.size(); ++color) apply_color(frame.tev_colors[color], 4 + color * 4);
        for (u32 color = 0; color < frame.tev_k_colors.size(); ++color) apply_color(frame.tev_k_colors[color], 16 + color * 4);
        if (material->GetTexSRTCap()) {
            const auto texture = runtime.textureFrameForContent(material->GetName());
            const std::array values{texture.translate_s, texture.translate_t, texture.rotate, texture.scale_s, texture.scale_t};
            for (u32 component = 0; component < values.size(); ++component)
                if (values[component]) material->SetTexSRTElement(0, component, *values[component]);
        }
    };
    animate_material(pane.GetMaterial());
    if (auto* window = nw4r::ut::DynamicCast<nw4r::lyt::Window*>(&pane))
        for (u32 frame = 0; frame < window->mFrameNum; ++frame) animate_material(window->GetFrameMaterial(frame));
    state.previous[i] = published(pane);
}

void Nw4rLayoutRecords::animate_pane(u32 index) {
    const aurora::allocation::HostAllocationScope host;
    // Original AnimateSelf applies this pane's local animation only. Global
    // matrices are published by the later layout matrix/follow phase.
    if (_state->published) import_pane(index);
    publish_pane(index, false);
}

nw4r::lyt::Pane* Nw4rLayoutRecords::pane(const char* name) {
    synchronize();
    if (!name) return _state->panes.front().get();
    // Retail manager lookup uses first pane in original depth-first order.
    for (auto& pane : _state->panes) if (std::strcmp(pane->mName, name) == 0) return pane.get();
    return nullptr;
}
nw4r::lyt::Group* Nw4rLayoutRecords::group(const char* name) {
    for (auto& group : _state->groups) if (name && std::strcmp(group->GetName(), name) == 0) return group.get();
    return nullptr;
}
u32 Nw4rLayoutRecords::group_index(const char* name) const {
    for (u32 i = 0; i < _state->groups.size(); ++i)
        if (name && std::strcmp(_state->groups[i]->GetName(), name) == 0) return i;
    return group_count();
}
u32 Nw4rLayoutRecords::pane_count() const { return static_cast<u32>(_state->panes.size()); }
u32 Nw4rLayoutRecords::group_count() const { return static_cast<u32>(_state->groups.size()); }
u32 Nw4rLayoutRecords::pane_index(const nw4r::lyt::Pane* pane) const {
    if (const auto* record = dynamic_cast<const NativePaneIdentity*>(pane); record && &record->owner == this && record->index != kDetachedPane)
        return record->index;
    aurora::throw_host_exception<std::logic_error>("The NW4R pane belongs to a different layout resource owner");
}
u32 Nw4rLayoutRecords::text_line_count(const char* pane_name) const {
    const aurora::allocation::HostAllocationScope host;
    auto* root = const_cast<Nw4rLayoutRecords*>(this)->pane(pane_name);
    if (!root) return 0;
    const auto root_index = pane_index(root);
    const auto& layout = _state->runtime.mBrlytLayout;
    u32 maximum = 0;
    for (const auto& text : layout.text_boxes) {
        auto index = static_cast<s32>(text.pane_index);
        while (index >= 0 && static_cast<u32>(index) != root_index) index = layout.panes[index].parent_index;
        if (index < 0) continue;
        const auto* box = nw4r::ut::DynamicCast<const nw4r::lyt::TextBox*>(_state->panes.at(text.pane_index).get());
        if (box && box->mTextBuf) maximum = std::max(maximum, static_cast<u32>(MR::countMessageLine(box->mTextBuf)));
    }
    return maximum;
}
#ifndef NDEBUG
void Nw4rLayoutRecords::debug_dump_text(std::ostream& output) const {
    output << " DETACHED_PANES " << _state->detached_panes.size() << '\n';
    for (const auto& record : _state->panes) {
        const auto& pane = *record;
        bool visible = pane.IsVisible();
        for (auto* parent = pane.mpParent; parent; parent = parent->mpParent) visible &= parent->IsVisible();
        output << " PANE " << pane.mName << " parent=" << (pane.mpParent ? pane.mpParent->mName : "-")
               << " visible=" << visible << " alpha=" << unsigned(pane.mAlpha) << '/' << unsigned(pane.mGlbAlpha)
               << " position=" << pane.mTranslate.x << ',' << pane.mTranslate.y << ',' << pane.mTranslate.z
               << " size=" << pane.mSize.width << ',' << pane.mSize.height << " base=" << unsigned(pane.mBasePosition)
               << " matrix=";
        for (const auto& row : pane.mGlbMtx.m) for (float value : row) output << value << ',';
        output << '\n';
        auto* box = nw4r::ut::DynamicCast<const nw4r::lyt::TextBox*>(&pane);
        if (!box) continue;
        output << "  TEXT font=" << box->mFontSize.width << ',' << box->mFontSize.height
               << " spacing=" << box->mCharSpace << ',' << box->mLineSpace
               << " position=" << unsigned(box->mTextPosition) << " alignment=" << unsigned(box->GetTextAlignment())
               << " color=" << std::hex << u32(box->mTextColors[0]) << ',' << u32(box->mTextColors[1]) << std::dec;
        if (box->mpFont) output << " font-metrics=" << box->mpFont->GetWidth() << ',' << box->mpFont->GetHeight()
                                << ',' << box->mpFont->GetAscent() << ',' << box->mpFont->GetBaselinePos();
        if (box->mpMaterial) for (u32 color = 0; color < 2; ++color) {
            const auto value = box->mpMaterial->GetTevColor(color);
            output << " tev" << color << '=' << value.r << ',' << value.g << ',' << value.b << ',' << value.a;
        }
        if (const auto* processor = dynamic_cast<const CustomTagProcessor*>(box->mpTagProcessor))
            output << " shadow=" << processor->mIsShadow << " text=" << processor->mIsText;
        output << " units=" << std::hex;
        for (u32 i = 0; i < box->mTextLen; ++i) output << u32(box->mTextBuf[i]) << ',';
        output << std::dec << '\n';
    }
}
#endif
void animate_native_pane(const nw4r::lyt::Pane* pane) {
    if (const auto* record = dynamic_cast<const NativePaneIdentity*>(pane); record && record->index != kDetachedPane)
        record->owner.animate_pane(record->index);
}
bool synchronize_native_pane(const nw4r::lyt::Pane* pane) {
    if (const auto* record = dynamic_cast<const NativePaneIdentity*>(pane); record && record->index != kDetachedPane) {
        record->owner.synchronize();
        return true;
    }
    return false;
}
void Nw4rLayoutRecords::require_mutable_resource_graph(std::string_view operation) const {
    if (_state->published && !_state->retiring)
        aurora::throw_host_exception<std::logic_error>(std::string(operation) + " requires matching renderer resource topology support");
}
void validate_native_pane_hierarchy_change(const nw4r::lyt::Pane* parent, const nw4r::lyt::Pane* child) {
    if (const auto* record = dynamic_cast<const NativePaneIdentity*>(parent)) record->owner.require_mutable_resource_graph("Reparenting a published NW4R pane");
    if (const auto* record = dynamic_cast<const NativePaneIdentity*>(child)) record->owner.require_mutable_resource_graph("Reparenting a published NW4R pane");
}
void validate_native_pane_rename(const nw4r::lyt::Pane* pane, const char* name) {
    if (const auto* record = dynamic_cast<const NativePaneIdentity*>(pane); record && std::strncmp(pane->mName, name, 16) != 0)
        record->owner.require_mutable_resource_graph("Renaming a published NW4R pane");
}
void validate_native_group_append(const nw4r::lyt::Group* group) {
    if (const auto* record = dynamic_cast<const NativeGroupRecord*>(group)) record->owner.require_mutable_resource_graph("Appending to a published NW4R resource group");
}
} // namespace smgpc::layout
