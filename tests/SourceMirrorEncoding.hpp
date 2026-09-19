#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace smgpc::test {
    namespace source_mirror_detail {
        inline bool identifier_char(char value) {
            return std::isalnum(static_cast<unsigned char>(value)) || value == '_';
        }

        inline std::size_t skip_space(std::string_view source, std::size_t position) {
            while (position < source.size() && std::isspace(static_cast<unsigned char>(source[position])))
                ++position;
            return position;
        }

        inline std::size_t comment_end(std::string_view source, std::size_t position) {
            if (source.substr(position, 2) == "/*") {
                const auto end = source.find("*/", position + 2);
                return end == source.npos ? source.size() : end + 2;
            }
            if (source.substr(position, 2) != "//")
                return position;
            auto end = position + 2;
            do {
                end = source.find('\n', end);
                if (end == source.npos)
                    return source.size();
                auto previous = end;
                if (previous && source[previous - 1] == '\r')
                    --previous;
                ++end;
                if (!previous || source[previous - 1] != '\\')
                    break;
            } while (end < source.size());
            return end;
        }

        inline std::size_t skip_trivia(std::string_view source, std::size_t position) {
            while (true) {
                position = skip_space(source, position);
                const auto end = comment_end(source, position);
                if (end == position)
                    return position;
                position = end;
            }
        }

        // Return the complete literal token, including any user-defined suffix.
        // Prefixes are recognized so CP932 text inside wide/raw literals stays data.
        inline std::size_t literal_end(std::string_view source, std::size_t position) {
            const auto start = position;
            if (source.substr(position, 2) == "u8")
                position += 2;
            else if (position < source.size() && (source[position] == 'u' || source[position] == 'U' || source[position] == 'L'))
                ++position;
            const bool raw = position < source.size() && source[position] == 'R';
            if (raw)
                ++position;
            if (position >= source.size() || (source[position] != '"' && source[position] != '\''))
                return start;
            const char quote = source[position++];
            if (raw) {
                if (quote != '"')
                    return start;
                const auto opening = source.find('(', position);
                if (opening == source.npos || opening - position > 16)
                    return source.size();
                const auto closing = ")" + std::string(source.substr(position, opening - position)) + "\"";
                const auto end = source.find(closing, opening + 1);
                if (end == source.npos)
                    return source.size();
                position = end + closing.size();
            } else {
                while (position < source.size()) {
                    const auto value = source[position++];
                    if (value == '\\' && position < source.size())
                        ++position;
                    else if (value == quote)
                        break;
                }
            }
            while (position < source.size() && identifier_char(source[position]))
                ++position;
            return position;
        }
    }  // namespace source_mirror_detail

    // This intentionally preserves comments, whitespace and every non-wrapper
    // byte. It grants no general include, token, formatting or logic exemption.
    inline std::string without_cp932_source_adaptations(std::string_view source) {
        using namespace source_mirror_detail;
        constexpr std::string_view include = "#include \"compat/Cp932Literal.hpp\"\n";
        if (source.substr(0, include.size()) == include)
            source.remove_prefix(include.size());
        std::string result;
        for (std::size_t cursor = 0; cursor < source.size();) {
            if (const auto next = comment_end(source, cursor); next != cursor) {
                result.append(source.substr(cursor, next - cursor));
                cursor = next;
                continue;
            }
            const auto literal = literal_end(source, cursor);
            if (literal != cursor) {
                result.append(source.substr(cursor, literal - cursor));
                cursor = literal;
                continue;
            }
            if (identifier_char(source[cursor])) {
                auto end = cursor + 1;
                while (end < source.size() && identifier_char(source[end]))
                    ++end;
                if (source.substr(cursor, end - cursor) == "CP932") {
                    auto opening = skip_space(source, end);
                    if (opening < source.size() && source[opening] == '(') {
                        const auto first = skip_space(source, opening + 1);
                        auto last = first;
                        auto next = first;
                        while (next < source.size() && source[next] == '"') {
                            last = literal_end(source, next);
                            // An encoding wrapper may contain only ordinary
                            // literal tokens, never user-defined literal calls.
                            if (last == next || source[last - 1] != '"')
                                break;
                            next = skip_trivia(source, last);
                        }
                        if (last > first && source[last - 1] == '"' && next == skip_space(source, last) &&
                            next < source.size() && source[next] == ')') {
                            result.append(source.substr(first, last - first));
                            cursor = next + 1;
                            continue;
                        }
                    }
                }
                result.append(source.substr(cursor, end - cursor));
                cursor = end;
                continue;
            }
            result.push_back(source[cursor++]);
        }
        return result;
    }

    inline bool source_matches_with_cp932(std::string_view original, std::string_view native) {
        return original == native || original == without_cp932_source_adaptations(native);
    }
}  // namespace smgpc::test
