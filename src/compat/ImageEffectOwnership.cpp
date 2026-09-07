#include "compat/ImageEffectOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Screen/BloomEffect.hpp"
#include "Game/Screen/BloomEffectSimple.hpp"
#include "Game/Screen/DepthOfFieldBlur.hpp"
#include "Game/Screen/ImageEffectDirector.hpp"
#include "Game/Screen/ImageEffectResource.hpp"
#include "Game/Screen/ImageEffectState.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Screen/ScreenBlurEffect.hpp"
#include <aurora/exception.hpp>

#include <array>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        constexpr std::array ids{SceneObj_ImageEffectSystemHolder, SceneObj_BloomEffect,
            SceneObj_BloomEffectSimple, SceneObj_ScreenBlurEffect, SceneObj_DepthOfFieldBlur,
            SceneObj_WaterAreaHolder};
    }

    class ImageEffectOwnership::Impl final {
    public:
        explicit Impl(SceneObjHolder& value) : holder(value) {}
        struct Record {
            NameObj* root = nullptr;
            JutTextureOwnership textures;
            bool prepared = false;
        };
        Record* find(int id) noexcept {
            for (std::size_t i = 0; i < ids.size(); ++i) if (ids[i] == id) return &records[i];
            return nullptr;
        }
        SceneObjHolder& holder;
        std::array<Record, ids.size()> records;
        ImageEffectResource* resource = nullptr;
        std::array<ImageEffectState*, 5> states{};
        Mtx* bloom_first = nullptr;
        Mtx* bloom_second = nullptr;
        OceanBowl** bowls = nullptr;
        OceanRing** rings = nullptr;
        OceanSphere** spheres = nullptr;
        WhirlPool** whirls = nullptr;
        WhirlPoolAccelerator** accelerators = nullptr;
    };

    ImageEffectOwnership::ImageEffectOwnership(SceneObjHolder& holder) {
        JkrHostAllocationScope host;
        _impl = std::make_unique<Impl>(holder);
    }
    ImageEffectOwnership::~ImageEffectOwnership() {
        prepare_retirement();
        reclaim_prepared();
    }
    bool ImageEffectOwnership::handles(int id) noexcept {
        for (auto candidate : ids) if (candidate == id) return true;
        return false;
    }
    NameObj* ImageEffectOwnership::construct(int id) {
        if (!scene::current_scene_allocation_domain())
            aurora::throw_host_exception<std::logic_error>("Original image effects require the scene's actual Game allocation domain");
        switch (id) {
        case SceneObj_ImageEffectSystemHolder: return new ImageEffectSystemHolder();
        case SceneObj_BloomEffect: return new BloomEffect("ブルーム");
        case SceneObj_BloomEffectSimple: return new BloomEffectSimple();
        case SceneObj_ScreenBlurEffect: return new ScreenBlurEffect("画面ブラー");
        case SceneObj_DepthOfFieldBlur: return new DepthOfFieldBlur("被写界深度ブラー");
        case SceneObj_WaterAreaHolder: return new WaterAreaHolder();
        default: aurora::throw_host_exception<std::invalid_argument>("SceneObj is not an original image-effect owner");
        }
    }
    void ImageEffectOwnership::capture_shared_textures(JutTextureConstructionScope& scope) noexcept {
        auto* record = _impl->find(SceneObj_ImageEffectSystemHolder);
        if (!record->root || !_impl->resource) return;
        auto& resource = *_impl->resource;
        for (auto* texture : {resource._0, resource._4, resource._8, resource._C, resource._10,
                             resource._14, resource._18, resource._1C, resource._20})
            record->textures.adopt(scope, texture);
    }
    void ImageEffectOwnership::capture(int id, NameObj& object, JutTextureConstructionScope& scope) noexcept {
        auto* record = _impl->find(id);
        if (!record) return;
        record->root = &object;
        switch (id) {
        case SceneObj_ImageEffectSystemHolder: {
            auto& holder = static_cast<ImageEffectSystemHolder&>(object);
            _impl->resource = holder.mResource;
            auto& director = *holder.mDirector;
            _impl->states = {director.mStateNull, director.mStateBloomNormal, director.mStateBloomSimple,
                             director.mStateScreenBlur, director.mStateDepthOfField};
            break;
        }
        case SceneObj_BloomEffect: {
            auto& bloom = static_cast<BloomEffect&>(object);
            _impl->bloom_first = bloom._48;
            _impl->bloom_second = bloom._4C;
            break;
        }
        case SceneObj_WaterAreaHolder: {
            auto& water = static_cast<WaterAreaHolder&>(object);
            _impl->bowls = water.mOceanBowls;
            _impl->rings = water.mOceanRings;
            _impl->spheres = water.mOceanSpheres;
            _impl->whirls = water.mWhirlPools;
            _impl->accelerators = water.mWhirlPoolAccelerators;
            break;
        }
        default: break;
        }
        // These textures are published directly in the shared resource even
        // when a later allocation in the requesting effect constructor fails.
        capture_shared_textures(scope);
        record->textures.adopt_all(scope);
    }
    void ImageEffectOwnership::prepare_rollback(NameObjRuntimeRegistrationMarker marker) noexcept {
        for (auto& record : _impl->records)
            if (record.root && name_obj_runtime_object_was_registered_since(record.root, marker)) record.prepared = true;
    }
    void ImageEffectOwnership::prepare_retirement() noexcept {
        for (auto& record : _impl->records) if (record.root) record.prepared = true;
    }
    void ImageEffectOwnership::reclaim_prepared() noexcept {
        JkrHostAllocationScope host;
        for (std::size_t i = _impl->records.size(); i > 0; --i) {
            auto& record = _impl->records[i - 1];
            if (!record.prepared) continue;
            switch (ids[i - 1]) {
            case SceneObj_ImageEffectSystemHolder:
                for (auto*& state : _impl->states) { delete state; state = nullptr; }
                delete _impl->resource;
                _impl->resource = nullptr;
                break;
            case SceneObj_BloomEffect:
                delete[] _impl->bloom_first;
                delete[] _impl->bloom_second;
                _impl->bloom_first = _impl->bloom_second = nullptr;
                break;
            case SceneObj_WaterAreaHolder:
                delete[] _impl->bowls;
                delete[] _impl->rings;
                delete[] _impl->spheres;
                delete[] _impl->whirls;
                delete[] _impl->accelerators;
                _impl->bowls = nullptr; _impl->rings = nullptr; _impl->spheres = nullptr;
                _impl->whirls = nullptr; _impl->accelerators = nullptr;
                break;
            default: break;
            }
            record.textures.clear();
            record.root = nullptr;
            record.prepared = false;
        }
    }
}
