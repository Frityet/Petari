#include "compat/LanguageOwnership.hpp"
#include "Game/System/Language.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/Util/SingletonHolder.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    template<class Exception, class Function> void rejects(Function function) {
        try { function(); } catch (const Exception&) { return; }
        throw std::runtime_error("Language operation did not reject its missing or invalid owner");
    }
}

int main() {
    try {
        require(SingletonHolder<GameSystem>::get() == nullptr, "Fixture starts without a process owner");
        rejects<std::logic_error>([] { MR::getLanguage(); });
        for (u32 index = 0; index < MR::getLanguageNum(); ++index) {
            const std::string_view prefix = MR::getLanguagePrefixByIndex(index);
            smgpc::compat::LanguageOwnership owner(prefix);
            require(MR::getCurrentLanguagePrefix() == prefix, "Explicit resource language retains the original ID/prefix mapping");
            require(prefix.substr(0, 2) == MR::getCurrentRegionPrefix(), "Original region helper uses the retained language");
            const auto language = MR::getLanguage();
            {
                smgpc::compat::LanguageOwnership nested(prefix == "KrKorean" ? "UsEnglish" : "KrKorean");
                require(MR::getLanguage() != language, "Nested explicit language owns its actual original ID");
            }
            require(MR::getLanguage() == language, "Retiring nested language restores the prior owner");
            rejects<std::invalid_argument>([] { smgpc::compat::LanguageOwnership invalid("invented"); });
            require(MR::getLanguage() == language, "Failed construction preserves the live language owner");
        }
        rejects<std::logic_error>([] { MR::getLanguage(); });
        {
            smgpc::compat::LanguageOwnership owner("KrKorean");
            SingletonHolder<GameSystem>::init();
            // A real process that has not constructed its object holder cannot
            // borrow unrelated standalone language state during initialization.
            rejects<std::logic_error>([] { MR::getLanguage(); });
            delete SingletonHolder<GameSystem>::release();
            require(std::string_view(MR::getCurrentLanguagePrefix()) == "KrKorean",
                    "Standalone ownership resumes after actual process retirement");
        }
        rejects<std::logic_error>([] { MR::getLanguage(); });
        std::cout << "PASS explicit original language mappings, nested lifetime, failed-init rollback and process-owner precedence\n";
        return 0;
    } catch (const std::exception& error) {
        if (auto* system = SingletonHolder<GameSystem>::release()) delete system;
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
