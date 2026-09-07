#pragma once

#include <cstdint>
#include <string>

#include "resource/TplTexture.hpp"

namespace nw4r::lyt {

    class TexMap final {
    public:
        TexMap(std::string name, smgpc::resource::DecodedTexture image, std::uint8_t wrapS, std::uint8_t wrapT, std::uint8_t minFilter,
               std::uint8_t magFilter);

        [[nodiscard]] const std::string &name() const;
        [[nodiscard]] const smgpc::resource::DecodedTexture &image() const;
        [[nodiscard]] std::uint8_t wrap_s() const;
        [[nodiscard]] std::uint8_t wrap_t() const;
        [[nodiscard]] std::uint8_t min_filter() const;
        [[nodiscard]] std::uint8_t mag_filter() const;

    private:
        std::string _name;
        smgpc::resource::DecodedTexture _image;
        std::uint8_t _wrap_s = 0U;
        std::uint8_t _wrap_t = 0U;
        std::uint8_t _min_filter = 0U;
        std::uint8_t _mag_filter = 0U;
    };

}  // namespace nw4r::lyt
