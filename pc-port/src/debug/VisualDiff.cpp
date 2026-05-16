#include <spng.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Image {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector< std::uint8_t > rgba;
};

struct Crop {
    std::uint32_t x = 0U;
    std::uint32_t y = 0U;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
};

struct RmsResult {
    double rgb_rms = 0.0;
    double normalized_rms = 0.0;
};

void print_usage(std::ostream &out) {
    out << "usage: smg-pc-visual-diff [--crop x,y,w,h] [--max-full-normalized-rms value] [--max-crop-normalized-rms value] <expected.png> <actual.png>\n";
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

[[nodiscard]] std::uint32_t parse_u32(std::string_view text, std::string_view field_name) {
    if (text.empty()) {
        throw std::runtime_error("empty crop " + std::string(field_name));
    }

    auto value = std::uint64_t {};
    for (const auto ch : text) {
        if (ch < '0' || ch > '9') {
            throw std::runtime_error("invalid crop " + std::string(field_name) + ": " + std::string(text));
        }
        value = value * 10U + static_cast< std::uint64_t >(ch - '0');
        if (value > std::numeric_limits< std::uint32_t >::max()) {
            throw std::runtime_error("crop " + std::string(field_name) + " is too large");
        }
    }

    return static_cast< std::uint32_t >(value);
}

[[nodiscard]] double parse_double(std::string_view text, std::string_view field_name) {
    if (text.empty()) {
        throw std::runtime_error("empty " + std::string(field_name));
    }

    const auto value_text = std::string(text);
    std::size_t parsed = 0U;
    const auto value = std::stod(value_text, &parsed);
    if (parsed != value_text.size() || !std::isfinite(value) || value < 0.0) {
        throw std::runtime_error("invalid " + std::string(field_name) + ": " + value_text);
    }

    return value;
}

[[nodiscard]] Crop parse_crop(std::string_view text) {
    auto fields = std::vector< std::string_view > {};
    auto start = std::size_t {};
    while (start <= text.size()) {
        const auto comma = text.find(',', start);
        fields.push_back(text.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start));
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1U;
    }

    if (fields.size() != 4U) {
        throw std::runtime_error("crop must be x,y,w,h");
    }

    auto crop = Crop {
        .x = parse_u32(fields[0], "x"),
        .y = parse_u32(fields[1], "y"),
        .width = parse_u32(fields[2], "width"),
        .height = parse_u32(fields[3], "height"),
    };

    if (crop.width == 0U || crop.height == 0U) {
        throw std::runtime_error("crop width and height must be non-zero");
    }

    return crop;
}

void validate_crop(const Crop &crop, const Image &image) {
    if (crop.x > image.width || crop.y > image.height || crop.width > image.width - crop.x || crop.height > image.height - crop.y) {
        throw std::runtime_error("crop is outside image bounds");
    }
}

[[nodiscard]] RmsResult calculate_rgb_rms(const Image &expected, const Image &actual, const Crop &crop) {
    auto sum_squares = 0.0L;

    for (auto y = 0U; y < crop.height; ++y) {
        const auto row = static_cast< std::size_t >(crop.y + y) * expected.width;
        for (auto x = 0U; x < crop.width; ++x) {
            const auto pixel = (row + crop.x + x) * 4U;
            for (auto channel = 0U; channel < 3U; ++channel) {
                const auto delta = static_cast< int >(expected.rgba[pixel + channel]) - static_cast< int >(actual.rgba[pixel + channel]);
                sum_squares += static_cast< long double >(delta * delta);
            }
        }
    }

    const auto sample_count = static_cast< long double >(crop.width) * static_cast< long double >(crop.height) * 3.0L;
    const auto rgb_rms = std::sqrt(static_cast< double >(sum_squares / sample_count));
    return RmsResult {
        .rgb_rms = rgb_rms,
        .normalized_rms = rgb_rms / 255.0,
    };
}

void print_rms(std::string_view label, const RmsResult &result) {
    std::cout << label << "_rgb_rms: " << result.rgb_rms << '\n';
    std::cout << label << "_normalized_rms: " << result.normalized_rms << '\n';
}

[[nodiscard]] bool exceeds_threshold(std::string_view label, const RmsResult &result, std::optional< double > max_normalized_rms) {
    if (!max_normalized_rms.has_value() || result.normalized_rms <= *max_normalized_rms) {
        return false;
    }

    std::cerr << label << " normalized RMS " << result.normalized_rms << " exceeds threshold " << *max_normalized_rms << '\n';
    return true;
}

}  // namespace

int main(int argc, char **argv) {
    try {
        auto crop = std::optional< Crop > {};
        auto max_full_normalized_rms = std::optional< double > {};
        auto max_crop_normalized_rms = std::optional< double > {};
        auto paths = std::vector< std::filesystem::path > {};

        for (auto i = 1; i < argc; ++i) {
            const auto arg = std::string_view(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                return 0;
            }
            if (arg == "--crop") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--crop requires x,y,w,h");
                }
                crop = parse_crop(argv[++i]);
                continue;
            }
            if (arg == "--max-full-normalized-rms") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--max-full-normalized-rms requires a value");
                }
                max_full_normalized_rms = parse_double(argv[++i], "full normalized RMS threshold");
                continue;
            }
            if (arg == "--max-crop-normalized-rms") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--max-crop-normalized-rms requires a value");
                }
                max_crop_normalized_rms = parse_double(argv[++i], "crop normalized RMS threshold");
                continue;
            }

            paths.emplace_back(arg);
        }

        if (paths.size() != 2U) {
            print_usage(std::cerr);
            return 2;
        }

        const auto expected = load_png(paths[0]);
        const auto actual = load_png(paths[1]);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "expected_size: " << expected.width << 'x' << expected.height << '\n';
        std::cout << "actual_size: " << actual.width << 'x' << actual.height << '\n';

        if (expected.width != actual.width || expected.height != actual.height) {
            throw std::runtime_error("image sizes differ; RMS requires matching dimensions");
        }

        const auto full_crop = Crop {
            .x = 0U,
            .y = 0U,
            .width = expected.width,
            .height = expected.height,
        };
        auto failed_threshold = false;
        const auto full_result = calculate_rgb_rms(expected, actual, full_crop);
        print_rms("full", full_result);
        failed_threshold = exceeds_threshold("full", full_result, max_full_normalized_rms);

        if (crop.has_value()) {
            validate_crop(*crop, expected);
            const auto crop_result = calculate_rgb_rms(expected, actual, *crop);
            print_rms("crop", crop_result);
            failed_threshold = exceeds_threshold("crop", crop_result, max_crop_normalized_rms) || failed_threshold;
        }

        return failed_threshold ? 1 : 0;
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
