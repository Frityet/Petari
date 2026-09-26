#pragma once

#include <cstddef>

#include "nw4r/ut/ResFontBase.h"

namespace nw4r {
    namespace ut {
        struct BinaryFileHeader;

        class ResFont : public detail::ResFontBase {
        public:
            ResFont();
            virtual ~ResFont();

            bool SetResource(void* pBuffer);
            bool SetResource(void* pBuffer, std::size_t bufferSize);
            void RemoveResource();

            static FontInformation* Rebuild(BinaryFileHeader* pHeader);
        };
    };  // namespace ut
};  // namespace nw4r
