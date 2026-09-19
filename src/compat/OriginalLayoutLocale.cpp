#include "Game/Screen/LayoutManager.hpp"
#include "Game/System/Language.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include <nw4r/lyt/pane.h>
#include <cstring>

namespace {
    namespace Local {
        template < int N >
        class BitFlag {
        public:
            BitFlag() {
                MR::zeroMemory(mFlags, sizeof(mFlags));
            }

            virtual ~BitFlag() {}

            virtual void onBit(int index) {
                mFlags[index / 32] |= 1 << (index % 32);
            }

            virtual void offBit(int index) {
                mFlags[index / 32] &= ~(1 << (index % 32));
            }

            virtual bool isTrue(int index) const {
                return (mFlags[index / 32] & (1 << (index % 32))) != 0;
            }

            virtual bool isAnythingTrue() const {
                for (int i = 0; i < N / 32; i++) {
                    if (mFlags[i] != 0) {
                        return true;
                    }
                }
                return false;
            }

            u32 mFlags[N / 32];
        };
    };  // namespace Local
};  // namespace

void LayoutManager::removeUnnecessaryPanes(nw4r::lyt::Pane* pPane) {
    Local::BitFlag< 128 > languagePanes;
    Local::BitFlag< 128 > currentLanguagePanes;
    nw4r::lyt::PaneList& children = pPane->mChildList;
    s32 index = -1;
    for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
        ++index;
        const char* name = (*it).mName;
        u32 length = strlen(name);
        if (length < 4) {
            continue;
        }
        const char* suffix = name + length - 4;
        for (u32 language = 0; language < MR::getLanguageNum(); ++language) {
            if (strncmp(suffix, MR::getLanguagePrefixByIndex(language), 4) == 0) {
                languagePanes.onBit(index);
                if (strncmp(suffix, MR::getCurrentLanguagePrefix(), 4) == 0) {
                    currentLanguagePanes.onBit(index);
                }
                break;
            }
        }
    }

    if (languagePanes.isAnythingTrue()) {
        index = 0;
        nw4r::lyt::PaneList::Iterator it = children.GetBeginIter();
        if (currentLanguagePanes.isAnythingTrue()) {
            while (it != children.GetEndIter()) {
                nw4r::lyt::PaneList::Iterator current = it++;
                if (currentLanguagePanes.isTrue(index)) {
                    char name[20];
                    const char* source = (*current).mName;
                    u32 length = strlen(source) - 4;
                    strncpy(name, source, length);
                    name[length] = '\0';
                    (*current).SetName(name);
                } else {
                    pPane->RemoveChild(&*current);
                }
                ++index;
            }
        } else {
            while (it != children.GetEndIter()) {
                nw4r::lyt::PaneList::Iterator current = it++;
                if (languagePanes.isTrue(index)) {
                    pPane->RemoveChild(&*current);
                }
                ++index;
            }
        }
    } else {
        for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
            removeUnnecessaryPanes(&*it);
        }
    }
}
