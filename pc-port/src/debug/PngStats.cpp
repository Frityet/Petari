#include <spng.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Image {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector< std::uint8_t > rgba;
};

void print_usage(std::ostream &out) {
    out << "usage: smg-pc-png-stats <image.png>\n";
}

[[nodiscard]] std::vector< std::uint8_t > read_file(const std::filesystem::path &path) {
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot open " + path.string());
    }

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    if (size < 0) {
        throw std::runtime_error("cannot determine size of " + path.string());
    }

    auto bytes = std::vector< std::uint8_t >(static_cast< std::size_t >(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast< char * >(bytes.data()), static_cast< std::streamsize >(bytes.size()));
    if (!file && !bytes.empty()) {
        throw std::runtime_error("failed to read " + path.string());
    }

    return bytes;
}

void free_spng_ctx(spng_ctx *ctx) {
    if (ctx != nullptr) {
        spng_ctx_free(ctx);
    }
}

[[nodiscard]] Image load_png(const std::filesystem::path &path) {
    const auto bytes = read_file(path);
    auto ctx = std::unique_ptr< spng_ctx, decltype(&free_spng_ctx) >(spng_ctx_new(0), free_spng_ctx);
    if (ctx == nullptr) {
        throw std::runtime_error("failed to allocate PNG decoder");
    }

    if (spng_set_png_buffer(ctx.get(), bytes.data(), bytes.size()) != 0) {
        throw std::runtime_error("failed to set PNG buffer for " + path.string());
    }

    auto ihdr = spng_ihdr {};
    if (spng_get_ihdr(ctx.get(), &ihdr) != 0) {
        throw std::runtime_error("failed to read PNG header from " + path.string());
    }

    auto decoded_size = std::size_t {};
    if (spng_decoded_image_size(ctx.get(), SPNG_FMT_RGBA8, &decoded_size) != 0) {
        throw std::runtime_error("failed to determine decoded PNG size for " + path.string());
    }

    auto image = Image {
        .width = ihdr.width,
        .height = ihdr.height,
        .rgba = std::vector< std::uint8_t >(decoded_size),
    };

    if (spng_decode_image(ctx.get(), image.rgba.data(), image.rgba.size(), SPNG_FMT_RGBA8, SPNG_DECODE_TRNS) != 0) {
        throw std::runtime_error("failed to decode PNG " + path.string());
    }

    return image;
}

[[nodiscard]] std::uint32_t rgb_key(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return (static_cast< std::uint32_t >(r) << 16U) | (static_cast< std::uint32_t >(g) << 8U) | static_cast< std::uint32_t >(b);
}

void print_json_string(std::string_view text) {
    std::cout << '"';
    for (const char ch : text) {
        switch (ch) {
        case '\\':
            std::cout << "\\\\";
            break;
        case '"':
            std::cout << "\\\"";
            break;
        case '\n':
            std::cout << "\\n";
            break;
        case '\r':
            std::cout << "\\r";
            break;
        case '\t':
            std::cout << "\\t";
            break;
        default:
            std::cout << ch;
            break;
        }
    }
    std::cout << '"';
}

void print_stats(const std::filesystem::path &path, const Image &image) {
    const auto pixel_count = static_cast< std::uint64_t >(image.width) * static_cast< std::uint64_t >(image.height);
    auto nonblack = std::uint64_t {};
    auto max_channel = std::uint32_t {};
    auto sums = std::array< std::uint64_t, 3 > {};
    auto unique = std::set< std::uint32_t > {};

    for (auto pixel = std::size_t {}; pixel + 3U < image.rgba.size(); pixel += 4U) {
        const auto r = image.rgba[pixel];
        const auto g = image.rgba[pixel + 1U];
        const auto b = image.rgba[pixel + 2U];
        if (r > 8U || g > 8U || b > 8U) {
            ++nonblack;
        }
        max_channel = std::max< std::uint32_t >(max_channel, std::max({r, g, b}));
        sums[0] += r;
        sums[1] += g;
        sums[2] += b;
        if (unique.size() < 4096U) {
            unique.insert(rgb_key(r, g, b));
        }
    }

    const auto mean = [&](std::size_t channel) {
        if (pixel_count == 0U) {
            return 0.0;
        }
        return static_cast< double >(sums[channel]) / static_cast< double >(pixel_count);
    };
    const auto nonblack_ratio = pixel_count == 0U ? 0.0 : static_cast< double >(nonblack) / static_cast< double >(pixel_count);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << '{';
    std::cout << "\"path\":";
    print_json_string(path.string());
    std::cout << ",\"width\":" << image.width;
    std::cout << ",\"height\":" << image.height;
    std::cout << ",\"nonblack_pixels\":" << nonblack;
    std::cout << ",\"nonblack_ratio\":" << nonblack_ratio;
    std::cout << ",\"max_channel\":" << max_channel;
    std::cout << ",\"mean_rgb\":[" << mean(0) << ',' << mean(1) << ',' << mean(2) << ']';
    std::cout << ",\"sample_unique_rgb\":" << unique.size();
    std::cout << "}\n";
}

}  // namespace

int main(int argc, char **argv) {
    try {
        if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
            print_usage(std::cout);
            return 0;
        }
        if (argc != 2) {
            print_usage(std::cerr);
            return 2;
        }
        const auto path = std::filesystem::path(argv[1]);
        print_stats(path, load_png(path));
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
