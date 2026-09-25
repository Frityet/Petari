#include "nw4r/ut/Font.h"
#include <aurora/allocation.hpp>

namespace nw4r {
    namespace ut {
        void Font::InitReaderFunc(FontEncoding encode) {
            switch (encode) {
            case FONT_ENCODING_UTF8: {
                mReadFunc = &CharStrmReader::ReadNextCharUTF8;
                break;
            }

            case FONT_ENCODING_UTF16: {
                mReadFunc = &CharStrmReader::ReadNextCharUTF16;
                break;
            }

            case FONT_ENCODING_SJIS: {
                mReadFunc = &CharStrmReader::ReadNextCharSJIS;
                break;
            }

            case FONT_ENCODING_CP1252:
            default: {
                mReadFunc = &CharStrmReader::ReadNextCharCP1252;
                break;
            }
            }
        }
    };  // namespace ut
};  // namespace nw4r

namespace nw4r::ut {
    Font::Font() {
        aurora::allocation::HostAllocationScope host;
        mHostResourceState = std::make_shared<HostFontResourceState>();
    }

    Font::~Font() = default;

    std::weak_ptr<const HostFontResourceState> Font::GetHostResourceState() const noexcept {
        return mHostResourceState;
    }

}  // namespace nw4r::ut
