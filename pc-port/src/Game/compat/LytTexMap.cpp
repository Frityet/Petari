#include "Game/compat/LytTexMap.hpp"

#include <utility>

namespace nw4r::lyt {

TexMap::TexMap(std::string name, smgpc::game::DecodedTexture image, std::uint8_t wrapS, std::uint8_t wrapT, std::uint8_t minFilter,
               std::uint8_t magFilter)
    : _name(std::move(name)), _image(std::move(image)), _wrap_s(wrapS), _wrap_t(wrapT), _min_filter(minFilter), _mag_filter(magFilter) {
}

const std::string &TexMap::name() const {
    return _name;
}

const smgpc::game::DecodedTexture &TexMap::image() const {
    return _image;
}

std::uint8_t TexMap::wrap_s() const {
    return _wrap_s;
}

std::uint8_t TexMap::wrap_t() const {
    return _wrap_t;
}

std::uint8_t TexMap::min_filter() const {
    return _min_filter;
}

std::uint8_t TexMap::mag_filter() const {
    return _mag_filter;
}

}  // namespace nw4r::lyt
