#include "resource/TextEncoding.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool value, std::string_view message) {
        if (!value) throw std::runtime_error(std::string(message));
    }

    void rejects(std::string_view input) {
        try {
            (void)smgpc::resource::encode_cp932(input);
        } catch (const std::runtime_error&) {
            return;
        }
        throw std::runtime_error("Invalid or unrepresentable UTF-8 was accepted");
    }
}

int main() {
    using smgpc::resource::decode_cp932;
    using smgpc::resource::encode_cp932;
    const std::array samples{
        std::pair<std::string_view, std::string_view>{"", ""},
        std::pair<std::string_view, std::string_view>{"共通着地普通", "\x8b\xa4\x92\xca\x92\x85\x92\x6e\x95\x81\x92\xca"},
        std::pair<std::string_view, std::string_view>{"属性", "\x91\xae\x90\xab"},
        std::pair<std::string_view, std::string_view>{"フェードワイプ", "\x83\x74\x83\x46\x81\x5b\x83\x68\x83\x8f\x83\x43\x83\x76"},
        std::pair<std::string_view, std::string_view>{"ｱｲｳｴｵ", "\xb1\xb2\xb3\xb4\xb5"},
        std::pair<std::string_view, std::string_view>{"①", "\x87\x40"},
        std::pair<std::string_view, std::string_view>{"名前\\後半", "\x96\xbc\x91\x4f\x5c\x8c\xe3\x94\xbc"},
    };
    for (const auto& [unicode, bytes] : samples) {
        require(encode_cp932(unicode) == bytes, "CP932 encoded bytes differ from original name bytes");
        require(decode_cp932(bytes) == unicode, "CP932 presentation decoding failed");
    }

    std::string ascii;
    for (unsigned i = 0; i < 128; ++i) ascii.push_back(static_cast<char>(i));
    require(encode_cp932(ascii) == ascii, "ASCII including embedded NUL must preserve every byte");
    require(decode_cp932(ascii) == ascii, "ASCII presentation must preserve every byte");
    const auto embedded = std::string("前\0後", 7);
    const auto embedded_bytes = std::string("\x91\x4f\0\x8c\xe3", 5);
    require(encode_cp932(embedded) == embedded_bytes, "Embedded NUL truncated Game text");
    require(decode_cp932(embedded_bytes) == embedded, "Embedded NUL truncated presentation text");

    std::string long_input, long_bytes;
    for (unsigned i = 0; i < 8192; ++i) {
        long_input += samples[1].first;
        long_bytes += samples[1].second;
    }
    require(encode_cp932(long_input) == long_bytes, "Long CP932 conversion was truncated");
    require(decode_cp932(long_bytes) == long_input, "Long presentation conversion was truncated");

    // Malformed UTF-8, surrogate scalars, values above U+10FFFF, and valid
    // Unicode without a CP932 representation may not acquire substitute keys.
    const std::array invalid{
        std::string_view("\x80"), std::string_view("\xc0\xaf"),
        std::string_view("\xe3\x81"), std::string_view("\xe3\x28\x82"),
        std::string_view("\xed\xa0\x80"), std::string_view("\xf4\x90\x80\x80"),
        std::string_view("\xf5\x80\x80\x80"), std::string_view("😀"),
        std::string_view("é"), std::string_view("a\0😀", 6),
    };
    for (auto input : invalid) rejects(input);
    // Error paths close their converter and do not affect later requests.
    for (unsigned i = 0; i < 256; ++i) {
        rejects("😀");
        require(encode_cp932(samples[1].first) == samples[1].second, "Conversion after rejection failed");
    }
    std::cout << "CP932 names=" << samples.size() << " ascii_bytes=128 embedded_nul=pass long_names=8192 "
                 "invalid_or_unrepresentable=10 recovery_cycles=256\n";
}
