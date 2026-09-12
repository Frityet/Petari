#pragma once

#include <revolution.h>
#include <memory>

namespace nw4r {
    namespace ut {
        class Font;
    };

    namespace lyt {
        struct HostTextureResourceState;
        typedef u32 ResType;

        class ResourceAccessor {
        public:
            ResourceAccessor();

            virtual ~ResourceAccessor();
            virtual void* GetResource(ResType, const char*, u32* = 0) = 0;
            virtual ut::Font* GetFont(const char*);
            // Native resource accessors transfer image ownership to SDK materials
            // independently of the accessor object's own lifetime.
            virtual std::shared_ptr<const HostTextureResourceState> GetHostTextureResourceState(const char*) { return {}; }
        };
    };
};
