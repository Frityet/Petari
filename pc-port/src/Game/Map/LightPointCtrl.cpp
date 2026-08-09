#include "Game/Map/LightPointCtrl.hpp"

#include "Game/Map/LightFunction.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/Color.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <cmath>
#include <limits>
#include <memory>

namespace {
    constexpr auto cNoBlendStep = s32{-1};
    constexpr auto cDefaultBlendDuration = s32{30};
    constexpr auto cPointLightRadius = 15.0F;
    constexpr auto cMinimumBrightness = 0.95F;
    constexpr auto cMaximumBrightness = 0.999999F;
    constexpr auto cAbsentBrightness = 0.001F;

    [[nodiscard]] f32 clampBrightness(f32 brightness) {
        // PPC's unordered compare clears both LT and GT. The retail bge/ble
        // sequence therefore reaches the raw-value store for NaN instead of
        // selecting either endpoint.
        if (std::isnan(brightness)) {
            return brightness;
        }
        if (brightness < cMinimumBrightness) {
            return cMinimumBrightness;
        }
        if (brightness <= cMaximumBrightness) {
            return brightness;
        }
        return cMaximumBrightness;
    }

    [[nodiscard]] f32 interpolate(f32 rate, f32 from, f32 to) {
        return from + ((to - from) * rate);
    }

    [[nodiscard]] u8 truncateColorChannel(f32 value) {
        // PowerPC fctiwz produces the integer-indefinite value for NaN and
        // overflow; its low byte is zero when blendColor stores the channel.
        if (!std::isfinite(value) || value < static_cast< f32 >(std::numeric_limits< s32 >::min()) ||
            value > static_cast< f32 >(std::numeric_limits< s32 >::max())) {
            return 0U;
        }
        return static_cast< u8 >(static_cast< s32 >(value));
    }

    void blendColor(_GXColor* pOut, const _GXColor& rFrom, const _GXColor& rTo, f32 rate) {
        pOut->r = truncateColorChannel(interpolate(rate, static_cast< f32 >(rFrom.r), static_cast< f32 >(rTo.r)));
        pOut->g = truncateColorChannel(interpolate(rate, static_cast< f32 >(rFrom.g), static_cast< f32 >(rTo.g)));
        pOut->b = truncateColorChannel(interpolate(rate, static_cast< f32 >(rFrom.b), static_cast< f32 >(rTo.b)));
        pOut->a = truncateColorChannel(interpolate(rate, static_cast< f32 >(rFrom.a), static_cast< f32 >(rTo.a)));
    }

    void blendVec(TVec3f* pOut, const TVec3f& rFrom, const TVec3f& rTo, f32 rate) {
        pOut->set(interpolate(rate, rFrom.x, rTo.x), interpolate(rate, rFrom.y, rTo.y), interpolate(rate, rFrom.z, rTo.z));
    }

    [[nodiscard]] bool isLiveGeneration(const LiveActor* actor, std::uint64_t generation) noexcept {
        return actor != nullptr && generation != 0U && smgpc::compat::name_obj_runtime_generation(actor) == generation;
    }

}  // namespace

LightPointCtrl::LightPointCtrl() {
    auto current = std::make_unique< PointLightInfo >();
    auto requested = std::make_unique< PointLightInfo >();
    auto transitionStart = std::make_unique< PointLightInfo >();

    clearPointLight(current.get());
    clearPointLight(requested.get());
    clearPointLight(transitionStart.get());

    _14 = current.release();
    _18 = requested.release();
    _1C = transitionStart.release();
}

LightPointCtrl::~LightPointCtrl() {
    delete _1C;
    delete _18;
    delete _14;
    _1C = nullptr;
    _18 = nullptr;
    _14 = nullptr;
}

void LightPointCtrl::loadPointLight() {
    LightFunction::loadPointLightInfo(_14);
}

void LightPointCtrl::update() {
    if (_0 == cNoBlendStep) {
        if (_10 != nullptr && !isLiveGeneration(_10, _10Generation)) {
            _10 = nullptr;
            _10Generation = 0U;
            clearPointLight(_18);
        }
        _8 = _10;
        _8Generation = _10Generation;
        tryBlendStart();
    }

    updatePointLight();

    if (_0 == cNoBlendStep) {
        _C = _8;
        _CGeneration = _8Generation;
        *_1C = *_18;
        _10 = nullptr;
        _10Generation = 0U;
    }
}

void LightPointCtrl::requestPointLight(const LiveActor* pActor, TVec3f position, Color8 color, f32 brightness, s32 duration) {
    if (_0 != cNoBlendStep || pActor == nullptr || !isUpdateCandidateActor(pActor)) {
        return;
    }

    const auto generation = smgpc::compat::name_obj_runtime_generation(pActor);
    if (generation == 0U) {
        return;
    }

    _10 = pActor;
    _10Generation = generation;
    _18->mPosition = position;
    _18->mColor = static_cast< GXColor >(color);
    _18->mBrightness = clampBrightness(brightness);
    _18->mRadius = cPointLightRadius;
    _18->mDistAttnFn = GX_DA_STEEP;
    _4 = duration >= 0 ? duration : cDefaultBlendDuration;
}

void LightPointCtrl::updatePointLight() {
    if (_8 == nullptr && _C != nullptr) {
        clearPointLight(_18);
    }

    if (_8 == nullptr && _0 == cNoBlendStep) {
        clearPointLight(_14);
        return;
    }

    if (_0 == cNoBlendStep) {
        *_14 = *_18;
        return;
    }

    // PPC's duration-zero path feeds the cosine helper its integer-conversion
    // zero phase. Keep the one inclusive transition step, but do not turn a
    // retail duration of zero into a one-frame duration or an immediate snap.
    const auto rate = _4 == 0 ? 0.0F : MR::getEaseInOutValue(static_cast< f32 >(_0) / static_cast< f32 >(_4), 0.0F, 1.0F, 1.0F);
    blendPointLight(_14, *_1C, *_18, rate);

    if (_0 < _4) {
        ++_0;
    } else {
        _0 = cNoBlendStep;
    }
}

void LightPointCtrl::clearPointLight(PointLightInfo* pInfo) {
    if (pInfo == nullptr) {
        return;
    }

    pInfo->mPosition.zero();
    pInfo->mColor = _GXColor{0U, 0U, 0U, 0xFFU};
    pInfo->mBrightness = cAbsentBrightness;
    pInfo->mRadius = cPointLightRadius;
    pInfo->mDistAttnFn = GX_DA_STEEP;
    _4 = cDefaultBlendDuration;
}

void LightPointCtrl::blendPointLight(PointLightInfo* pOut, const PointLightInfo& rFrom, const PointLightInfo& rTo, f32 rate) {
    if (_8 == nullptr) {
        pOut->mPosition = rFrom.mPosition;
    } else if (_C == nullptr) {
        pOut->mPosition = rTo.mPosition;
    } else {
        blendVec(&pOut->mPosition, rFrom.mPosition, rTo.mPosition, rate);
    }

    const auto fromBrightness = _C == nullptr ? cMinimumBrightness : rFrom.mBrightness;
    const auto toBrightness = _8 == nullptr ? cMinimumBrightness : rTo.mBrightness;
    pOut->mBrightness = interpolate(rate, fromBrightness, toBrightness);
    blendColor(&pOut->mColor, rFrom.mColor, rTo.mColor, rate);
    pOut->mRadius = interpolate(rate, rFrom.mRadius, rTo.mRadius);
}

bool LightPointCtrl::tryBlendStart() {
    if (_C == _8 && _CGeneration == _8Generation) {
        return false;
    }

    _0 = 0;
    return true;
}

bool LightPointCtrl::isUpdateCandidateActor(const LiveActor* pActor) const {
    if (pActor == nullptr || smgpc::compat::name_obj_runtime_generation(pActor) == 0U) {
        return false;
    }
    if (!isLiveGeneration(_10, _10Generation)) {
        return true;
    }

    return MR::calcDistanceToPlayer(pActor) < MR::calcDistanceToPlayer(_10);
}
