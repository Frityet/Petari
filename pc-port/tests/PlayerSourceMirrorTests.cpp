#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    constexpr auto cPlayerSources = std::array{
        std::string_view{"DrawAdaptor"},
        std::string_view{"FireMarioBall"},
        std::string_view{"J3DModelX"},
        std::string_view{"JetTurtleShadow"},
        std::string_view{"MarineSnow"},
        std::string_view{"Mario"},
        std::string_view{"Mario2D"},
        std::string_view{"MarioAccess"},
        std::string_view{"MarioActor"},
        std::string_view{"MarioActorCamera"},
        std::string_view{"MarioActorClap"},
        std::string_view{"MarioActorDefensiveMsg"},
        std::string_view{"MarioActorDraw"},
        std::string_view{"MarioActorEye"},
        std::string_view{"MarioActorGameOver"},
        std::string_view{"MarioActorGravity"},
        std::string_view{"MarioActorHand"},
        std::string_view{"MarioActorInit"},
        std::string_view{"MarioActorMatrix"},
        std::string_view{"MarioActorMorph"},
        std::string_view{"MarioActorOffensiveMsg"},
        std::string_view{"MarioActorPad"},
        std::string_view{"MarioActorParts"},
        std::string_view{"MarioActorPunch"},
        std::string_view{"MarioActorRush"},
        std::string_view{"MarioActorRushMsg"},
        std::string_view{"MarioActorSensor"},
        std::string_view{"MarioActorShadow"},
        std::string_view{"MarioActorSpecialDraw"},
        std::string_view{"MarioActorTakeMsg"},
        std::string_view{"MarioActorWipe"},
        std::string_view{"MarioAnimationEfx"},
        std::string_view{"MarioAnimator"},
        std::string_view{"MarioBee"},
        std::string_view{"MarioBlown"},
        std::string_view{"MarioBump"},
        std::string_view{"MarioClimb"},
        std::string_view{"MarioCollision"},
        std::string_view{"MarioConst"},
        std::string_view{"MarioDamage"},
        std::string_view{"MarioDamageCrush"},
        std::string_view{"MarioDamageFreeze"},
        std::string_view{"MarioDamageParalyze"},
        std::string_view{"MarioDamageStun"},
        std::string_view{"MarioEffect"},
        std::string_view{"MarioEnforce"},
        std::string_view{"MarioFaint"},
        std::string_view{"MarioFlip"},
        std::string_view{"MarioFlow"},
        std::string_view{"MarioFoo"},
        std::string_view{"MarioFpView"},
        std::string_view{"MarioFrontStep"},
        std::string_view{"MarioHang"},
        std::string_view{"MarioHolder"},
        std::string_view{"MarioInit"},
        std::string_view{"MarioJump"},
        std::string_view{"MarioMagic"},
        std::string_view{"MarioMapCode"},
        std::string_view{"MarioMessenger"},
        std::string_view{"MarioModule"},
        std::string_view{"MarioMove"},
        std::string_view{"MarioMove25D"},
        std::string_view{"MarioMove2D"},
        std::string_view{"MarioMoveSphere"},
        std::string_view{"MarioNullBck"},
        std::string_view{"MarioParts"},
        std::string_view{"MarioPress"},
        std::string_view{"MarioRabbit"},
        std::string_view{"MarioRecovery"},
        std::string_view{"MarioSearchLight"},
        std::string_view{"MarioShadow"},
        std::string_view{"MarioSideStep"},
        std::string_view{"MarioSkate"},
        std::string_view{"MarioSlider"},
        std::string_view{"MarioSlip"},
        std::string_view{"MarioSlope"},
        std::string_view{"MarioSound"},
        std::string_view{"MarioSpecial"},
        std::string_view{"MarioSpin"},
        std::string_view{"MarioState"},
        std::string_view{"MarioStep"},
        std::string_view{"MarioStick"},
        std::string_view{"MarioSukekiyo"},
        std::string_view{"MarioSwim"},
        std::string_view{"MarioSwimDamage"},
        std::string_view{"MarioTalk"},
        std::string_view{"MarioTask"},
        std::string_view{"MarioTeresa"},
        std::string_view{"MarioWait"},
        std::string_view{"MarioWalk"},
        std::string_view{"MarioWall"},
        std::string_view{"MarioWarp"},
        std::string_view{"MatrixControl"},
        std::string_view{"ModelHolder"},
        std::string_view{"RushEndInfo"},
        std::string_view{"TornadoMario"},
    };

    constexpr auto cPlayerHeaders = std::array{
        std::string_view{"DLchanger"},
        std::string_view{"DrawAdaptor"},
        std::string_view{"FireMarioBall"},
        std::string_view{"J3DModelX"},
        std::string_view{"JetTurtleShadow"},
        std::string_view{"MarineSnow"},
        std::string_view{"Mario"},
        std::string_view{"MarioAbyssDamage"},
        std::string_view{"MarioAccess"},
        std::string_view{"MarioActor"},
        std::string_view{"MarioAnimator"},
        std::string_view{"MarioAnimatorData"},
        std::string_view{"MarioBlown"},
        std::string_view{"MarioBump"},
        std::string_view{"MarioClimb"},
        std::string_view{"MarioConst"},
        std::string_view{"MarioCrush"},
        std::string_view{"MarioDamage"},
        std::string_view{"MarioDarkDamage"},
        std::string_view{"MarioEffect"},
        std::string_view{"MarioFaint"},
        std::string_view{"MarioFireDamage"},
        std::string_view{"MarioFireDance"},
        std::string_view{"MarioFireRun"},
        std::string_view{"MarioFlip"},
        std::string_view{"MarioFlow"},
        std::string_view{"MarioFoo"},
        std::string_view{"MarioFpView"},
        std::string_view{"MarioFreeze"},
        std::string_view{"MarioFrontStep"},
        std::string_view{"MarioHang"},
        std::string_view{"MarioHolder"},
        std::string_view{"MarioMagic"},
        std::string_view{"MarioMapCode"},
        std::string_view{"MarioMessenger"},
        std::string_view{"MarioModule"},
        std::string_view{"MarioMove"},
        std::string_view{"MarioMoveSphere"},
        std::string_view{"MarioNullBck"},
        std::string_view{"MarioParalyze"},
        std::string_view{"MarioParts"},
        std::string_view{"MarioRabbit"},
        std::string_view{"MarioRecovery"},
        std::string_view{"MarioSearchLight"},
        std::string_view{"MarioShadow"},
        std::string_view{"MarioSideStep"},
        std::string_view{"MarioSkate"},
        std::string_view{"MarioSlider"},
        std::string_view{"MarioState"},
        std::string_view{"MarioStep"},
        std::string_view{"MarioStick"},
        std::string_view{"MarioStun"},
        std::string_view{"MarioSukekiyo"},
        std::string_view{"MarioSwim"},
        std::string_view{"MarioTalk"},
        std::string_view{"MarioTeresa"},
        std::string_view{"MarioWait"},
        std::string_view{"MarioWall"},
        std::string_view{"MarioWarp"},
        std::string_view{"MatrixControl"},
        std::string_view{"ModelHolder"},
        std::string_view{"RushEndInfo"},
        std::string_view{"TornadoMario"},
    };

    static_assert(cPlayerSources.size() == 96);
    static_assert(cPlayerHeaders.size() == 63);

    constexpr auto cDebugDivergentSources = std::array{
        std::string_view{"MarioActor"},
        std::string_view{"MarioActorDraw"},
        std::string_view{"MarioAnimator"},
    };

    constexpr auto cDebugBegin = std::string_view{"#if !defined(NDEBUG)  // SMGPC_DEBUG_DIVERGENCE"};
    constexpr auto cRetailBegin = std::string_view{"#else  // SMGPC_RETAIL_SOURCE"};
    constexpr auto cDebugEnd = std::string_view{"#endif  // SMGPC_DEBUG_DIVERGENCE"};

    [[nodiscard]] std::string readFile(const std::string &path) {
        auto stream = std::ifstream(path, std::ios::binary);
        if (!stream.is_open()) {
            throw std::runtime_error("could not open source-boundary file: " + path);
        }

        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

    [[nodiscard]] bool permitsDebugDivergence(const std::string_view name) {
        for (const auto permitted : cDebugDivergentSources) {
            if (name == permitted) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] std::size_t lineStart(const std::string &source, const std::size_t position) {
        const auto newline = source.rfind('\n', position);
        return newline == std::string::npos ? 0 : newline + 1;
    }

    [[nodiscard]] std::size_t lineEnd(const std::string &source, const std::size_t position) {
        const auto newline = source.find('\n', position);
        return newline == std::string::npos ? source.size() : newline + 1;
    }

    [[nodiscard]] std::string selectRetailSource(const std::string &source, std::size_t *guardCount) {
        auto selected = std::string{};
        auto cursor = std::size_t{};

        while (true) {
            const auto debugMarker = source.find(cDebugBegin, cursor);
            if (debugMarker == std::string::npos) {
                selected.append(source, cursor, std::string::npos);
                break;
            }

            const auto blockStart = lineStart(source, debugMarker);
            const auto retailMarker = source.find(cRetailBegin, debugMarker + cDebugBegin.size());
            const auto endMarker = retailMarker == std::string::npos
                                       ? std::string::npos
                                       : source.find(cDebugEnd, retailMarker + cRetailBegin.size());
            if (retailMarker == std::string::npos || endMarker == std::string::npos) {
                throw std::runtime_error("malformed SMGPC debug-divergence guard");
            }

            const auto retailStart = lineEnd(source, retailMarker);
            const auto retailEnd = lineStart(source, endMarker);
            selected.append(source, cursor, blockStart - cursor);
            selected.append(source, retailStart, retailEnd - retailStart);
            cursor = lineEnd(source, endMarker);
            ++*guardCount;
        }

        return selected;
    }

    void requireMirrored(const std::string_view name, const std::string &rootPath, const std::string &portPath) {
        const auto root = readFile(rootPath);
        const auto port = readFile(portPath);
        if (root == port) {
            return;
        }

        if (!permitsDebugDivergence(name)) {
            throw std::runtime_error("PC Game mirror is not byte-identical: " + portPath);
        }

        auto guardCount = std::size_t{};
        if (selectRetailSource(port, &guardCount) != root || guardCount == 0) {
            throw std::runtime_error("PC debug mirror does not retain byte-identical retail branches: " + portPath);
        }
    }

}  // namespace

int main() {
    auto failures = 0;
    const auto gameXmake = readFile("src/Game/xmake.lua");

    for (const auto name : cPlayerSources) {
        try {
            const auto file = std::string(name) + ".cpp";
            requireMirrored(name, "../src/Game/Player/" + file, "src/Game/Player/" + file);

            const auto exclusion = std::string{"\"Player/"} + file + "\"";
            if (name == "MarioHolder") {
                if (gameXmake.find(exclusion) != std::string::npos) {
                    throw std::runtime_error("production MarioHolder source is unexpectedly excluded");
                }
            } else if (gameXmake.find(exclusion) == std::string::npos) {
                throw std::runtime_error("provider-incomplete Player source is not explicitly excluded: " + file);
            }
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << name << ".cpp: " << error.what() << '\n';
        }
    }

    for (const auto name : cPlayerHeaders) {
        try {
            const auto file = std::string(name) + ".hpp";
            requireMirrored(name, "../include/Game/Player/" + file, "src/Game/Player/" + file);
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << name << ".hpp: " << error.what() << '\n';
        }
    }

    if (failures == 0) {
        std::cout << "Player source mirror passed: 96/96 retail source branches exact, 63/63 headers exact, "
                     "1 production TU and 95 explicit exclusions\n";
    }

    return failures == 0 ? 0 : 1;
}
