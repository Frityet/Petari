#include "resource/RemixSequenceResource.hpp"

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <limits>
#include <stdexcept>

namespace smgpc::resource {

std::vector<std::uint32_t> decode_remix_sequence(std::span<const std::uint8_t> bytes) {
    const aurora::allocation::HostAllocationScope host;
    if (bytes.size() < 4 || bytes.size() % 4 != 0)
        aurora::throw_host_exception<std::invalid_argument>("Remix sequence has an incomplete word header or payload");
    std::vector<std::uint32_t> words(bytes.size() / 4);
    for (std::size_t i = 0; i < words.size(); ++i) {
        const auto* word = bytes.data() + i * 4;
        words[i] = (std::uint32_t(word[0]) << 24) | (std::uint32_t(word[1]) << 16) |
                   (std::uint32_t(word[2]) << 8) | word[3];
    }

    std::size_t cursor = 1;
    const auto consume = [&](std::size_t count) {
        if (count > words.size() - cursor)
            aurora::throw_host_exception<std::invalid_argument>("Remix sequence contains truncated group or note data");
        const auto start = cursor;
        cursor += count;
        return start;
    };
    const auto signed_count = [](std::uint32_t value) {
        if (value > std::numeric_limits<std::int32_t>::max())
            aurora::throw_host_exception<std::invalid_argument>("Remix sequence count exceeds the original signed range");
        return std::size_t(value);
    };

    const auto groups = signed_count(words[0]);
    consume(groups); // The original parser skips the group offset table.
    for (std::size_t group = 0; group < groups; ++group) {
        const auto header = consume(2);
        const auto tracks = signed_count(words[header]);
        const auto notes = signed_count(words[header + 1]);
        consume(notes);
        // One instrument word followed by four signed words per note.
        // Division bounds both the multiplication and the original pointer walk.
        const auto track_words = 1 + notes * 4;
        if (tracks > (words.size() - cursor) / track_words)
            aurora::throw_host_exception<std::invalid_argument>("Remix sequence contains truncated track data");
        consume(tracks * track_words);
    }
    return words;
}

}
