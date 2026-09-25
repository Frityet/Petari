#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/Language.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <array>
#include <stdexcept>
#include <string_view>
#include <revolution/sc.h>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void verify_language() {
    auto& holder = *SingletonHolder<GameSystem>::get()->mObjHolder;
    require(MR::getLanguage() == holder.mLanguage && MR::getDecidedLanguageFromIPL() == holder.mLanguage &&
                MR::getLanguageFromIPL() == SCGetLanguage(),
            "the original GameSystem object holder retains the language decided from real console settings");
    constexpr std::array languages{
        Language{0x10, "JpJapanese"}, Language{0x21, "UsEnglish"}, Language{0x24, "UsSpanish"},
        Language{0x23, "UsFrench"}, Language{0x01, "EuEnglish"}, Language{0x04, "EuSpanish"},
        Language{0x03, "EuFrench"}, Language{0x02, "EuGerman"}, Language{0x05, "EuItalian"},
        Language{0x06, "EuDutch"}, Language{0x37, "CnSimpChinese"}, Language{0x49, "KrKorean"}
    };
    require(MR::getLanguageNum() == languages.size(), "all twelve original regional languages remain available");
    struct Restore { u32& field; u32 value; ~Restore() { field = value; } } restore{holder.mLanguage, holder.mLanguage};
    for (u32 i = 0; i < languages.size(); ++i) {
        const auto& language = languages[i];
        holder.mLanguage = language.mId;
        const std::string_view prefix(language.mName);
        require(MR::getLanguage() == language.mId && MR::getLanguageFromIPL() == (language.mId & 0xf) &&
                    MR::getLanguagePrefixByIndex(i) == prefix && MR::getCurrentLanguagePrefix() == prefix &&
                    MR::getCurrentRegionPrefix() == prefix.substr(0, 2),
                "language/region facade reads each original mapping directly from the actual owner field");
    }
}
}
int main() { return smgpc::test::run_stage_resource_process("original-language", verify_language); }
